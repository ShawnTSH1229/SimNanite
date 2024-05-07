#pragma once
#include "SimNaniteCullCommon.h"

class CInstanceCullPass
{
public:
	void Init(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context);
	void UpdataCullingParameters(const Frustum& frustum);
	void GPUCull(ComputeContext& Context, SNaniteCullingContext* nanite_cull_context);
private:
	// Init
	void CreatePSO(CShaderCompiler& shader_compiler);
	void BuildNaniteInstanceSceneData(std::vector<SNaniteMeshInstance>& mesh_instances, DescriptorHeap& tex_heap, SNaniteCullingContext* nanite_cull_context);

	std::vector<SNaniteInstanceSceneData> m_nanite_instance_scene_data;

	StructuredBuffer m_ins_scene_data_gpu;
	StructuredBuffer m_culled_ins_scene_data_gpu;
	ByteAddressBuffer m_culled_instance_num;

	SCullingParameters m_cull_parameters;

	RootSignature InstanceCullRootSig;
	ComputePSO InstanceCullPSO;
};