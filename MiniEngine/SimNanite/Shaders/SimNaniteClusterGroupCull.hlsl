#include "SimNaniteCommon.hlsl"
cbuffer CullingParameters : register(b0)
{
    float4 planes[6];
    uint total_instance_num;
    float3 camera_world_pos;
};

#define GROUP_SIZE 32
ByteAddressBuffer node_task_queue: register(t0);
StructuredBuffer<SSimNaniteBVHNode> scene_nodes: register(t1);
StructuredBuffer<SSimNaniteClusterGroup> scene_cluster_group_infos: register(t2);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t3);

RWStructuredBuffer<SQueuePassState> queue_pass_state : register(u0);
RWByteAddressBuffer cluster_task_queue: register(u1);
RWByteAddressBuffer cluster_task_batch_size: register(u2);

// 2 * 1 * 1
[numthreads(GROUP_SIZE, 1, 1)]
void ClusterGroupCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint thread_idx = groupId.x * GROUP_SIZE + groupIndex;
    int node_left_num = queue_pass_state[0].node_num;

    if(thread_idx < node_left_num)
    {
        uint node_task_write_offset = queue_pass_state[0].node_task_write_offset;
        uint node_process_index = node_task_write_offset - thread_idx - 1;

        uint2 node_task = node_task_queue.Load2(node_process_index * 8);

        const uint instance_index = node_task.x;
        const uint node_index = node_task.y;

        SSimNaniteBVHNode bvh_node = scene_nodes[node_index];

        const float4x4 world_matrix = culled_instance_scene_data[instance_index].world_matrix;
        float3 bounding_center = mul(world_matrix, float4(bvh_node.bound_sphere_center, 1.0)).xyz;

        float3 scale = float3(world_matrix[0][0], world_matrix[1][1], world_matrix[2][2]);
        float max_scale = max(max(scale.x, scale.y), scale.z);
        float bounding_radius = bvh_node.bound_sphere_radius * max_scale;

        bool is_culled = false;

        // frustum culling
        {
            bool in_frustum = true;
            for(int i = 0; i < 6; i++)
            {
                in_frustum = in_frustum && (DistanceFromPoint(planes[i], bounding_center) + bounding_radius > 0.0f);
            }

            if(in_frustum == false) { is_culled = true; }
        }

        int node_task_change_num = -1;
        if(is_culled == false)
        {
            if(bvh_node.is_leaf_node == 1)
            {
                float dist = distance(bounding_center, camera_world_pos);
                uint clu_group_index = bvh_node.clu_group_idx;
                SSimNaniteClusterGroup clu_group = scene_cluster_group_infos[clu_group_index];

                if(dist < clu_group.cluster_next_lod_dist && dist > clu_group.cluster_pre_lod_dist)
                {
                     uint child_cluster_num = scene_cluster_group_infos[clu_group_index].cluster_num;

                    uint write_offset;
                    InterlockedAdd(queue_pass_state[0].cluster_task_write_offset, 1, write_offset);

                    uint clu_start_idx = scene_cluster_group_infos[clu_group_index].cluster_start_index;
                    uint2 write_cluster_task = uint2(instance_index, clu_start_idx);

                    cluster_task_batch_size.Store(write_offset * 4 /*sizeof(uint)*/, child_cluster_num);
                    cluster_task_queue.Store2(write_offset * 8 /* sizeof(uint2)*/, write_cluster_task);
                }
            }
        }        
    }
}