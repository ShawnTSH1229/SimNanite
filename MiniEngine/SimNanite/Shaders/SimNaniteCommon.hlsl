// max cluster size = 64 (cluster batch per mesh) * 200 (instance number) * 4 (mesh number) * 8 ( sizeof(uint2) ) = 51200 * 8 = 409,600 = 400KB
// max cluster group size = 8 ( cluster group per lod ) * 5 ( max lod ) * 200 ( instance number ) * 4 (mesh number) * 8 ( sizeof(uint2) )  = 32,000 * 8 = 256,000 = 250KB
// max init cluster group size = 8 ( cluster group per lod ) * 1 ( max lod ) * 200 ( instance number ) * 4 (mesh number) * 8 ( sizeof(uint2) )  = 6,400 * 8 = 51,200 = 250KB
// max instance num = 200 ( instance number ) * 4 ( mesh number )
// max indirect draw command  = max clucster num

struct SNaniteInstanceSceneData
{
    float4x4 world_matrix;
    float3 bounding_center; // world space bounding box
    float bounding_radius;
    uint nanite_res_id;

    float padding_0;
	float padding_1;
	float padding_2;
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

// culling
struct SSimNaniteCluster
{
    float3 bound_sphere_center;
    float bound_sphere_radius;

    uint mesh_index;

    uint index_count;
    uint start_index_location;
    uint start_vertex_location;
};

// culling
struct SSimNaniteClusterGroup
{
    float3 bound_sphere_center;
    float bound_sphere_radius;

    float cluster_next_lod_dist; // the last lod dist is infinite

    uint child_group_num;
    uint child_group_start_index[8]; // the index of the scene global cluster group array

    uint cluster_num;
    uint cluster_start_index; // the index of the scene global cluster array
};

struct SSimNaniteMesh
{
    uint lod0_cluster_group_num;
    uint lod0_cluster_group_start_index;
};

// indirect draw
struct SSimNaniteClusterDraw
{
    float4x4 world_matrix;

    uint index_count;
    uint start_index_location;
    uint start_vertex_location;

    float padding_0;
};

//typedef struct D3D12_DRAW_ARGUMENTS
//    {
//    UINT VertexCountPerInstance;
//    UINT InstanceCount;
//    UINT StartVertexLocation;
//    UINT StartInstanceLocation;
//    } 	D3D12_DRAW_ARGUMENTS;
struct SIndirectDrawParameters
{
    uint vertex_count;
    uint instance_count;
    uint start_vertex_location;
    uint start_instance_location;
};


float DistanceFromPoint(float4 plane, float3 in_point)
{
    return dot(in_point, normalize(plane.xyz)) + plane.w;
}

