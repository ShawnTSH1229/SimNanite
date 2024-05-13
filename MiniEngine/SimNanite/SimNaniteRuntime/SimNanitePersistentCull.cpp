#include "SimNanitePersistentCull.h"

void CPersistentCullPass::Init(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context)
{
	CreatePSO(*nanite_cull_init_desc.shader_compiler);
	InitBuffer(nanite_cull_init_desc, nanite_cull_context);

	nanite_cull_init_desc.base_pass_ctx->scene_cluster_infos = &m_scene_cluster_infos;
}

void CPersistentCullPass::UpdataCullingParameters(const Frustum& frustum, DirectX::XMFLOAT3 camera_world_pos, SNaniteCullingContext* nanite_cull_context)
{

}

void CPersistentCullPass::GPUCull(ComputeContext& Context, SNaniteCullingContext* nanite_cull_context)
{
	// clear buffer
	// clear cluster_group_task_queue to 0xff
	// clear cluster_task_queue to 0
	// clear cluster_task_batch_size to 0
	// reset inrect draw commnad hardware_indirect_draw_command
	// reset inrect draw commnad software_indirect_draw_command

	Context.ClearUAV(cluster_group_task_queue);
	Context.ClearUAV(cluster_task_queue);
	Context.ClearUAV(cluster_task_batch_size);
	Context.ClearUAV(m_hardware_indirect_draw_command.GetCounterBuffer());
	Context.ClearUAV(m_software_indirect_draw_cmd_param.GetCounterBuffer());
	Context.ClearUAV(m_software_dispatch_indirect_command.GetCounterBuffer());

	//init pass
	{
		Context.SetRootSignature(PersistentCullInitRootSig);
		Context.SetPipelineState(PersistentCullInitPSO);

		// SRV
		Context.TransitionResource(*nanite_cull_context->m_culled_instance_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(*nanite_cull_context->m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_scene_mesh_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(m_cluster_group_init_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(m_queue_pass_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//SRV
		Context.SetDynamicDescriptors(0, 0, 1, &nanite_cull_context->m_culled_instance_num->GetSRV());
		Context.SetDynamicDescriptors(0, 1, 1, &nanite_cull_context->m_culled_ins_scene_data_gpu->GetSRV());
		Context.SetDynamicDescriptors(0, 2, 1, &m_scene_mesh_infos.GetSRV());

		//UAV
		Context.SetDynamicDescriptors(1, 0, 1, &m_cluster_group_init_task_queue.GetUAV());
		Context.SetDynamicDescriptors(1, 1, 1, &m_queue_pass_state.GetUAV());

		int max_instance = TEST_MAX_INSTANCE_NUM_PER_MESH * TEST_MAX_MESH_NUM;

		Context.Dispatch((max_instance + 32 - 1) / 32, 1, 1);
	}

	// persistent cull
	{
		Context.SetRootSignature(PersistentCullRootSig);
		Context.SetPipelineState(PersistentCullPSO);

		// SRV
		Context.TransitionResource(m_cluster_group_init_task_queue, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_scene_cluster_group_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(m_scene_mesh_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		Context.TransitionResource(*nanite_cull_context->m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(cluster_group_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(cluster_task_queue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(cluster_task_batch_size, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(m_queue_pass_state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		Context.SetDynamicConstantBufferView(0, sizeof(SPersistentCullParameter), &m_persistentCullParameter);

		//SRV
		Context.SetDynamicDescriptors(1, 0, 1, &m_cluster_group_init_task_queue.GetSRV());
		Context.SetDynamicDescriptors(1, 1, 1, &m_scene_cluster_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 2, 1, &m_scene_cluster_group_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 3, 1, &m_scene_mesh_infos.GetSRV());
		Context.SetDynamicDescriptors(1, 4, 1, &nanite_cull_context->m_culled_ins_scene_data_gpu->GetSRV());

		//UAV
		Context.SetDynamicDescriptors(2, 0, 1, &cluster_group_task_queue.GetUAV());
		Context.SetDynamicDescriptors(2, 1, 1, &cluster_task_queue.GetUAV());
		Context.SetDynamicDescriptors(2, 2, 1, &cluster_task_batch_size.GetUAV());
		Context.SetDynamicDescriptors(2, 3, 1, &m_queue_pass_state.GetUAV());

		Context.SetDynamicDescriptors(2, 4, 1, &m_hardware_indirect_draw_command.GetUAV());
		Context.SetDynamicDescriptors(2, 5, 1, &m_software_indirect_draw_cmd_param.GetUAV());

		int max_instance = TEST_MAX_INSTANCE_NUM_PER_MESH * TEST_MAX_MESH_NUM;

		Context.Dispatch((max_instance + 32 - 1) / 32, 1, 1);
	}

	// indrect command gen
	{
		Context.SetRootSignature(IndirectCmdGenRootSig);
		Context.SetPipelineState(IndirectCmdGenPSO);

		// SRV
		Context.TransitionResource(m_queue_pass_state, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		//UAV
		Context.TransitionResource(m_software_indirect_draw_cmd_param, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.TransitionResource(m_software_dispatch_indirect_command, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//SRV
		Context.SetDynamicDescriptors(0, 0, 1, &m_queue_pass_state.GetSRV());
		Context.SetDynamicDescriptors(0, 1, 1, &m_software_indirect_draw_cmd_param.GetSRV());

		//UAV
		Context.SetDynamicDescriptors(1, 0, 1, &m_software_dispatch_indirect_command.GetUAV());

		SoftwareIndirectCmdGen software_indirect_cmd_gen;
		software_indirect_cmd_gen.rasterize_parameters_gpu_address = m_software_indirect_draw_cmd_param.GetGpuVirtualAddress();
		Context.SetDynamicConstantBufferView(0, sizeof(SoftwareIndirectCmdGen), &software_indirect_cmd_gen);

		Context.Dispatch(1, 1, 1);
	}
}

void CPersistentCullPass::InitBuffer(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context)
{
	m_queue_pass_state.Create(L"None", 1, sizeof(SQueuePassState_Deprecated), nullptr);
	nanite_cull_context->m_queue_pass_state = &m_queue_pass_state;

	int cluster_group_init_task_num = TEST_CLUSTER_GROUP_PER_LOD * 1 * TEST_MAX_INSTANCE_NUM_PER_MESH * TEST_MAX_MESH_NUM;
	m_cluster_group_init_task_queue.Create(L"None", cluster_group_init_task_num, sizeof(uint32_t) * 2, nullptr);

	BuildScneneData(nanite_cull_init_desc);
	m_scene_mesh_infos.Create(L"None", m_scene_mesh_infos_cpu.size(), sizeof(SSimNaniteMesh_deprecated), m_scene_mesh_infos_cpu.data());
	m_scene_cluster_infos.Create(L"None", m_scene_cluster_infos_cpu.size(), sizeof(SSimNaniteCluster_Deprecated), m_scene_cluster_infos_cpu.data());
	m_scene_cluster_group_infos.Create(L"None", m_scene_cluster_group_infos_cpu.size(), sizeof(SSimNaniteClusterGroup_deprecated), m_scene_cluster_group_infos_cpu.data());
	
	int cluster_group_task_num = TEST_CLUSTER_GROUP_PER_LOD * 5 * TEST_MAX_INSTANCE_NUM_PER_MESH * TEST_MAX_MESH_NUM;
	cluster_group_task_queue.Create(L"None", cluster_group_task_num, sizeof(uint32_t) * 2, nullptr);

	int cluster_task_num = TEST_MAX_CLUSTER_BATCH_PER_MESH * TEST_MAX_INSTANCE_NUM_PER_MESH * TEST_MAX_MESH_NUM;
	cluster_task_queue.Create(L"None", cluster_task_num, sizeof(uint32_t) * 2, nullptr);
	cluster_task_batch_size.Create(L"None", cluster_task_num, sizeof(uint32_t), nullptr);
	m_hardware_indirect_draw_command.Create(L"None", cluster_task_num, sizeof(SHardwareRasterizationIndirectCommand));
	m_software_indirect_draw_cmd_param.Create(L"None", cluster_task_num,sizeof(SHardwareRasterizationIndirectCommand));

	m_software_dispatch_indirect_command.Create(L"None", TEST_MAX_MESH_NUM, sizeof(SSoftwareRasterizationIndirectCommand));
}

void CPersistentCullPass::BuildScneneData(SNaniteCullInitDesc& nanite_cull_init_desc)
{
	std::vector<SNaniteMeshInstance>& mesh_instances = *nanite_cull_init_desc.mesh_instances;

	int scene_mesh_infos_size = mesh_instances.size();
	int scene_cluster_infos_size = 0;
	int scene_cluster_group_info_size = 0;
	for (int mesh_idx = 0; mesh_idx < scene_mesh_infos_size; mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = mesh_instances[mesh_idx];
		scene_cluster_infos_size += mesh_instance.m_nanite_mesh_resource.m_clusters.size();
		scene_cluster_group_info_size += mesh_instance.m_nanite_mesh_resource.m_cluster_groups.size();
	}

	m_scene_mesh_infos_cpu.resize(scene_mesh_infos_size);
	m_scene_cluster_infos_cpu.resize(scene_cluster_infos_size);
	m_scene_cluster_group_infos_cpu.resize(scene_cluster_group_info_size);

	int scene_cluster_group_info_idx = 0;
	int scene_cluster_info_idx = 0;

	for (int mesh_idx = 0; mesh_idx < scene_mesh_infos_size; mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = mesh_instances[mesh_idx];
		CSimNaniteMeshResource& mesh_resource = mesh_instance.m_nanite_mesh_resource;

		SSimNaniteMesh_deprecated nanite_mesh_gpu;
		nanite_mesh_gpu.lod0_cluster_group_num = mesh_instance.m_nanite_mesh_resource.m_nanite_lods[0].m_cluster_group_num;
		nanite_mesh_gpu.lod0_cluster_group_start_index = mesh_instance.m_nanite_mesh_resource.m_nanite_lods[0].m_cluster_group_start;

		//nanite_mesh_gpu.mesh_constant_gpu_address = mesh_instance.m_mesh_constants_gpu.GetGpuVirtualAddress();
		nanite_mesh_gpu.instance_data_gpu_address = mesh_instance.m_instance_buffer.GetGpuVirtualAddress();
		nanite_mesh_gpu.pos_vertex_buffer_gpu_address = mesh_instance.m_vertex_pos_buffer.GetGpuVirtualAddress();
		nanite_mesh_gpu.index_buffer_gpu_address = mesh_instance.m_index_buffer.GetGpuVirtualAddress();
		m_scene_mesh_infos_cpu[mesh_idx] = nanite_mesh_gpu;

		for (int clu_group_idx = 0; clu_group_idx < mesh_resource.m_cluster_groups.size(); clu_group_idx++)
		{
			CSimNaniteClusterGrpupResource& nanite_cluster_group_resource = mesh_resource.m_cluster_groups[clu_group_idx];

			DirectX::BoundingSphere bounding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bounding_sphere, nanite_cluster_group_resource.m_bouding_box);

			SSimNaniteClusterGroup_deprecated nanite_cluster_group;
			nanite_cluster_group.bound_sphere_center = bounding_sphere.Center;
			nanite_cluster_group.bound_sphere_radius = bounding_sphere.Radius;

			nanite_cluster_group.cluster_next_lod_dist = nanite_cluster_group_resource.cluster_next_lod_dist;
			//nanite_cluster_group.child_group_num = nanite_cluster_group_resource.m_child_group_num;
			//for (int chd_idx = 0; chd_idx < nanite_cluster_group_resource.m_child_group_num; chd_idx++)
			//{
			//	nanite_cluster_group.child_group_index[chd_idx] = scene_cluster_group_info_idx + nanite_cluster_group_resource.m_child_group_indices[chd_idx];
			//}
			nanite_cluster_group.cluster_num = nanite_cluster_group_resource.m_cluster_num;
			nanite_cluster_group.cluster_start_index = scene_cluster_info_idx + nanite_cluster_group_resource.m_clusters_indices[0];
			m_scene_cluster_group_infos_cpu.push_back(nanite_cluster_group);
		}

		for (int clu_idx = 0; clu_idx < mesh_resource.m_clusters.size(); clu_idx++)
		{
			CSimNaniteClusterResource& nanite_cluster_resource = mesh_resource.m_clusters[clu_idx];

			DirectX::BoundingSphere bounding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bounding_sphere, nanite_cluster_resource.m_bouding_box);

			SSimNaniteCluster_Deprecated nanite_cluster;
			nanite_cluster.bound_sphere_center = bounding_sphere.Center;
			nanite_cluster.bound_sphere_radius = bounding_sphere.Radius;

			nanite_cluster.mesh_index = mesh_idx;
			nanite_cluster.index_count = nanite_cluster_resource.m_index_count;
			nanite_cluster.start_index_location = nanite_cluster_resource.m_start_index_location;
			nanite_cluster.start_vertex_location = nanite_cluster_resource.m_start_vertex_location;
			m_scene_cluster_infos_cpu.push_back(nanite_cluster);
		}

		scene_cluster_group_info_idx += mesh_resource.m_cluster_groups.size();
		scene_cluster_info_idx += mesh_resource.m_clusters.size();
	}
	m_scene_mesh_infos.Create(L"None", scene_mesh_infos_size, sizeof(SSimNaniteMesh_deprecated), m_scene_mesh_infos_cpu.data());
}

void CPersistentCullPass::CreatePSO(CShaderCompiler& shader_compiler)
{
	{
		PersistentCullInitRootSig.Reset(2);
		PersistentCullInitRootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
		PersistentCullInitRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		PersistentCullInitRootSig.Finalize(L"PersistentCullInit");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = shader_compiler.Compile(L"Shaders/PersistentCullInit.hlsl", L"InitPersistentCull", L"cs_5_1", nullptr, 0);
		PersistentCullInitPSO.SetRootSignature(PersistentCullInitRootSig);
		PersistentCullInitPSO.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		PersistentCullInitPSO.Finalize();
	}

	{
		PersistentCullRootSig.Reset(3);
		PersistentCullRootSig[0].InitAsConstantBuffer(0);
		PersistentCullRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
		PersistentCullRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 6);
		PersistentCullRootSig.Finalize(L"PersistentCull");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = shader_compiler.Compile(L"Shaders/PersistentCull.hlsl", L"CSMain", L"cs_5_1", nullptr, 0);
		PersistentCullPSO.SetRootSignature(PersistentCullRootSig);
		PersistentCullPSO.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		PersistentCullPSO.Finalize();
	}

	{
		IndirectCmdGenRootSig.Reset(3);
		IndirectCmdGenRootSig[0].InitAsConstantBuffer(0);
		IndirectCmdGenRootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		IndirectCmdGenRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		IndirectCmdGenRootSig.Finalize(L"IndirectCmdGenRootSig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = shader_compiler.Compile(L"Shaders/SoftwareRasterIndirectCmdGen.hlsl", L"IndirectCmdGen", L"cs_5_1", nullptr, 0);
		IndirectCmdGenPSO.SetRootSignature(IndirectCmdGenRootSig);
		IndirectCmdGenPSO.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		IndirectCmdGenPSO.Finalize();
	}
}
