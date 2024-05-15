#include "SimNaniteCommon.hlsl"

#ifndef DEBUG_CLUSTER_GROUP
#define DEBUG_CLUSTER_GROUP 0
#endif

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

StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t3);//cluster

RWByteAddressBuffer node_task_queue: register(u0);//todo: insert a barrier for waw resource
globallycoherent RWStructuredBuffer<SQueuePassState> queue_pass_state: register(u1);
RWByteAddressBuffer cluster_task_queue: register(u2); //todo: clear
RWByteAddressBuffer cluster_task_batch_size: register(u3); //todo: clear

RWStructuredBuffer<SSimNaniteClusterDraw> hardware_indirect_draw_cmd : register(u4); 
RWByteAddressBuffer hardware_indirect_draw_num : register(u5); //todo: clear

#if DEBUG_CLUSTER_GROUP
RWStructuredBuffer<SClusterGroupCullVis> cluster_group_cull_vis: register(u3);
RWByteAddressBuffer cluster_group_culled_num: register(u4); //todo: clear
#endif

// node cull task
groupshared uint group_node_processed_mask;
groupshared uint group_start_node_idx;
groupshared int group_node_task_num;

// cluster cull task
groupshared uint group_start_clu_index;
groupshared uint group_clu_task_size;

#include "SimNaniteProcessClusterCommon.hlsl"

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

        if(in_frustum == false) { is_culled = true; }
    }

    // hzb culling
    { }

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
                #if DEBUG_CLUSTER_GROUP
                uint debug_write_index = 0;
                cluster_group_culled_num.InterlockedAdd(0, 1, debug_write_index);

                SClusterGroupCullVis cluster_group_vis = (SClusterGroupCullVis)0;
                cluster_group_vis.cluster_num = scene_cluster_group_infos[clu_group_index].cluster_num;
                cluster_group_vis.cluster_start_index = scene_cluster_group_infos[clu_group_index].cluster_start_index;
                cluster_group_vis.culled_instance_index = instance_index;
                cluster_group_cull_vis[debug_write_index] = cluster_group_vis;
                #endif

                // add cluster
                uint child_cluster_num = scene_cluster_group_infos[clu_group_index].cluster_num;

                uint write_offset;
                InterlockedAdd(queue_pass_state[0].cluster_task_write_offset, 1, write_offset);

                uint clu_start_idx = scene_cluster_group_infos[clu_group_index].cluster_start_index;
                uint2 write_cluster_task = uint2(instance_index, clu_start_idx);
    
                cluster_task_batch_size.Store(write_offset * 4 /*sizeof(uint)*/, child_cluster_num);
                cluster_task_queue.Store2(write_offset * 8 /* sizeof(uint2)*/, write_cluster_task);

                // assume cluster batch size < 32
                //for(uint idx = 0; idx < cluster_batch_size - 1; idx +1)
                //{
                //    uint clu_start_idx = scene_cluster_group_infos[clu_group_index].cluster_start_index + idx * GROUP_SIZE;
                //    uint2 write_cluster_task = uint2(clu_start_idx,instance_index);
                //
                //    cluster_task_batch_size.Store(write_offset + idx,GROUP_SIZE);
                //    cluster_task_queue.Store2(write_offset + idx,write_cluster_task);
                //}
            }
        }
        else
        {
            uint write_offset; //todo: wave vote optimization
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

    // node cull
    bool has_node_task = true;
    uint node_start_idx = 0;
    uint node_processed_size = GROUP_PROCESS_NODE_NUM;

    // cluster cull
    uint clu_task_start_index = 0xFFFFFFFFu;

    //while(true)
    for(int debug_idx = 0; debug_idx < 2000; debug_idx++)
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
                    InterlockedAdd(queue_pass_state[0].node_task_read_offset, GROUP_PROCESS_NODE_NUM, group_start_node_idx);
                }
                GroupMemoryBarrierWithGroupSync();

                node_processed_size = 0;
                node_start_idx = group_start_node_idx;
            }

            const uint node_task_index = node_start_idx + node_processed_size + group_index;
            bool is_node_ready = (node_processed_size + group_index) < GROUP_PROCESS_NODE_NUM;

            uint2 group_task = node_task_queue.Load2(node_task_index * 8);
            is_node_ready = is_node_ready && ((group_task.x != 0) || (group_task.y != 0));

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
                    node_task_queue.Store2(node_task_index * 8 /* sizeof(uint2)*/, uint2(0, 0));
                }
                node_processed_size += batch_node_size;
                continue;
            }
        }   

        if(clu_task_start_index == 0xFFFFFFFFu)
        {
            if(group_index == 0)
            {
                InterlockedAdd(queue_pass_state[0].cluster_task_read_offset, 1, group_start_clu_index);
            }
            GroupMemoryBarrierWithGroupSync();
            clu_task_start_index = group_start_clu_index;
        }

        if (group_index == 0)
        {
            group_node_task_num = queue_pass_state[0].node_num;
            group_clu_task_size = cluster_task_batch_size.Load(clu_task_start_index * 4 /*sizeof uint*/);
        }
        GroupMemoryBarrierWithGroupSync();

        uint clu_task_ready_size = group_clu_task_size;
        if(!has_node_task && clu_task_ready_size == 0)
        {
            break;
        }

        if(clu_task_ready_size > 0)
        {
            if(group_index < clu_task_ready_size)
            {
                const uint2 cluster_task = cluster_task_queue.Load2(clu_task_start_index * 8 /*sizeof(uint2)*/);
                const uint cluster_task_instance_index = cluster_task.x;
                const uint cluster_task_clu_start_index = cluster_task.y;
                ProcessCluster(cluster_task_instance_index, cluster_task_clu_start_index + group_index);
            }
            clu_task_start_index = 0xFFFFFFFFu;
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

