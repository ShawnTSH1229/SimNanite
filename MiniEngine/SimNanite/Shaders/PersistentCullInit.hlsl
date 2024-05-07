#define GROUP_SIZE 32
#include "CullingCommon.hlsl"



//!!!!!!!!!!!!!!!!!!!!!!!!
//RWByteAddressBuffer cluster_group_task_queue_to_init: register(u1); //clear this buffer to 0x0FFFxxxu (default value)

//struct SQueuePassState
//{
//    uint group_task_offset;
//    uint cluster_task_offset;
//
//    uint group_task_write_offset;
//    uint cluster_task_write_offset;
//
//    uint clu_group_num;
//    uint init_clu_group_num;
//};

ByteAddressBuffer culled_instance_num_buffer : register(t0);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t1);
StructuredBuffer<SSimNaniteMesh> scene_mesh_infos: register(t2);//mesh

RWByteAddressBuffer out_cluster_group_init_task_queue: register(u0);
RWStructuredBuffer<SQueuePassState> queue_pass_state_to_init: register(u1); // clear this struct in instance cull pass

groupshared uint group_generated_task_num;
groupshared uint gloabl_task_write_index;

// 800 instance / 32
[numthreads(GROUP_SIZE, 1, 1)]
void InitPersistentCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * GROUP_SIZE) + groupIndex;
    uint culled_instance_num = culled_instance_num_buffer.Load(0);

    if(groupIndex == 0)
    {
        group_generated_task_num = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint group_cul_group_store_index = 0;
    uint mesh_index = 0;
    uint lod0_cluster_group_num = 0;

    if(index < culled_instance_num)
    {
        mesh_index = culled_instance_scene_data[index].nanite_res_id;;
        lod0_cluster_group_num = scene_mesh_infos[mesh_index].lod0_cluster_group_num;
        InterlockedAdd(group_generated_task_num,lod0_cluster_group_num, group_cul_group_store_index);
    }

    GroupMemoryBarrierWithGroupSync();

    if(groupIndex == 0 && index < culled_instance_num)
    {
        uint add_value = group_generated_task_num;
        InterlockedAdd(queue_pass_state_to_init[0].init_clu_group_num,add_value,gloabl_task_write_index);
        InterlockedAdd(queue_pass_state_to_init[0].clu_group_num,add_value);
    }

    GroupMemoryBarrierWithGroupSync();

    if(index < culled_instance_num)
    {
        uint group_gloabl_task_write_index = gloabl_task_write_index;

        uint lod0_cluster_group_start_index = scene_mesh_infos[mesh_index].lod0_cluster_group_start_index;

        uint group_write_index = group_gloabl_task_write_index + group_cul_group_store_index;

        for(uint write_idx = 0; write_idx < lod0_cluster_group_num; write_idx++)
        {
           out_cluster_group_init_task_queue.Store2(group_write_index + write_idx,uint2(index,lod0_cluster_group_start_index + write_idx));
        }
    }

}