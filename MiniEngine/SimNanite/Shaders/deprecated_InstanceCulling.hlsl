#include "CullingCommon.hlsl"

cbuffer CullingParameters : register(b0)
{
    float4 planes[6];
    uint total_instance_num;
    //padding
}

StructuredBuffer<SNaniteInstanceSceneData> input_instance_scene_data: register(t0);

RWStructuredBuffer<SNaniteInstanceSceneData> out_instance_scene_data: register(u0);
RWByteAddressBuffer culled_instance_num : register(u1);
RWStructuredBuffer<SQueuePassState> queue_pass_state_to_init : register(u2);

[numthreads(64, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * 64) + groupIndex;

    if (index < total_instance_num)
    {
        bool InFrustum = true;
        for(int i=0; i<6; i++)
        {
            InFrustum = InFrustum && (DistanceFromPoint(planes[i],input_instance_scene_data[index].bounding_center) + input_instance_scene_data[index].bounding_radius > 0.0f);
        }

        if(InFrustum == true)
        {
            uint store_index;
            culled_instance_num.InterlockedAdd(0, 1, store_index);
            out_instance_scene_data[store_index] = input_instance_scene_data[index];
        }
    }

    if(index == 0)
    {
        SQueuePassState queuePassState;
        queuePassState.group_task_offset = 0;
        queuePassState.cluster_task_offset = 0;

        queuePassState.group_task_write_offset = 0;
        queuePassState.cluster_task_write_offset = 0;

        queuePassState.clu_group_num = 0;
        queuePassState.init_clu_group_num = 0;
        queuePassState.global_dispatch_indirect_size = 0;
        
        queue_pass_state_to_init[0] = queuePassState;
    }
}