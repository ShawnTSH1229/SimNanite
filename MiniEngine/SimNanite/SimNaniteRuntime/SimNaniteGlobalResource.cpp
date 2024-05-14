#include "SimNaniteGlobalResource.h"

static SSimNaniteGlobalResource gSimNaniteResource;

SSimNaniteGlobalResource& GetSimNaniteGlobalResource()
{
    return gSimNaniteResource;
}

static void InitializeInstanceSceneData()
{
	int total_instance_num = 0;
	for (int mesh_idx = 0; mesh_idx < gSimNaniteResource.m_mesh_instances.size(); mesh_idx++)
	{
		total_instance_num += gSimNaniteResource.m_mesh_instances[mesh_idx].m_instance_datas.size();
	}
	gSimNaniteResource.m_nanite_instance_scene_data.resize(total_instance_num);

	int global_ins_idx = 0;
	for (int mesh_idx = 0; mesh_idx < gSimNaniteResource.m_mesh_instances.size(); mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = gSimNaniteResource.m_mesh_instances[mesh_idx];
		SNaniteInstanceSceneData instance_scene_data = {};
		instance_scene_data.m_nanite_resource_id = mesh_idx;
		for (int ins_idx = 0; ins_idx < mesh_instance.m_instance_datas.size(); ins_idx++)
		{
			DirectX::BoundingSphere bouding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bouding_sphere, mesh_instance.m_nanite_mesh_resource.m_bouding_box);
			bouding_sphere.Transform(instance_scene_data.m_bouding_sphere, mesh_instance.m_instance_datas[ins_idx].World);
			instance_scene_data.m_world = mesh_instance.m_instance_datas[ins_idx].World;
			gSimNaniteResource.m_nanite_instance_scene_data[global_ins_idx] = instance_scene_data;
			global_ins_idx++;
		}
	}

	{
		gSimNaniteResource.m_inst_scene_data_gpu.Create(L"m_inst_scene_data_gpu", gSimNaniteResource.m_nanite_instance_scene_data.size(), sizeof(SNaniteInstanceSceneData), gSimNaniteResource.m_nanite_instance_scene_data.data());
		gSimNaniteResource.m_culled_ins_scene_data_gpu.Create(L"m_culled_ins_scene_data_gpu", gSimNaniteResource.m_nanite_instance_scene_data.size(), sizeof(SNaniteInstanceSceneData), nullptr);
		gSimNaniteResource.m_culled_instance_num.Create(L"m_culled_instance_num", 1, sizeof(uint32_t), nullptr);

		GraphicsContext& InitContext = GraphicsContext::Begin();
		InitContext.TransitionResource(gSimNaniteResource.m_inst_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		InitContext.TransitionResource(gSimNaniteResource.m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		InitContext.TransitionResource(gSimNaniteResource.m_culled_instance_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		InitContext.ClearUAV(gSimNaniteResource.m_culled_instance_num);
		InitContext.Finish();
	}
}

static void InitPersistentCullGPUResource()
{
	gSimNaniteResource.m_queue_pass_state.Create(L"m_queue_pass_state", 1, sizeof(SQueuePassState), nullptr);
	std::vector<SSimNaniteMesh>& m_scene_mesh_infos_cpu = gSimNaniteResource.m_scene_mesh_infos_cpu;
	std::vector<SSimNaniteCluster>& m_scene_cluster_infos_cpu = gSimNaniteResource.m_scene_cluster_infos_cpu;
	std::vector<SSimNaniteClusterGroup>& m_scene_cluster_group_infos_cpu = gSimNaniteResource.m_scene_cluster_group_infos_cpu;
	std::vector<SSimNaniteBVHNode> m_scene_bvh_nodes_infos_cpu = gSimNaniteResource.m_scene_bvh_nodes_infos_cpu;

	std::vector<SNaniteMeshInstance>& mesh_instances = gSimNaniteResource.m_mesh_instances;
	
	int scene_mesh_infos_size = mesh_instances.size();
	int scene_cluster_infos_size = 0;
	int scene_cluster_group_info_size = 0;
	int scene_node_info_size = 0;
	int scene_vertex_num = 0;
	int scene_index_num = 0;

	for (int mesh_idx = 0; mesh_idx < scene_mesh_infos_size; mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = mesh_instances[mesh_idx];
		scene_cluster_infos_size += mesh_instance.m_nanite_mesh_resource.m_clusters.size();
		scene_cluster_group_info_size += mesh_instance.m_nanite_mesh_resource.m_cluster_groups.size();
		scene_node_info_size += mesh_instance.m_nanite_mesh_resource.m_bvh_nodes.size();
		scene_vertex_num += mesh_instance.m_nanite_mesh_resource.m_positions.size();
		scene_index_num += mesh_instance.m_nanite_mesh_resource.m_indices.size();
	}

	m_scene_mesh_infos_cpu.resize(scene_mesh_infos_size);
	m_scene_cluster_infos_cpu.resize(scene_cluster_infos_size);
	m_scene_cluster_group_infos_cpu.resize(scene_cluster_group_info_size + 1);
	m_scene_bvh_nodes_infos_cpu.resize(scene_node_info_size);

	m_scene_cluster_group_infos_cpu[0] = SSimNaniteClusterGroup();;
	int scene_cluster_group_info_idx = 1;
	int scene_cluster_info_idx = 0;
	int scene_node_info_idx = 0;

	std::vector<DirectX::XMFLOAT3> positions_data;
	std::vector<DirectX::XMFLOAT3> normal_data;
	std::vector<DirectX::XMFLOAT2> uv_data;
	std::vector<unsigned int> indices_data;
	positions_data.resize(scene_vertex_num);
	normal_data.resize(scene_vertex_num);
	uv_data.resize(scene_vertex_num);
	indices_data.resize(scene_index_num);

	uint32_t vertex_offsets = 0;
	uint32_t index_offsets = 0;

	for (int mesh_idx = 0; mesh_idx < scene_mesh_infos_size; mesh_idx++)
	{
		SNaniteMeshInstance& mesh_instance = mesh_instances[mesh_idx];
		CSimNaniteMeshResource& mesh_resource = mesh_instance.m_nanite_mesh_resource;

		memcpy(((uint8_t*)positions_data.data()) + vertex_offsets * sizeof(DirectX::XMFLOAT3), mesh_resource.m_positions.data(), mesh_resource.m_positions.size() * sizeof(DirectX::XMFLOAT3));
		memcpy(((uint8_t*)normal_data.data()) + vertex_offsets * sizeof(DirectX::XMFLOAT3), mesh_resource.m_normals.data(), mesh_resource.m_normals.size() * sizeof(DirectX::XMFLOAT3));
		memcpy(((uint8_t*)uv_data.data()) + vertex_offsets * sizeof(DirectX::XMFLOAT2), mesh_resource.m_uvs.data(), mesh_resource.m_uvs.size() * sizeof(DirectX::XMFLOAT2));
		
		memcpy(((uint8_t*)indices_data.data()) + index_offsets * sizeof(unsigned int), mesh_resource.m_indices.data(), mesh_resource.m_indices.size() * sizeof(unsigned int));

		SSimNaniteMesh nanite_mesh_gpu;
		nanite_mesh_gpu.m_root_node_num = mesh_instance.m_nanite_mesh_resource.m_nanite_lods.size();
		for (uint32_t root_idx = 0; root_idx < nanite_mesh_gpu.m_root_node_num; root_idx++)
		{
			nanite_mesh_gpu.m_root_node_indices[root_idx] = mesh_instance.m_nanite_mesh_resource.m_nanite_lods[root_idx].m_root_node_index + scene_node_info_idx;
		}
		m_scene_mesh_infos_cpu[mesh_idx] = nanite_mesh_gpu;

		for (int clu_group_idx = 0; clu_group_idx < mesh_resource.m_cluster_groups.size(); clu_group_idx++)
		{
			CSimNaniteClusterGrpupResource& nanite_cluster_group_resource = mesh_resource.m_cluster_groups[clu_group_idx];

			DirectX::BoundingSphere bounding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bounding_sphere, nanite_cluster_group_resource.m_bouding_box);

			SSimNaniteClusterGroup nanite_cluster_group;
			nanite_cluster_group.bound_sphere_center = bounding_sphere.Center;
			nanite_cluster_group.bound_sphere_radius = bounding_sphere.Radius;

			nanite_cluster_group.cluster_pre_lod_dist = nanite_cluster_group_resource.cluster_pre_lod_dist;
			nanite_cluster_group.cluster_next_lod_dist = nanite_cluster_group_resource.cluster_next_lod_dist;
			nanite_cluster_group.cluster_num = nanite_cluster_group_resource.m_cluster_num;
			nanite_cluster_group.cluster_start_index = scene_cluster_info_idx + nanite_cluster_group_resource.m_clusters_indices[0];
			m_scene_cluster_group_infos_cpu[scene_cluster_group_info_idx + clu_group_idx] = nanite_cluster_group;
		}

		for (int clu_idx = 0; clu_idx < mesh_resource.m_clusters.size(); clu_idx++)
		{
			CSimNaniteClusterResource& nanite_cluster_resource = mesh_resource.m_clusters[clu_idx];

			DirectX::BoundingSphere bounding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bounding_sphere, nanite_cluster_resource.m_bouding_box);

			SSimNaniteCluster nanite_cluster;
			nanite_cluster.bound_sphere_center = bounding_sphere.Center;
			nanite_cluster.bound_sphere_radius = bounding_sphere.Radius;

			nanite_cluster.mesh_index = mesh_idx;
			nanite_cluster.index_count = nanite_cluster_resource.m_index_count;
			nanite_cluster.start_index_location = nanite_cluster_resource.m_start_index_location + index_offsets;
			nanite_cluster.start_vertex_location = nanite_cluster_resource.m_start_vertex_location + vertex_offsets;
			m_scene_cluster_infos_cpu[scene_cluster_info_idx + clu_idx] = nanite_cluster;
		}

		for (int node_idx = 0; node_idx < mesh_resource.m_bvh_nodes.size(); node_idx++)
		{
			const SClusterGroupBVHNode& bvh_node = mesh_resource.m_bvh_nodes[node_idx];

			DirectX::BoundingSphere bounding_sphere;
			DirectX::BoundingSphere::CreateFromBoundingBox(bounding_sphere, bvh_node.m_bouding_box);

			SSimNaniteBVHNode node_info;
			node_info.is_leaf_node = bvh_node.m_is_leaf_node;
			node_info.bound_sphere_center = bounding_sphere.Center;
			node_info.bound_sphere_radius = bounding_sphere.Radius;
			node_info.child_node_indices[0] = bvh_node.m_child_node_index[0] + scene_node_info_idx;
			node_info.child_node_indices[1] = bvh_node.m_child_node_index[1] + scene_node_info_idx;
			node_info.child_node_indices[2] = bvh_node.m_child_node_index[2] + scene_node_info_idx;
			node_info.child_node_indices[3] = bvh_node.m_child_node_index[3] + scene_node_info_idx;
			node_info.clu_group_idx = scene_cluster_group_info_idx + bvh_node.m_cluster_group_index;
			m_scene_bvh_nodes_infos_cpu[scene_node_info_idx + node_idx] = node_info;
		}

		scene_cluster_group_info_idx += mesh_resource.m_cluster_groups.size();
		scene_cluster_info_idx += mesh_resource.m_clusters.size();
		scene_node_info_idx += mesh_resource.m_bvh_nodes.size();

		vertex_offsets += mesh_resource.m_positions.size();
		index_offsets += mesh_resource.m_indices.size();
	}

	gSimNaniteResource.m_scene_mesh_infos.Create(L"m_scene_mesh_infos", m_scene_mesh_infos_cpu.size(), sizeof(SSimNaniteMesh), m_scene_mesh_infos_cpu.data());
	gSimNaniteResource.m_scene_cluster_infos.Create(L"m_scene_cluster_infos", m_scene_cluster_infos_cpu.size(), sizeof(SSimNaniteCluster), m_scene_cluster_infos_cpu.data());
	gSimNaniteResource.m_scene_cluster_group_infos.Create(L"m_scene_cluster_group_infos", m_scene_cluster_group_infos_cpu.size(), sizeof(SSimNaniteClusterGroup), m_scene_cluster_group_infos_cpu.data());
	gSimNaniteResource.m_scene_bvh_nodes_infos.Create(L"m_scene_bvh_nodes_infos", m_scene_bvh_nodes_infos_cpu.size(), sizeof(SSimNaniteBVHNode), m_scene_bvh_nodes_infos_cpu.data());

	gSimNaniteResource.m_scene_pos_vertex_buffer.Create(L"m_scene_pos_vertex_buffer", positions_data.size(), sizeof(XMFLOAT3), positions_data.data());
	gSimNaniteResource.m_scene_normal_vertex_buffer.Create(L"m_scene_normal_vertex_buffer", normal_data.size(), sizeof(XMFLOAT3), normal_data.data());
	gSimNaniteResource.m_scene_uv_vertex_buffer.Create(L"m_scene_uv_vertex_buffer", uv_data.size(), sizeof(XMFLOAT2), uv_data.data());
	gSimNaniteResource.m_scene_index_buffer.Create(L"m_scene_index_buffer", indices_data.size(), sizeof(unsigned int), indices_data.data());

	int cluster_group_task_num = SIM_NANITE_CLUSTER_GROUP_PER_LOD * 5 * SIM_NANITE_MAX_INSTANCE_NUM_PER_MESH * SIM_NANITE_MAX_MESH_NUM;
	gSimNaniteResource.m_cluster_group_cull_vis.Create(L"cluster_group_cull_vis", cluster_group_task_num, sizeof(SClusterGroupCullVis));
	gSimNaniteResource.m_cluster_group_culled_num.Create(L"m_cluster_group_culled_num",1,sizeof(uint32_t));

	gSimNaniteResource.hardware_indirect_draw_cmds.Create(L"hardware_indirect_draw_cmd", 200 * 100, sizeof(SSimNaniteClusterDraw));
	gSimNaniteResource.hardware_indirect_draw_num.Create(L"hardware_indirect_draw_num", 1, sizeof(uint32_t));
	gSimNaniteResource.hardware_draw_indirect.Create(L"hardware_draw_indirect", 1, sizeof(D3D12_DRAW_ARGUMENTS));
}

void InitGlobalResource()
{
	InitializeInstanceSceneData();
	InitPersistentCullGPUResource();

}
