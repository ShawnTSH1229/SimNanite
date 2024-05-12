#pragma once
#include "SimNaniteMeshInstance.h"

#define TEST_MAX_CLUSTER_BATCH_PER_MESH 64 /* =  2048 Cluster / 32 */
#define TEST_MAX_INSTANCE_NUM_PER_MESH 200
#define TEST_MAX_MESH_NUM 4 
#define TEST_CLUSTER_GROUP_PER_LOD 8
#define TEST_MAX_LOD 5



__declspec(align(256)) struct SCullingParameters_Deprecated
{
	Math::BoundingPlane planes[6];
	uint32_t total_instance_num;
};

__declspec(align(256)) struct SBasePassParam
{
    uint32_t material_index;
};

struct SCullResultInfo
{
	uint32_t culled_instance_num;
};

struct SNaniteInstanceSceneData_Depracated
{
	Math::Matrix4 m_world;         // Object to world
	DirectX::BoundingSphere m_bouding_sphere; //world space bounding box
	uint32_t m_nanite_resource_id;
	uint32_t m_nanite_sub_instance_id;

	float padding_0;
	float padding_1;
};

// Persistent cull
struct SSimNaniteCluster_Deprecated
{
    DirectX::XMFLOAT3 bound_sphere_center;
    float bound_sphere_radius;

    uint32_t mesh_index;
    
    uint32_t index_count;
    uint32_t start_index_location;
    uint32_t start_vertex_location;
};

struct SSimNaniteClusterGroup_deprecated
{
    DirectX::XMFLOAT3 bound_sphere_center;
    float bound_sphere_radius;

    float cluster_next_lod_dist; // the last lod dist is infinite

    uint32_t child_group_num;
    //uint32_t child_group_start_index; // the index of the scene global cluster group array
    uint32_t child_group_index[8];

    uint32_t cluster_num;
    uint32_t cluster_start_index; // the index of the scene global cluster array
};

struct SSimNaniteMesh_deprecated
{
    uint32_t lod0_cluster_group_num;
    uint32_t lod0_cluster_group_start_index;

    //handless
    uint64_t mesh_constant_gpu_address; //b0
    //uint64_t global_constant_gpu_address; //b1
    uint64_t instance_data_gpu_address; //t0

    uint64_t pos_vertex_buffer_gpu_address;
    uint64_t index_buffer_gpu_address;
};

struct SHardwareRasterizationIndirectCommand
{
    uint64_t mesh_constant_gpu_address; //b0
    uint64_t global_constant_gpu_address; //b1
    uint64_t instance_data_gpu_address; //t0

    uint64_t pos_vertex_buffer_gpu_address; //vb
    uint64_t index_buffer_gpu_address; //ib

    uint32_t IndexCountPerInstance;
    uint32_t InstanceCount;
    uint32_t StartIndexLocation;
    int  BaseVertexLocation;
    uint32_t StartInstanceLocation;
};

struct SSoftwareRasterizationIndirectCommand
{
    uint64_t mesh_constant_gpu_address; //b0
    uint64_t global_constant_gpu_address; //b1
    uint64_t instance_data_gpu_address; //t0
    uint64_t pos_vertex_buffer_gpu_address; //t1
    uint64_t index_buffer_gpu_address; //t2
    uint64_t rasterize_parameters_gpu_address; //t2

    uint32_t thread_group_x;
    uint32_t thread_group_y;
    uint32_t thread_group_z;
};
;

struct SQueuePassState_Deprecated
{
    uint32_t group_task_offset;
    uint32_t cluster_task_offset;

    uint32_t group_task_write_offset;
    uint32_t cluster_task_write_offset;

    uint32_t clu_group_num;
    uint32_t init_clu_group_num;

    uint32_t global_dispatch_indirect_size;
};

struct SNaniteCullingContext
{
    StructuredBuffer* m_queue_pass_state;
    StructuredBuffer* m_culled_ins_scene_data_gpu;
    ByteAddressBuffer* m_culled_instance_num;
    uint64_t mesh_global_constant_gpu_address;;
};

struct SNaniteRasterizationContext
{
    StructuredBuffer* m_hardware_indirect_draw_command;
    StructuredBuffer* m_software_indirect_draw_command;
};

struct SNaniteBasePassContext
{
    std::vector<SNaniteMeshInstance>* mesh_instances;
    D3D12_GPU_VIRTUAL_ADDRESS m_nanite_gloabl_constant_address;
    
    StructuredBuffer* culled_instance_scene_data;
    StructuredBuffer* scene_cluster_infos;
    
    DescriptorHeap* s_SamplerHeap;
};


struct SNaniteCullInitDesc
{
    std::vector<SNaniteMeshInstance>* mesh_instances;
    DescriptorHeap* tex_heap;
    CShaderCompiler* shader_compiler;

    SNaniteBasePassContext* base_pass_ctx;
};
