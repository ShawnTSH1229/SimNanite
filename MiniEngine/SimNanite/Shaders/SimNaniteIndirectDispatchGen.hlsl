#include "SimNaniteCommon.hlsl"
StructuredBuffer<SQueuePassState> queue_pass_state: register(t0); // cleared in instance cull pass
RWStructuredBuffer<SPersistentCullIndirectCmd> cull_indirect_dispatch_cmd: register(u0);

[numthreads(1, 1, 1)]
void GeneratePersistentCullIndirectCmd(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    SPersistentCullIndirectCmd indirect_dispatch_cmd;
    indirect_dispatch_cmd.thread_group_count_x = (queue_pass_state[0].node_num + 31u) / 32u;
    indirect_dispatch_cmd.thread_group_count_y = 1;
    indirect_dispatch_cmd.thread_group_count_z = 1;
    cull_indirect_dispatch_cmd[0] = indirect_dispatch_cmd;
}