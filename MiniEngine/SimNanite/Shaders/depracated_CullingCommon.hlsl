// max cluster size = 64 (cluster batch per mesh) * 200 (instance number) * 4 (mesh number) * 8 ( sizeof(uint2) ) = 51200 * 8 = 409,600 = 400KB
// max cluster group size = 8 ( cluster group per lod ) * 5 ( max lod ) * 200 ( instance number ) * 4 (mesh number) * 8 ( sizeof(uint2) )  = 32,000 * 8 = 256,000 = 250KB
// max init cluster group size = 8 ( cluster group per lod ) * 1 ( max lod ) * 200 ( instance number ) * 4 (mesh number) * 8 ( sizeof(uint2) )  = 6,400 * 8 = 51,200 = 250KB
// max instance num = 200 ( instance number ) * 4 ( mesh number )
// max indirect draw command  = max clucster num

struct SNaniteInstanceSceneData
{
    float4x4 worlt_mat;   // Object to world
    float3 bounding_center;
    float bounding_radius;
    
    uint nanite_res_id;
    // todo:remove these member
    uint nanite_sub_instance_id;

    float2 padding;
};

// Persistent cull
struct SSimNaniteCluster
{
    float3 bound_sphere_center;
    float bound_sphere_radius;

    uint mesh_index;

    uint index_count;
    uint start_index_location;
    uint start_vertex_location;
};

struct SSimNaniteClusterGroup
{
    float3 bound_sphere_center;
    float bound_sphere_radius;

    float cluster_next_lod_dist; // the last lod dist is infinite

    uint child_group_num;
    uint child_group_index[8]; // the index of the scene global cluster group array

    uint cluster_num;
    uint cluster_start_index; // the index of the scene global cluster array
};

struct SSimNaniteMesh
{
    uint lod0_cluster_group_num;
    uint lod0_cluster_group_start_index;
};

struct SHardwareRasterizationIndirectCommand
{
    uint2 mesh_constant_gpu_address; //b0
    uint2 global_constant_gpu_address; //b1
    uint2 instance_data_gpu_address; //t0

    uint2 pos_vertex_buffer_gpu_address; //vb
    uint2 index_buffer_gpu_address; //ib

    uint IndexCountPerInstance;
	uint InstanceCount;
	uint StartIndexLocation;
	int  BaseVertexLocation;
	uint StartInstanceLocation;
};

struct SSoftwareRasterizationIndirectCommand
{
    uint2 mesh_constant_gpu_address; //b0
    uint2 global_constant_gpu_address; //b1
    uint2 instance_data_gpu_address; //t0
    uint2 pos_vertex_buffer_gpu_address; //t1
    uint2 index_buffer_gpu_address; //t2
    uint2 rasterize_parameters_gpu_address; //t2

    //hack here! optimize this step in the future
    uint thread_group_x;
    uint thread_group_y;
    uint thread_group_z;
};

struct SQueuePassState
{
    uint group_task_offset;
    uint cluster_task_offset;

    uint group_task_write_offset;
    uint cluster_task_write_offset;

    uint clu_group_num;
    uint init_clu_group_num;

    uint global_dispatch_indirect_size;
};

float DistanceFromPoint(float4 plane, float3 in_point)
{
    return dot(in_point, normalize(plane.xyz)) + plane.w;
}