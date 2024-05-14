#include "GameCore.h"
#include "CameraController.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "TemporalEffects.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "PostEffects.h"
#include "SSAO.h"
#include "FXAA.h"
#include "SystemTime.h"
#include "TextRenderer.h"
#include "ParticleEffectManager.h"
#include "GameInput.h"
#include "glTF.h"
#include "Renderer.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ShadowCamera.h"
#include "Display.h"

#include "SimNaniteBuilder/SimNaniteObjLoader.h"
#include "SimNaniteCommon/ShaderCompile.h"
#include "TextureConvert.h"
#include "SimNaniteRuntime/SimNaniteMeshInstance.h"
#include "SimNaniteRuntime/SimNaniteInstanceCulling.h"
#include "SimNaniteRuntime/SimNanitePersistentCulling.h"
#include "SimNaniteRuntime/SimNaniteGlobalResource.h"
#include "SimNaniteRuntime/SimNaniteVisualize.h"
#include "SimNaniteRuntime/SimNaniteRasterization.h"

//deprecated
//#include "SimNaniteRuntime/SimNaniteCull.h"
//#include "SimNaniteRuntime/SimNaniteRasterizer.h"
//#include "SimNaniteRuntime/SimNaniteBasePass.h"

#define DEBUG_TEST 0

using namespace GameCore;
using namespace Math;
using namespace Graphics;
using namespace std;
using namespace DirectX;

class SimNaniteApp : public GameCore::IGameApp
{
public:

    SimNaniteApp(void) {}

    virtual void Startup(void) override;
    virtual void Cleanup(void) override;

    virtual void Update(float deltaT) override;
    virtual void RenderScene(void) override;

private:

    Camera m_Camera;
    unique_ptr<CameraController> m_CameraController;

    //SNaniteBasePassContext nanite_base_pass_ctx;

    CSimNaniteInstanceCulling m_instance_culling;
    CPersistentCulling m_persistent_culling;
    CSimNaniteVisualizer m_nanite_visualizer;
    CRasterizationPass m_nanite_rasterizer;

#if DEBUG_TEST
    CNaniteCuller m_nanite_culler;
    CSimNaniteRasterizer m_nanite_rasterizer;
    CSimNaniteBasePass m_nanite_base_pass;

    SBuildCluster total_mesh_cluster;
#endif
};

CREATE_APPLICATION( SimNaniteApp )

void SimNaniteApp::Startup(void)
{
    MotionBlur::Enable = false;
    TemporalEffects::EnableTAA = false;
    FXAA::Enable = false;
    PostEffects::EnableHDR = false;
    PostEffects::BloomEnable = false;
    PostEffects::EnableAdaptation = false;
    SSAO::Enable = false;

    m_Camera.SetEyeAtUp(Vector3(0, 10, 0), Vector3(kZero), Vector3(kYUnitVector));
    m_Camera.SetZRange(1.0f, 10000.0f);
    m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));

    GetSimNaniteGlobalResource().m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
    GetSimNaniteGlobalResource().m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
    GetSimNaniteGlobalResource().m_MainViewport.MinDepth = 0.0f;
    GetSimNaniteGlobalResource().m_MainViewport.MaxDepth = 1.0f;
    
    GetSimNaniteGlobalResource().m_MainScissor.left = 0;
    GetSimNaniteGlobalResource().m_MainScissor.top = 0;
    GetSimNaniteGlobalResource().m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
    GetSimNaniteGlobalResource().m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

    GetSimNaniteGlobalResource().m_shader_compiler.Init();
    
    GetSimNaniteGlobalResource().s_TextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    GetSimNaniteGlobalResource().s_SamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);

    CreateExampleNaniteMeshInstances(GetSimNaniteGlobalResource().m_mesh_instances, GetSimNaniteGlobalResource().s_TextureHeap, GetSimNaniteGlobalResource().s_SamplerHeap);
    
    InitGlobalResource();

    m_nanite_visualizer.Init();
    m_instance_culling.Init();
    m_persistent_culling.Init();
    m_nanite_rasterizer.Init();

