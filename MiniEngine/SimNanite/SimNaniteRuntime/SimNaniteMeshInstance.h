#pragma once
#include "../SimNaniteBuilder/SimNaniteObjLoader.h"
#include "../SimNaniteCommon/ShaderCompile.h"
#include "TextureConvert.h"

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

__declspec(align(256)) struct SNanitrGlobalConstants
{
    Math::Matrix4 ViewProjMatrix;
    Math::Vector3 CameraPos;
    Math::Vector3 SunDirection;
    Math::Vector3 SunIntensity;

    float RenderTargetSizeX;
    float RenderTargetSizeY;
};

__declspec(align(256)) struct SNaniteMeshConstants
{
    Math::Matrix4 World;         // Object to world
    Math::Matrix3 WorldIT;       // Object normal to world normal
};

class SNaniteMeshInstance
{
public:
	CSimNaniteMeshResource m_nanite_mesh_resource;
	GraphicsPSO m_gfx_pso;

    ByteAddressBuffer m_mesh_constants_gpu;
    UploadBuffer m_mesh_constants_cpu;

    TextureRef m_texture;
    uint32_t m_tex_table_index;
    uint32_t m_sampler_table_idx;

    ByteAddressBuffer m_vertex_pos_buffer;
    ByteAddressBuffer m_vertex_norm_buffer;
    ByteAddressBuffer m_vertex_uv_buffer;

    ByteAddressBuffer m_index_buffer;

    std::vector<SInstanceData> m_instance_datas;
    StructuredBuffer m_instance_buffer;
    uint32_t m_instance_buffer_idx;
};

void CreateExampleNaniteMeshInstances(std::vector<SNaniteMeshInstance>& out_mesh_instances,
    DescriptorHeap& tex_heap, DescriptorHeap& sampler_heap);