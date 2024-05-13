#include "SimNaniteCommon.hlsl"

#define GROUP_SIZE 32

ByteAddressBuffer culled_instance_num_buffer : register(t0);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t1);
StructuredBuffer<SSimNaniteMesh> scene_mesh_infos: register(t2);
StructuredBuffer<SSimNaniteBVHNode> scene_nodes: register(t3);

RWByteAddressBuffer out_node_task_queue: register(u0);
RWStructuredBuffer<SQueuePassState> queue_pass_state_to_init: register(u1); // cleared in instance cull pass

[numthreads(GROUP_SIZE, 1, 1)]
void InitPersistentCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * GROUP_SIZE) + groupIndex;
    uint culled_instance_num = culled_instance_num_buffer.Load(0);
    
    if(index < culled_instance_num)
    {
        uint mesh_index = culled_instance_scene_data[index].nanite_res_id;
        uint root_node_num = scene_mesh_infos[mesh_index].root_node_num;

        uint gloabl_task_write_index = 0;
        InterlockedAdd(queue_pass_state_to_init[0].node_num, root_node_num, gloabl_task_write_index);

        for(uint write_idx = 0; write_idx < root_node_num; write_idx++)
        {
            out_node_task_queue.Store2((gloabl_task_write_index + write_idx) * 8 /* sizeof(uint2)*/, uint2(index, scene_mesh_infos[mesh_index].root_node_indices[write_idx]));
        }
    }
}