#if 0
    SNaniteCullInitDesc culler_init_desc;
    culler_init_desc.mesh_instances = &m_mesh_instances;
    culler_init_desc.shader_compiler = &m_shader_compiler;
    culler_init_desc.tex_heap = &s_TextureHeap;
    culler_init_desc.base_pass_ctx = &nanite_base_pass_ctx;
    m_nanite_culler.Init(culler_init_desc);

    SNaniteRasterizerInitDesc rasterizer_init_desc;
    rasterizer_init_desc.shader_compiler = &m_shader_compiler;
    m_nanite_rasterizer.Init(rasterizer_init_desc);

    CSimNaniteBasePassInitDesc base_pass_init_desc;
    base_pass_init_desc.m_shader_compiler = &m_shader_compiler;
    m_nanite_base_pass.Init(&base_pass_init_desc);

    nanite_base_pass_ctx.mesh_instances = &m_mesh_instances;
    nanite_base_pass_ctx.s_SamplerHeap = &s_SamplerHeap;
#endif
}

void SimNaniteApp::Cleanup(void)
{

}

namespace Graphics
{
    extern EnumVar DebugZoom;
}

void SimNaniteApp::Update(float deltaT)
{
    ScopedTimer prof(L"Update State");

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

#if 0
    m_nanite_culler.UpdataCullingParameters(m_Camera.GetWorldSpaceFrustum(), DirectX::XMFLOAT3(cam_position.GetX(), cam_position.GetY(), cam_position.GetZ()));
#endif
    m_CameraController->Update(deltaT);


    if (GameInput::IsFirstPressed(GameInput::kKey_0))
    {
        GetSimNaniteGlobalResource().vis_type = 0;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_1))
    {
        GetSimNaniteGlobalResource().m_need_update_freezing_data = true;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_2))
    {
        GetSimNaniteGlobalResource().m_vis_cluster_lod--;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_4))
    {
        GetSimNaniteGlobalResource().vis_type = 4;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_5))
    {
        GetSimNaniteGlobalResource().m_vis_cluster_lod++;
    }


    if (GameInput::IsFirstPressed(GameInput::kKey_6))
    {
        GetSimNaniteGlobalResource().vis_type = 6;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_7))
    {
        GetSimNaniteGlobalResource().vis_type = 7;
    }


    const Vector3 cam_position = m_Camera.GetPosition();
    SCullingParameters& culling_parameters = GetSimNaniteGlobalResource().m_culling_parameters;
    culling_parameters.m_camera_world_position_x = cam_position.GetX();
    culling_parameters.m_camera_world_position_y = cam_position.GetY();
    culling_parameters.m_camera_world_position_z = cam_position.GetZ();
    culling_parameters.m_total_instance_num = GetSimNaniteGlobalResource().m_nanite_instance_scene_data.size();
    for (int plane_idx = 0; plane_idx < 6; plane_idx++)
    {
        culling_parameters.m_planes[plane_idx] = m_Camera.GetWorldSpaceFrustum().GetFrustumPlane(Frustum::PlaneID(plane_idx));
    }
}

void SimNaniteApp::RenderScene(void)
{
    ComputeContext& cptContext = ComputeContext::Begin(L"Culling");

    SNanitrGlobalConstants globals;
    globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
    globals.CameraPos = m_Camera.GetPosition();
    globals.SunDirection = Math::Vector3(1, 1, 1);
    globals.SunIntensity = Math::Vector3(1, 1, 1);
    globals.RenderTargetSizeX = g_SceneColorBuffer.GetWidth();
    globals.RenderTargetSizeY = g_SceneColorBuffer.GetHeight();

    DynAlloc cb = cptContext.ReserveUploadMemory(sizeof(SNanitrGlobalConstants));
    memcpy(cb.DataPtr, &globals, sizeof(SNanitrGlobalConstants));
    GetSimNaniteGlobalResource().m_view_constant_address = cb.GpuAddress;
    
    m_instance_culling.Culling(cptContext);
    m_persistent_culling.GPUCull(cptContext);

    cptContext.Finish();
    
    m_nanite_rasterizer.Rasterization();
    m_nanite_visualizer.Render();
}
