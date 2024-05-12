#include "SimNaniteCommon.hlsl"

#define DEBUG_CLUSTER_GROUP 1

#define GROUP_SIZE 32
#define GROUP_PROCESS_GROUP_NUM GROUP_SIZE
#define GROUP_PROCESS_CLUSTER_NUM GROUP_SIZE

cbuffer CullingParameters : register(b0)
{
    float4 planes[6];
    uint total_instance_num;
    float3 camera_world_pos;
};

ByteAddressBuffer cluster_group_init_task_queue: register(t0);
StructuredBuffer<SSimNaniteClusterGroup> scene_cluster_group_infos: register(t1);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t2);

RWByteAddressBuffer cluster_group_task_queue: register(u0);
globallycoherent RWStructuredBuffer<SQueuePassState> queue_pass_state: register(u1);
#if DEBUG_CLUSTER_GROUP
RWStructuredBuffer<SClusterGroupCullVis> cluster_group_cull_vis: register(u2);
RWByteAddressBuffer cluster_group_culled_num: register(u3); //todo: clear
#endif


groupshared uint group_clu_group_processed_mask;
groupshared uint group_start_cul_group_idx;

groupshared int group_clu_group_task_num;

void ProcessClusterGroup(uint instance_index,uint clu_group_index)
{
    const float4x4 world_matrix = culled_instance_scene_data[instance_index].world_matrix;
    float3 bounding_center = mul(world_matrix, float4(scene_cluster_group_infos[clu_group_index].bound_sphere_center, 1.0)).xyz;

    float3 scale = float3(world_matrix[0][0], world_matrix[1][1], world_matrix[2][2]);
    float max_scale = max(max(scale.x, scale.y), scale.z);
    float bounding_radius = scene_cluster_group_infos[clu_group_index].bound_sphere_radius * max_scale;

    bool is_culled = false;
    bool is_use_next_lod = false;

    // frustum culling
    {
        bool in_frustum = true;
        for(int i = 0; i < 6; i++)
        {
            in_frustum = in_frustum && (DistanceFromPoint(planes[i],bounding_center) + bounding_radius > 0.0f);
        }

        if(in_frustum == false)
        {
           is_culled = true;
        }
    }

    // hzb culling
    {

    }

    // compute the lod
    if(!is_culled)
    {
        float dist = distance(bounding_center, camera_world_pos);
        if(dist > scene_cluster_group_infos[clu_group_index].cluster_next_lod_dist)
        {
            is_use_next_lod = true;
        }
    }

    int group_task_change_num = -1;

    if(is_use_next_lod)
    {
        uint child_group_num = scene_cluster_group_infos[clu_group_index].child_group_num;
        uint write_offset;

        //todo: wave vote optimization
        InterlockedAdd(queue_pass_state[0].group_task_write_offset, child_group_num, write_offset);

        for(uint idx = 0; idx < child_group_num; idx++)
        {
            uint child_group_idx = scene_cluster_group_infos[clu_group_index].child_group_start_index[idx];
            uint2 write_group_task = uint2(instance_index, child_group_idx);
            cluster_group_task_queue.Store2(write_offset + idx, write_group_task);
        }
        group_task_change_num += child_group_num;
    }
    else
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

    InterlockedAdd(queue_pass_state[0].clu_group_num, group_task_change_num);
}



[numthreads(GROUP_SIZE, 1, 1)]
void PersistentCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint init_clu_group_num = queue_pass_state[0].init_clu_group_num;
    uint group_index = groupIndex;

    bool has_clu_group_task = true;
    uint clu_group_start_idx = 0;
    uint clu_group_processed_size = GROUP_PROCESS_GROUP_NUM;

    //while(true)
    for(int debug_idx = 0; debug_idx < 2; debug_idx++)
    {
        GroupMemoryBarrierWithGroupSync();
        
        if (group_index == 0)
		{
			group_clu_group_processed_mask = 0;
		}

        GroupMemoryBarrierWithGroupSync();

        uint clu_group_processed_mask = 0;
        if(has_clu_group_task)
        {
            if(clu_group_processed_size == GROUP_PROCESS_GROUP_NUM)
            {
                if(group_index == 0)
                {
                    InterlockedAdd(queue_pass_state[0].group_task_offset, GROUP_PROCESS_GROUP_NUM, group_start_cul_group_idx);
                }
                GroupMemoryBarrierWithGroupSync();

                clu_group_processed_size = 0;
                clu_group_start_idx = group_start_cul_group_idx;
            }

            const uint clu_group_task_index = clu_group_start_idx + clu_group_processed_size + group_index;
            bool is_clu_group_ready = (clu_group_processed_size + group_index) < GROUP_PROCESS_GROUP_NUM;

            uint2 group_task;
            if(clu_group_task_index < init_clu_group_num)// load from init cluster group task queue
            {
                group_task = cluster_group_init_task_queue.Load2(clu_group_task_index * 8); //uint2 = 8 bytes
            }
            else
            {
                group_task = cluster_group_task_queue.Load2(clu_group_task_index * 8); //uint2 = 8 bytes
                is_clu_group_ready = is_clu_group_ready && (group_task.x != 0) && (group_task.y != 0);
            }

            const uint group_task_instance_index = group_task.x;
            const uint group_task_clu_group_index = group_task.y;

            if(is_clu_group_ready)
            {
                InterlockedOr(group_clu_group_processed_mask, 1u << group_index);
            }
            AllMemoryBarrierWithGroupSync();

            clu_group_processed_mask = group_clu_group_processed_mask;

            if(clu_group_processed_mask & 1u)
            {
                int batch_group_size = firstbitlow(~clu_group_processed_mask);
                batch_group_size = (batch_group_size == -1) ? GROUP_SIZE : batch_group_size;
                if (group_index < uint(batch_group_size))
                {
                    ProcessClusterGroup(group_task_instance_index, group_task_clu_group_index);
                }
                clu_group_processed_size += int(batch_group_size);
                continue;
            }
        }   

        if (group_index == 0)
        {
            group_clu_group_task_num = queue_pass_state[0].clu_group_num;
            
            //todo: fix me
            //group_clu_task_size = cluster_task_batch_size.Load(clu_task_start_index);;
        }
        GroupMemoryBarrierWithGroupSync();

        //uint clu_task_ready_size = group_clu_task_size;
        uint clu_task_ready_size = 0;
        if(!has_clu_group_task && clu_task_ready_size == 0)
        {
            break;
        }

        int clu_group_task_num = group_clu_group_task_num;
        
        //debug only
        //if (has_clu_group_task && clu_group_task_num == 0)
        if (has_clu_group_task && clu_group_task_num <= 0)
        {
            has_clu_group_task = false;
        }
    }
}