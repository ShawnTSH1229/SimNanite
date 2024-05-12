#include "SimNaniteInstanceCull.h"

void CInstanceCullPass_Deprecated::Init(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context)
{
	CreatePSO(*nanite_cull_init_desc.shader_compiler);
	BuildNaniteInstanceSceneData(*nanite_cull_init_desc.mesh_instances, *nanite_cull_init_desc.tex_heap, nanite_cull_context);

	nanite_cull_init_desc.base_pass_ctx->culled_instance_scene_data = &m_culled_ins_scene_data_gpu;
}

void CInstanceCullPass_Deprecated::UpdataCullingParameters(const Frustum& frustum)
{
	m_cull_parameters.total_instance_num = m_nanite_instance_scene_data.size();
	for (int plane_idx = 0; plane_idx < 6; plane_idx++)
	{
		m_cull_parameters.planes[plane_idx] = frustum.GetFrustumPlane(Frustum::PlaneID(plane_idx));
	}
}

void CInstanceCullPass_Deprecated::GPUCull(ComputeContext& Context, SNaniteCullingContext* nanite_cull_context)
{
	
	Context.SetRootSignature(InstanceCullRootSig);
	Context.SetPipelineState(InstanceCullPSO);
	Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters_Deprecated), &m_cull_parameters);

	Context.TransitionResource(m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	Context.TransitionResource(m_culled_instance_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	Context.ClearUAV(m_culled_instance_num);
	Context.SetDynamicDescriptors(1, 0, 1, &m_culled_ins_scene_data_gpu.GetUAV());
	Context.SetDynamicDescriptors(1, 1, 1, &m_culled_instance_num.GetUAV());
	Context.SetDynamicDescriptors(1, 2, 1, &nanite_cull_context->m_queue_pass_state->GetUAV());

	Context.TransitionResource(m_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	Context.SetDynamicDescriptors(2, 0, 1, &m_ins_scene_data_gpu.GetSRV());
	Context.Dispatch((m_cull_parameters.total_instance_num + 64 - 1) / 64, 1, 1);
}

void CInstanceCullPass_Deprecated::CreatePSO(CShaderCompiler& shader_compiler)
{
	InstanceCullRootSig.Reset(3);
	InstanceCullRootSig[0].InitAsConstantBuffer(0);
	InstanceCullRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
	InstanceCullRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	InstanceCullRootSig.Finalize(L"InstanceCull");

	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = shader_compiler.Compile(L"Shaders/InstanceCulling.hlsl", L"CSMain", L"cs_5_1", nullptr, 0);
	InstanceCullPSO.SetRootSignature(InstanceCullRootSig);
	InstanceCullPSO.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	InstanceCullPSO.Finalize();
}

void CInstanceCullPass_Deprecated::BuildNaniteInstanceSceneData(std::vector<SNaniteMeshInstance>& mesh_instances, DescriptorHeap& tex_heap, SNaniteCullingContext* nanite_cull_context)
{
	int total_instance_num = 0;
	for (int mesh_idx = 0; mesh_idx < mesh_instances.size(); mesh_idx++)
	{
		total_instance_num += mesh_instances[mesh_idx].m_instance_datas.size();
	}
	m_nanite_instance_scene_data.resize(total_instance_num);

	int global_ins_idx = 0;
	for (int mesh_idx = 0; mesh_idx < mesh_instances.size(); mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = mesh_instances[mesh_idx];
		SNaniteInstanceSceneData_Depracated instance_scene_data = {};
		instance_scene_data.m_nanite_resource_id = mesh_idx;
		for (int ins_idx = 0; ins_idx < mesh_instance.m_instance_datas.size(); ins_idx++)
		{
			DirectX::BoundingSphere bouding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bouding_sphere, mesh_instance.m_nanite_mesh_resource.m_bouding_box);
			bouding_sphere.Transform(instance_scene_data.m_bouding_sphere, mesh_instance.m_instance_datas[ins_idx].World);
			instance_scene_data.m_world = mesh_instance.m_instance_datas[ins_idx].World;
			instance_scene_data.m_nanite_sub_instance_id = static_cast<uint32_t>(ins_idx);
			m_nanite_instance_scene_data[global_ins_idx] = instance_scene_data;
			global_ins_idx++;
		}
	}

	{
		m_ins_scene_data_gpu.Create(L"None", m_nanite_instance_scene_data.size(), sizeof(SNaniteInstanceSceneData_Depracated), m_nanite_instance_scene_data.data());
		m_culled_ins_scene_data_gpu.Create(L"None", m_nanite_instance_scene_data.size(), sizeof(SNaniteInstanceSceneData_Depracated), nullptr);
		m_culled_instance_num.Create(L"None", 1, sizeof(SCullResultInfo), nullptr);

		GraphicsContext& InitContext = GraphicsContext::Begin();
		InitContext.TransitionResource(m_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		InitContext.TransitionResource(m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		InitContext.TransitionResource(m_culled_instance_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		InitContext.ClearUAV(m_culled_instance_num);
		InitContext.Finish();
	}

	nanite_cull_context->m_culled_ins_scene_data_gpu = &m_culled_ins_scene_data_gpu;
	nanite_cull_context->m_culled_instance_num = &m_culled_instance_num;
}
