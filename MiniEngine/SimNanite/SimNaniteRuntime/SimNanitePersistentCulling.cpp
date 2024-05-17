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
		persistent_cull_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
#if CLUGROUP_CULL_DEBUG
		persistent_cull_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 9);
#else
		persistent_cull_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 7);
#endif
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

		m_cluster_task_queue.Create(L"m_cluster_task_queue", 200 * 100, sizeof(uint32_t) * 2);
		m_cluster_task_batch_size.Create(L"m_cluster_task_batch_size", 200 * 100, sizeof(uint32_t));
	}

	{
		indirect_cmd_gen_sig.Reset(2, 0);
		indirect_cmd_gen_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		indirect_cmd_gen_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		indirect_cmd_gen_sig.Finalize(L"indirect_cmd_gen_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteGenerateIndirectCmd.hlsl", L"HardwareIndirectDrawGen", L"cs_5_1", nullptr, 0);

		indirect_cmd_gen_pso = ComputePSO(L"indirect_cmd_gen_pso");
		indirect_cmd_gen_pso.SetRootSignature(indirect_cmd_gen_sig);
		indirect_cmd_gen_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		indirect_cmd_gen_pso.Finalize();
	}

	//{
	//	left_clu_group_sig.Reset(3, 0);
	//	left_clu_group_sig[0].InitAsConstantBuffer(0);
	//	left_clu_group_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
	//	left_clu_group_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
	//	left_clu_group_sig.Finalize(L"left_clu_group_sig");
	//
	//	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteClusterGroupCull.hlsl", L"ClusterGroupCull", L"cs_5_1", nullptr, 0);
	//
	//	left_clu_group_pso = ComputePSO(L"left_clu_group_pso");
	//	left_clu_group_pso.SetRootSignature(left_clu_group_sig);
	//	left_clu_group_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	//	left_clu_group_pso.Finalize();
	//}

	{
		cluster_cull_sig.Reset(3, 0);
		cluster_cull_sig[0].InitAsConstantBuffer(0);
		cluster_cull_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
		cluster_cull_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
		cluster_cull_sig.Finalize(L"cluster_cull_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteClusterCull.hlsl", L"ClusterCull", L"cs_5_1", nullptr, 0);

		cluster_cull_pso = ComputePSO(L"cluster_cull_pso");
		cluster_cull_pso.SetRootSignature(cluster_cull_sig);
		cluster_cull_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		cluster_cull_pso.Finalize();
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
		Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(m_node_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(m_cluster_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(m_cluster_task_batch_size, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().scene_indirect_draw_cmds, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().hardware_indirect_draw_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().software_indirect_draw_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

#if CLUGROUP_CULL_DEBUG
		Context.TransitionResource(GetSimNaniteGlobalResource().m_cluster_group_cull_vis, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_cluster_group_culled_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
		Context.FlushResourceBarriers();

		Context.ClearUAV(GetSimNaniteGlobalResource().m_cluster_group_culled_num);
		Context.ClearUAV(m_cluster_task_queue);
		Context.ClearUAV(m_cluster_task_batch_size);
		Context.ClearUAV(GetSimNaniteGlobalResource().hardware_indirect_draw_num);
		Context.ClearUAV(GetSimNaniteGlobalResource().software_indirect_draw_num);

		Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters), &GetSimNaniteGlobalResource().m_culling_parameters);

		//SRV
		Context.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().m_scene_bvh_nodes_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_cluster_group_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());
		Context.SetDynamicDescriptors(1, 3, 1, &GetSimNaniteGlobalResource().m_scene_cluster_infos.GetSRV());

		//UAV
		Context.SetDynamicDescriptors(2, 0, 1, &m_node_task_queue.GetUAV());
		Context.SetDynamicDescriptors(2, 1, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetUAV());
		Context.SetDynamicDescriptors(2, 2, 1, &m_cluster_task_queue.GetUAV());
		Context.SetDynamicDescriptors(2, 3, 1, &m_cluster_task_batch_size.GetUAV());
		Context.SetDynamicDescriptors(2, 4, 1, &GetSimNaniteGlobalResource().scene_indirect_draw_cmds.GetUAV());
		Context.SetDynamicDescriptors(2, 5, 1, &GetSimNaniteGlobalResource().hardware_indirect_draw_num.GetUAV());
		Context.SetDynamicDescriptors(2, 6, 1, &GetSimNaniteGlobalResource().software_indirect_draw_num.GetUAV());

#if CLUGROUP_CULL_DEBUG
		Context.SetDynamicDescriptors(2, 7, 1, &GetSimNaniteGlobalResource().m_cluster_group_cull_vis.GetUAV());
		Context.SetDynamicDescriptors(2, 8, 1, &GetSimNaniteGlobalResource().m_cluster_group_culled_num.GetUAV());
#endif
		// todo: fix me
		//Context.DispatchIndirect(persistent_cull_indirect_cmd);
		Context.Dispatch(1, 1, 1);
	}

	// left cluster group cull
	//{
	//	Context.SetRootSignature(left_clu_group_sig);
	//	Context.SetPipelineState(left_clu_group_pso);
	//
	//	// SRV
	//	Context.TransitionResource(m_cluster_task_queue, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//	Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_bvh_nodes_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//	Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_group_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//	Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//
	//	// UAV
	//	Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//	Context.TransitionResource(m_cluster_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	Context.TransitionResource(m_cluster_task_batch_size, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//
	//	Context.FlushResourceBarriers();
	//
	//	Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters), &GetSimNaniteGlobalResource().m_culling_parameters);
	//
	//	Context.SetDynamicDescriptors(1, 0, 1, &m_cluster_task_queue.GetSRV());
	//	Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_bvh_nodes_infos.GetSRV());
	//	Context.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_scene_cluster_group_infos.GetSRV());
	//	Context.SetDynamicDescriptors(1, 3, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());
	//
	//	Context.SetDynamicDescriptors(2, 0, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetUAV());
	//	Context.SetDynamicDescriptors(2, 1, 1, &m_cluster_task_queue.GetUAV());
	//	Context.SetDynamicDescriptors(2, 2, 1, &m_cluster_task_batch_size.GetUAV());
	//
	//	// 64 cluster groups
	//	Context.Dispatch(2, 1, 1);
	//}

	// left cluster cull
	{
		Context.SetRootSignature(cluster_cull_sig);
		Context.SetPipelineState(cluster_cull_pso);

		Context.TransitionResource(GetSimNaniteGlobalResource().m_queue_pass_state, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_cluster_task_queue, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_cluster_task_batch_size, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		Context.TransitionResource(GetSimNaniteGlobalResource().scene_indirect_draw_cmds, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().hardware_indirect_draw_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.FlushResourceBarriers();

		Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters), &GetSimNaniteGlobalResource().m_culling_parameters);

		Context.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetSRV());
		Context.SetDynamicDescriptors(1, 1, 1, &m_cluster_task_queue.GetSRV());
		Context.SetDynamicDescriptors(1, 2, 1, &m_cluster_task_batch_size.GetSRV());
		Context.SetDynamicDescriptors(1, 3, 1, &GetSimNaniteGlobalResource().m_scene_cluster_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 4, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());

		Context.SetDynamicDescriptors(2, 0, 1, &GetSimNaniteGlobalResource().scene_indirect_draw_cmds.GetUAV());
		Context.SetDynamicDescriptors(2, 1, 1, &GetSimNaniteGlobalResource().hardware_indirect_draw_num.GetUAV());
		Context.SetDynamicDescriptors(2, 2, 1, &GetSimNaniteGlobalResource().software_indirect_draw_num.GetUAV());

		// 320 clusters
		Context.Dispatch(10, 1, 1);
	}

	// generate draw indirect command
	{
		Context.SetRootSignature(indirect_cmd_gen_sig);
		Context.SetPipelineState(indirect_cmd_gen_pso);

		Context.TransitionResource(GetSimNaniteGlobalResource().hardware_indirect_draw_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().hardware_draw_indirect, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(GetSimNaniteGlobalResource().software_indirect_draw_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(GetSimNaniteGlobalResource().software_draw_indirect, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.FlushResourceBarriers();

		Context.SetDynamicDescriptors(0, 0, 1, &GetSimNaniteGlobalResource().hardware_indirect_draw_num.GetSRV());
		Context.SetDynamicDescriptors(0, 1, 1, &GetSimNaniteGlobalResource().software_indirect_draw_num.GetSRV());
		Context.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().hardware_draw_indirect.GetUAV());
		Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().software_draw_indirect.GetUAV());

		Context.Dispatch(1, 1, 1);
	}
	
}
