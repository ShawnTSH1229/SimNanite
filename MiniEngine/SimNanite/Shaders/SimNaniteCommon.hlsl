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
    uint node_task_read_offset;
    uint cluster_task_read_offset;

    uint node_task_write_offset;
    uint cluster_task_write_offset;

    int node_num;
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

    float cluster_pre_lod_dist;
    float cluster_next_lod_dist; // the last lod dist is infinite

    uint cluster_num;
    uint cluster_start_index; // the index of the scene global cluster array
};

struct SSimNaniteMesh
{
    uint root_node_num;
    uint root_node_indices[8]; // max lod number
};

struct SSimNaniteBVHNode
{
    uint child_node_indices[4];
    float3 bound_sphere_center;
    float bound_sphere_radius;

    uint is_leaf_node;
    uint clu_group_idx;

    uint padding_0;
    uint padding_1;
};

// indirect draw
struct SSimNaniteClusterDraw
{
    float4x4 world_matrix;

    uint index_count;
    uint start_index_location;
    uint start_vertex_location;

    uint material_idx;
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

struct SClusterGroupCullVis
{
    uint cluster_num;
    uint cluster_start_index;
    uint culled_instance_index;
};

struct SPersistentCullIndirectCmd
{
    uint thread_group_count_x;
    uint thread_group_count_y;
    uint thread_group_count_z;
};

float DistanceFromPoint(float4 plane, float3 in_point)
{
    return dot(in_point, normalize(plane.xyz)) + plane.w;
}

