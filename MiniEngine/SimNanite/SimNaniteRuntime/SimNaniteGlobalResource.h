#pragma once
#include "SimNaniteMeshInstance.h"

#define SIM_NANITE_MAX_CLUSTER_BATCH_PER_MESH 64 /* =  2048 Cluster / 32 */
#define SIM_NANITE_MAX_INSTANCE_NUM_PER_MESH 100
#define SIM_NANITE_MAX_MESH_NUM 4 
#define SIM_NANITE_CLUSTER_GROUP_PER_LOD 8
#define SIM_NANITE_MAX_LOD 5



#define CLUGROUP_CULL_DEBUG 0

__declspec(align(256)) struct SCullingParameters
{
	Math::BoundingPlane m_planes[6];
	uint32_t m_total_instance_num;
	float m_camera_world_position_x;
	float m_camera_world_position_y;
	float m_camera_world_position_z;
};

struct SNaniteInstanceSceneData
{
	Math::Matrix4 m_world;
	DirectX::BoundingSphere m_bouding_sphere; // world space bounding box
	
	uint32_t m_nanite_resource_id;
	
	float padding_0;
	float padding_1;
	float padding_2;
};

struct SQueuePassState
{
	uint32_t node_task_offset;
	uint32_t cluster_task_offset;

	uint32_t node_task_write_offset;
	uint32_t cluster_task_write_offset;

	uint32_t node_num;
	uint32_t global_dispatch_indirect_size;
};

struct SSimNaniteClusterDraw
{
	Math::Matrix4 m_world;

	uint32_t index_count;
	uint32_t start_index_location;
	uint32_t start_vertex_location;

	float padding_0;
};

struct SSimNaniteCluster
{
	DirectX::XMFLOAT3 bound_sphere_center;
	float bound_sphere_radius;

	uint32_t mesh_index;

	uint32_t index_count;
	uint32_t start_index_location;
	uint32_t start_vertex_location;
};

struct SSimNaniteClusterGroup
{
	DirectX::XMFLOAT3 bound_sphere_center;
	float bound_sphere_radius;

	float cluster_pre_lod_dist;
	float cluster_next_lod_dist; // the last lod dist is infinite

	uint32_t cluster_num;
	uint32_t cluster_start_index; // the index of the scene global cluster array
};

struct SSimNaniteMesh
{
	uint32_t m_root_node_num; //lod num
	uint32_t m_root_node_indices[8];
};

struct SSimNaniteBVHNode
{
	uint32_t child_node_indices[4];
	
	DirectX::XMFLOAT3 bound_sphere_center;
	float bound_sphere_radius;

	uint32_t is_leaf_node;
	uint32_t clu_group_idx;

	uint32_t padding_0;
	uint32_t padding_1;
};

struct SClusterGroupCullVis
{
	uint32_t cluster_num;
	uint32_t cluster_start_index;
	uint32_t culled_instance_index;
};

struct SSimNaniteGlobalResource
{
	// Initialized In App Setup
	std::vector<SNaniteMeshInstance> m_mesh_instances;
	DescriptorHeap s_TextureHeap;
	DescriptorHeap s_SamplerHeap;
	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;
	CShaderCompiler m_shader_compiler;

	// Updated in RenderScene
	D3D12_GPU_VIRTUAL_ADDRESS m_view_constant_address;

	// Updated in App Update
	SCullingParameters m_culling_parameters;

	// Initialized In InitGlobalResource

	/* Instance Scene Data*/
	std::vector<SNaniteInstanceSceneData> m_nanite_instance_scene_data;
	StructuredBuffer m_inst_scene_data_gpu;
	StructuredBuffer m_culled_ins_scene_data_gpu;
	ByteAddressBuffer m_culled_instance_num;

	/* Instance Queue State*/
	StructuredBuffer m_queue_pass_state;

	/* Persistent Cull Scene Data*/
	std::vector<SSimNaniteMesh>m_scene_mesh_infos_cpu;
	std::vector<SSimNaniteCluster>m_scene_cluster_infos_cpu;
	std::vector<SSimNaniteClusterGroup> m_scene_cluster_group_infos_cpu;
	std::vector<SSimNaniteBVHNode> m_scene_bvh_nodes_infos_cpu;
	
	StructuredBuffer m_scene_mesh_infos;
	StructuredBuffer m_scene_cluster_infos;
	StructuredBuffer m_scene_cluster_group_infos;
	StructuredBuffer m_scene_bvh_nodes_infos;

	ByteAddressBuffer m_scene_pos_vertex_buffer;
	ByteAddressBuffer m_scene_normal_vertex_buffer;
	ByteAddressBuffer m_scene_uv_vertex_buffer;
	ByteAddressBuffer m_scene_index_buffer;

	/* Rasterization */
	
	StructuredBuffer scene_indirect_draw_cmds; // 0 - 2w hardware draw, 2w - 2.5w software dispatch

	ByteAddressBuffer hardware_indirect_draw_num;
	StructuredBuffer hardware_draw_indirect;

	ByteAddressBuffer software_indirect_draw_num;
	StructuredBuffer software_draw_indirect;

	// Visualize Cluster Group
	StructuredBuffer m_cluster_group_cull_vis;
	ByteAddressBuffer m_cluster_group_culled_num;

	// Debug variable
	// 5 / 2 increase decrease cluster visualize lod
	// 1 update freezing data
	
	// 7 : visualize instance culling
	// 0 : default, visualize cluster
	int m_vis_cluster_lod = 0;
	bool m_need_update_freezing_data = true;

	int vis_type = 0;
};

SSimNaniteGlobalResource& GetSimNaniteGlobalResource();
void InitGlobalResource();