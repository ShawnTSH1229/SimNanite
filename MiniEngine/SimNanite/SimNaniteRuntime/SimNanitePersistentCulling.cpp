#include "SimNanitePersistentCulling.h"

void CPersistentCulling::Init()
{
	{
		m_persistent_cull_init_sig.Reset(2);
		m_persistent_cull_init_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
		m_persistent_cull_init_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		m_persistent_cull_init_sig.Finalize(L"PersistentCullInit");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNanitePersistentCullInit.hlsl", L"InitPersistentCull", L"cs_5_1", nullptr, 0);
		m_persistent_cull_init_pso.SetRootSignature(m_persistent_cull_init_sig);
		m_persistent_cull_init_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_persistent_cull_init_pso.Finalize();
	}

	{
		gen_indirect_dispacth_sig.Reset(2);
		gen_indirect_dispacth_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		gen_indirect_dispacth_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		gen_indirect_dispacth_sig.Finalize(L"gen_indirect_dispacth_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteIndirectDispatchGen.hlsl", L"GeneratePersistentCullIndirectCmd", L"cs_5_1", nullptr, 0);
		gen_indirect_dispacth_pso.SetRootSignature(gen_indirect_dispacth_sig);
		gen_indirect_dispacth_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		gen_indirect_dispacth_pso.Finalize();

		persistent_cull_indirect_cmd.Create(L"persistent_cull_indirect_cmd", 3, sizeof(uint32_t));
	}

	{
		persistent_cull_sig.Reset(3);
		persistent_cull_sig[0].InitAsConstantBuffer(0);
		persistent_cull_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
		persistent_cull_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
		persistent_cull_sig.Finalize(L"persistent_cull_sig");

#if CLUGROUP_CULL_DEBUG
		D3D_SHADER_MACRO debug_node_cull;
		debug_node_cull.Definition = "1";
		debug_node_cull.Name = "DEBUG_CLUSTER_GROUP";

		D3D_SHADER_MACRO defines[1] = { debug_node_cull };
		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNanitePersistentCull.hlsl", L"PersistentCull", L"cs_5_1", defines, 1);
#else
		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNanitePersistentCull.hlsl", L"PersistentCull", L"cs_5_1", nullptr, 0);
#endif
		persistent_cull_pso.SetRootSignature(persistent_cull_sig);
		persistent_cull_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		persistent_cull_pso.Finalize();

		constexpr int cluster_group_task_num = SIM_NANITE_CLUSTER_GROUP_PER_LOD * SIM_NANITE_MAX_LOD * SIM_NANITE_MAX_INSTANCE_NUM_PER_MESH * SIM_NANITE_MAX_MESH_NUM;
		m_node_task_queue.Create(L"m_node_task_queue", cluster_group_task_num, sizeof(uint32_t) * 2, nullptr);
	}
}

void CPersistentCulling::GPUCull(ComputeContext& Context)
{
	//init pass
	{
		Context.SetRootSignature(m_persistent_cull_init_sig);
		Context.SetPipelineState(m_persistent_cull_init_pso);

		// SRV
		Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_instance_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource ().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_mesh_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(m_node_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.ClearUAV(m_node_task_queue);

		//SRV
		Context.SetDynamicDescriptors(0, 0, 1, &GetSimNaniteGlobalResource().m_culled_instance_num.GetSRV());
		Context.SetDynamicDescriptors(0, 1, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());
		Context.SetDynamicDescriptors(0, 2, 1, &GetSimNaniteGlobalResource().m_scene_mesh_infos.GetSRV());

		//UAV
		Context.SetDynamicDescriptors(1, 0, 1, &m_node_task_queue.GetUAV());
		Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetUAV());

		int max_instance = SIM_NANITE_MAX_INSTANCE_NUM_PER_MESH * SIM_NANITE_MAX_MESH_NUM;

		Context.Dispatch((max_instance + 32 - 1) / 32, 1, 1);
	}

	Context.InsertUAVBarrier(m_node_task_queue);

	{
		Context.SetRootSignature(gen_indirect_dispacth_sig);
		Context.SetPipelineState(gen_indirect_dispacth_pso);

		Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(persistent_cull_indirect_cmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.SetDynamicDescriptors(0, 0, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetSRV());
		Context.SetDynamicDescriptors(1, 0, 1, &persistent_cull_indirect_cmd.GetUAV());

		Context.Dispatch(1, 1, 1);
	}

	// persistent cull
	{
		Context.SetRootSignature(persistent_cull_sig);
		Context.SetPipelineState(persistent_cull_pso);

		//Indirect
		Context.TransitionResource(persistent_cull_indirect_cmd, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		// SRV
		Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_bvh_nodes_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_group_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(m_node_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#if CLUGROUP_CULL_DEBUG
		Context.TransitionResource(GetSimNaniteGlobalResource().m_cluster_group_cull_vis, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_cluster_group_culled_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
		Context.FlushResourceBarriers();

		Context.ClearUAV(GetSimNaniteGlobalResource().m_cluster_group_culled_num);

		Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters), &GetSimNaniteGlobalResource().m_culling_parameters);

		//SRV
		Context.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().m_scene_bvh_nodes_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_cluster_group_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());

		//UAV
		Context.SetDynamicDescriptors(2, 0, 1, &m_node_task_queue.GetUAV());
		Context.SetDynamicDescriptors(2, 1, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetUAV());
#if CLUGROUP_CULL_DEBUG
		Context.SetDynamicDescriptors(2, 2, 1, &GetSimNaniteGlobalResource().m_cluster_group_cull_vis.GetUAV());
		Context.SetDynamicDescriptors(2, 3, 1, &GetSimNaniteGlobalResource().m_cluster_group_culled_num.GetUAV());
#endif
		Context.DispatchIndirect(persistent_cull_indirect_cmd);
	}
	
}
