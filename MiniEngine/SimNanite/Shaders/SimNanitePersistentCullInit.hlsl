#include "SimNaniteCommon.hlsl"

#define GROUP_SIZE 32

ByteAddressBuffer culled_instance_num_buffer : register(t0);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t1);
StructuredBuffer<SSimNaniteMesh> scene_mesh_infos: register(t2);

RWByteAddressBuffer out_cluster_group_task_queue: register(u0);
RWStructuredBuffer<SQueuePassState> queue_pass_state_to_init: register(u1); // cleared in instance cull pass

[numthreads(GROUP_SIZE, 1, 1)]
void InitPersistentCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * GROUP_SIZE) + groupIndex;
    uint culled_instance_num = culled_instance_num_buffer.Load(0);
    
    if(index < culled_instance_num)
    {
        uint mesh_index = culled_instance_scene_data[index].nanite_res_id;
        uint lod0_cluster_group_num = scene_mesh_infos[mesh_index].lod0_cluster_group_num;
        uint lod0_cluster_group_start_index = scene_mesh_infos[mesh_index].lod0_cluster_group_start_index;

        uint gloabl_task_write_index = 0;
        InterlockedAdd(queue_pass_state_to_init[0].init_clu_group_num, lod0_cluster_group_num, gloabl_task_write_index);
        InterlockedAdd(queue_pass_state_to_init[0].clu_group_num, lod0_cluster_group_num);

        for(uint write_idx = 0; write_idx < lod0_cluster_group_num; write_idx++)
        {
            out_cluster_group_task_queue.Store2(gloabl_task_write_index + write_idx, uint2(index,lod0_cluster_group_start_index + write_idx));
        }
    }
}