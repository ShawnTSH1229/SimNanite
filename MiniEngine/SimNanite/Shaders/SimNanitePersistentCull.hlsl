#include "SimNaniteCommon.hlsl"

#define DEBUG_CLUSTER_GROUP 1

#define GROUP_SIZE 32
#define GROUP_PROCESS_NODE_NUM GROUP_SIZE
#define GROUP_PROCESS_CLUSTER_NUM GROUP_SIZE

cbuffer CullingParameters : register(b0)
{
    float4 planes[6];
    uint total_instance_num;
    float3 camera_world_pos;
};

StructuredBuffer<SSimNaniteBVHNode> scene_nodes: register(t0);
StructuredBuffer<SSimNaniteClusterGroup> scene_cluster_group_infos: register(t1);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t2);

RWByteAddressBuffer node_task_queue: register(u0);//todo: insert a barrier for waw resource
globallycoherent RWStructuredBuffer<SQueuePassState> queue_pass_state: register(u1);
#if DEBUG_CLUSTER_GROUP
RWStructuredBuffer<SClusterGroupCullVis> cluster_group_cull_vis: register(u2);
RWByteAddressBuffer cluster_group_culled_num: register(u3); //todo: clear
#endif


groupshared uint group_node_processed_mask;
groupshared uint group_start_node_idx;

groupshared int group_node_task_num;

void ProcessNode(uint instance_index,uint node_index)
{
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

        if(in_frustum == false)
        {
           is_culled = true;
        }
    }

    // hzb culling
    {

    }

     int node_task_change_num = -1;
    if(is_culled == false)
    {
        if(bvh_node.is_leaf_node)
        {
            float dist = distance(bounding_center, camera_world_pos);
            uint clu_group_index = bvh_node.clu_group_idx;
            SSimNaniteClusterGroup clu_group = scene_cluster_group_infos[clu_group_index];
            if(dist < clu_group.cluster_next_lod_dist && dist > clu_group.cluster_pre_lod_dist)
            {
                #if DEBUG_CLUSTER_GROUP
                uint debug_write_index = 0;
                cluster_group_culled_num.InterlockedAdd(0, 1, debug_write_index);

                SClusterGroupCullVis cluster_group_vis = (SClusterGroupCullVis)0;
                cluster_group_vis.cluster_num = scene_cluster_group_infos[clu_group_index].cluster_num;
                cluster_group_vis.cluster_start_index = scene_cluster_group_infos[clu_group_index].cluster_start_index;
                cluster_group_vis.culled_instance_index = instance_index;
                cluster_group_cull_vis[debug_write_index] = cluster_group_vis;
                #endif
            }
        }
        else
        {
            uint write_offset;
            //todo: wave vote optimization
            InterlockedAdd(queue_pass_state[0].node_task_write_offset, 4, write_offset);

            for(uint cld_node_idx = 0; cld_node_idx < 4; cld_node_idx++)
            {
                uint2 write_node_task = uint2(instance_index, bvh_node.child_node_indices[cld_node_idx]);
                node_task_queue.Store2((write_offset + cld_node_idx) * 8 /* sizeof(uint2)*/, write_node_task);
            }

            node_task_change_num += 4;
        }
    }

    InterlockedAdd(queue_pass_state[0].node_num, node_task_change_num);
}



[numthreads(GROUP_SIZE, 1, 1)]
void PersistentCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint group_index = groupIndex;

    bool has_node_task = true;
    uint node_start_idx = 0;
    uint node_processed_size = GROUP_PROCESS_NODE_NUM;

    //while(true)
    for(int debug_idx = 0; debug_idx < 50; debug_idx++)
    {
        GroupMemoryBarrierWithGroupSync();
        
        if (group_index == 0)
		{
			group_node_processed_mask = 0;
		}

        GroupMemoryBarrierWithGroupSync();

        uint node_processed_mask = 0;
        if(has_node_task)
        {
            if(node_processed_size == GROUP_PROCESS_NODE_NUM)
            {
                if(group_index == 0)
                {
                    InterlockedAdd(queue_pass_state[0].node_task_offset, GROUP_PROCESS_NODE_NUM, group_start_node_idx);
                }
                GroupMemoryBarrierWithGroupSync();

                node_processed_size = 0;
                node_start_idx = group_start_node_idx;
            }

            const uint node_task_index = node_start_idx + node_processed_size + group_index;
            bool is_node_ready = (node_processed_size + group_index) < GROUP_PROCESS_NODE_NUM;

            uint2 group_task = node_task_queue.Load2(node_task_index * 8);
            is_node_ready = is_node_ready && (group_task.x != 0) && (group_task.y != 0);

            const uint group_task_instance_index = group_task.x;
            const uint group_task_node_index = group_task.y;

            if(is_node_ready)
            {
                InterlockedOr(group_node_processed_mask, 1u << group_index);
            }
            AllMemoryBarrierWithGroupSync();

            node_processed_mask = group_node_processed_mask;

            if(node_processed_mask & 1u)
            {
                int batch_node_size = firstbitlow(~node_processed_mask);
                batch_node_size = (node_processed_mask == 0xFFFFFFFFu) ? GROUP_SIZE : batch_node_size;
                if (group_index < uint(batch_node_size))
                {
                    ProcessNode(group_task_instance_index, group_task_node_index);
                }
                node_processed_size += int(batch_node_size);
                continue;
            }
        }   

        if (group_index == 0)
        {
            group_node_task_num = queue_pass_state[0].node_num;
            
            //todo: fix me
            //group_clu_task_size = cluster_task_batch_size.Load(clu_task_start_index);;
        }
        GroupMemoryBarrierWithGroupSync();

        //uint clu_task_ready_size = group_clu_task_size;
        uint clu_task_ready_size = 0;
        if(!has_node_task && clu_task_ready_size == 0)
        {
            break;
        }

        int node_task_num = group_node_task_num;
        
        //debug only
        //if (has_node_task && node_task_num == 0)
        if (has_node_task && node_task_num <= 0)
        {
            has_node_task = false;
        }
    }
}