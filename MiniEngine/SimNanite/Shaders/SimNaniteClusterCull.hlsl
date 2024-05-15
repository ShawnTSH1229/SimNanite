#include "SimNaniteCommon.hlsl"

StructuredBuffer<SQueuePassState> queue_pass_state : register(t0);
ByteAddressBuffer cluster_task_queue: register(t1);
ByteAddressBuffer cluster_task_batch_size: register(t2); 
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t3);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t4);

RWStructuredBuffer<SSimNaniteClusterDraw> hardware_indirect_draw_cmd : register(u0); 
RWByteAddressBuffer hardware_indirect_draw_num : register(u1); //todo: clear

#include "SimNaniteProcessClusterCommon.hlsl"

[numthreads(GROUP_SIZE, 1, 1)]
void ClusterCull(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint thread_idx = groupId.x * GROUP_SIZE + groupIndex;
    uint clu_task_start_idx = thread_idx + queue_pass_state[0].cluster_task_read_offset;
    if (clu_task_start_idx < queue_pass_state[0].cluster_task_write_offset)
    {
        const uint2 cluster_task = cluster_task_queue.Load2(clu_task_start_idx * 8 /*sizeof(uint2)*/);
        const uint cluster_task_instance_index = cluster_task.x;
        const uint cluster_task_clu_start_index = cluster_task.y;
        const uint clu_batch_size = cluster_task_batch_size.Load(clu_task_start_index * 4 /*sizeof uint*/);
        for(uint task_idx = 0; task_idx < clu_batch_size; task_idx++)
        {
            ProcessCluster(cluster_task_instance_index, cluster_task_clu_start_index + task_idx);
        }
    }
}