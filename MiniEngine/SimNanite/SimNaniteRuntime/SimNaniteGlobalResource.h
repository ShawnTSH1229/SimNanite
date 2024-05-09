#pragma once
#include "SimNaniteMeshInstance.h"

struct SSimNaniteGlobalResource
{
	std::vector<SNaniteMeshInstance> m_mesh_instances;

	DescriptorHeap s_TextureHeap;
	DescriptorHeap s_SamplerHeap;

	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;

	CShaderCompiler m_shader_compiler;

	D3D12_GPU_VIRTUAL_ADDRESS m_view_constant_address;

	int m_vis_cluster_lod = 0;
};

SSimNaniteGlobalResource& GetSimNaniteGlobalResource();