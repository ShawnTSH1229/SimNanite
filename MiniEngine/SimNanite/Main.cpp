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
#include "SimNaniteRuntime/SimNaniteCull.h"
#include "SimNaniteRuntime/SimNaniteRasterizer.h"
#include "SimNaniteRuntime/SimNaniteBasePass.h"

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

    D3D12_VIEWPORT m_MainViewport;
    D3D12_RECT m_MainScissor;

    DescriptorHeap s_TextureHeap;
    DescriptorHeap s_SamplerHeap;

    RootSignature m_RootSig;

    CShaderCompiler m_shader_compiler;
    
    GraphicsPSO pbr_pso;
    GraphicsPSO unlit_pso;

    std::vector<SNaniteMeshInstance> m_mesh_instances;
    SNaniteBasePassContext nanite_base_pass_ctx;

    CNaniteCuller m_nanite_culler;
    CSimNaniteRasterizer m_nanite_rasterizer;
    CSimNaniteBasePass m_nanite_base_pass;

    enum RootBindings
    {
        kMeshConstants,
        kGlobalConstants,
        kInstanceBufferSRVs,
        kMaterialSRVs,
        kMaterialSamplers,
        kNumRootBindings
    };

#if DEBUG_TEST
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

    m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
    m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
    m_MainViewport.MinDepth = 0.0f;
    m_MainViewport.MaxDepth = 1.0f;

    m_MainScissor.left = 0;
    m_MainScissor.top = 0;
    m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
    m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

    m_shader_compiler.Init();

    {

        m_RootSig.Reset(kNumRootBindings, 0);
        m_RootSig[kMeshConstants].InitAsConstantBuffer(0);
        m_RootSig[kGlobalConstants].InitAsConstantBuffer(1);
        m_RootSig[kInstanceBufferSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        m_RootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_RootSig[kMaterialSamplers].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_RootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,      1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

    //shader pbr pso
    {
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = m_shader_compiler.Compile(L"Shaders/mat_pbr.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = m_shader_compiler.Compile(L"Shaders/mat_pbr.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        pbr_pso = GraphicsPSO(L"pbr pbr");
        pbr_pso.SetRootSignature(m_RootSig);
        pbr_pso.SetRasterizerState(RasterizerDefault);
        pbr_pso.SetBlendState(BlendDisable);
        pbr_pso.SetDepthStencilState(DepthStateReadWrite);
        pbr_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        pbr_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pbr_pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
        pbr_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        pbr_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        pbr_pso.Finalize();
    }

    {
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = m_shader_compiler.Compile(L"Shaders/mat_unlit.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = m_shader_compiler.Compile(L"Shaders/mat_unlit.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        unlit_pso = GraphicsPSO(L"unlit pbr");
        unlit_pso.SetRootSignature(m_RootSig);
        unlit_pso.SetRasterizerState(RasterizerDefault);
        unlit_pso.SetBlendState(BlendDisable);
        unlit_pso.SetDepthStencilState(DepthStateReadWrite);
        unlit_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        unlit_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        unlit_pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
        unlit_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        unlit_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        unlit_pso.Finalize();
    }

    s_TextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    s_SamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);

    CreateExampleNaniteMeshInstances(m_mesh_instances, s_TextureHeap, s_SamplerHeap);
    m_mesh_instances[0].m_gfx_pso = pbr_pso;
    m_mesh_instances[1].m_gfx_pso = unlit_pso;

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

    const Vector3 cam_position = m_Camera.GetPosition();
#if 0
    m_nanite_culler.UpdataCullingParameters(m_Camera.GetWorldSpaceFrustum(), DirectX::XMFLOAT3(cam_position.GetX(), cam_position.GetY(), cam_position.GetZ()));
#endif
    m_CameraController->Update(deltaT);
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
    //m_nanite_culler.GPUCull(cptContext, cb.GpuAddress);
    
    cptContext.Finish();
    
    //nanite_base_pass_ctx.m_nanite_gloabl_constant_address = cb.GpuAddress;
    //m_nanite_base_pass.Render(&nanite_base_pass_ctx);
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
    
    const D3D12_VIEWPORT& viewport = m_MainViewport;
    const D3D12_RECT& scissor = m_MainScissor;
    
    {
        gfxContext.SetRootSignature(m_RootSig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, s_TextureHeap.GetHeapPointer());
        gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, s_SamplerHeap.GetHeapPointer());
    
        // Set common shader constants
        gfxContext.SetConstantBuffer(kGlobalConstants, cb.GpuAddress);
    
        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        gfxContext.ClearColor(g_SceneColorBuffer);
        gfxContext.ClearDepth(g_SceneDepthBuffer);
    
        gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
    
        gfxContext.SetViewportAndScissor(viewport, scissor);
        gfxContext.FlushResourceBarriers();
    
        for (int idx = 0; idx < m_mesh_instances.size(); idx++)
        {
            SNaniteMeshInstance& mesh_instance = m_mesh_instances[idx];
    
            gfxContext.SetConstantBuffer(kMeshConstants, mesh_instance.m_mesh_constants_gpu.GetGpuVirtualAddress());
            gfxContext.SetDescriptorTable(kInstanceBufferSRVs, s_TextureHeap[mesh_instance.m_instance_buffer_idx]);
            gfxContext.SetDescriptorTable(kMaterialSRVs, s_TextureHeap[mesh_instance.m_tex_table_index]);
            gfxContext.SetDescriptorTable(kMaterialSamplers, s_SamplerHeap[mesh_instance.m_sampler_table_idx]);
    
            gfxContext.SetPipelineState(mesh_instance.m_gfx_pso);
    
            gfxContext.SetVertexBuffer(0, mesh_instance.m_vertex_pos_buffer.VertexBufferView());
            gfxContext.SetVertexBuffer(1, mesh_instance.m_vertex_norm_buffer.VertexBufferView());
            gfxContext.SetVertexBuffer(2, mesh_instance.m_vertex_uv_buffer.VertexBufferView());
            gfxContext.SetIndexBuffer(mesh_instance.m_index_buffer.IndexBufferView());
    
            int instance_count = mesh_instance.m_instance_datas.size();
    
            CSimNaniteMeshResource& nanite_mesh_res = mesh_instance.m_nanite_mesh_resource;
            uint32_t want_visualize_lod = 1;
            //uint32_t visualize_lod = nanite_mesh_res.m_nanite_lods.size() > want_visualize_lod ? want_visualize_lod : nanite_mesh_res.m_nanite_lods.size() - 1;
            const CSimNaniteLodResource& nanite_lod = nanite_mesh_res.m_nanite_lods[want_visualize_lod];
            for (int nanite_group_idx = 0; nanite_group_idx < nanite_lod.m_cluster_group_num; nanite_group_idx++)
            {
                int sub_group_idx = nanite_lod.m_cluster_group_index[nanite_group_idx];
                const CSimNaniteClusterGrpupResource& nanite_group = nanite_mesh_res.m_cluster_groups[sub_group_idx];
    
                for (int clu_idx = 0; clu_idx < nanite_group.m_cluster_num; clu_idx++)
                {
                    int sub_clu_idx = nanite_group.m_clusters_indices[clu_idx];
                    const CSimNaniteClusterResource& nanite_clu = nanite_mesh_res.m_clusters[sub_clu_idx];
    
                    gfxContext.DrawIndexedInstanced(nanite_clu.m_index_count, instance_count, nanite_clu.m_start_index_location, nanite_clu.m_start_vertex_location, 0);
                }
            }
        }
    }
    
    gfxContext.Finish();
}
