#include "CullingCommon.hlsl"
cbuffer SoftwareIndirectCmdGen :register(b0)
{
    uint2 rasterize_parameters_gpu_address; //t2
}

StructuredBuffer<SQueuePassState> queue_pass_state: register(t0);
StructuredBuffer<SHardwareRasterizationIndirectCommand> software_indirect_draw_prarameters: register(t1);

AppendStructuredBuffer<SSoftwareRasterizationIndirectCommand> software_indirect_draw_commands: register(u0);

[numthreads(1, 1, 1)]
void IndirectCmdGen(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    SSoftwareRasterizationIndirectCommand software_indirect_draw_command;
    software_indirect_draw_command.mesh_constant_gpu_address = software_indirect_draw_prarameters[0].mesh_constant_gpu_address;
    software_indirect_draw_command.global_constant_gpu_address = software_indirect_draw_prarameters[0].global_constant_gpu_address;
    software_indirect_draw_command.instance_data_gpu_address = software_indirect_draw_prarameters[0].instance_data_gpu_address;
    software_indirect_draw_command.pos_vertex_buffer_gpu_address = software_indirect_draw_prarameters[0].pos_vertex_buffer_gpu_address;
    software_indirect_draw_command.index_buffer_gpu_address = software_indirect_draw_prarameters[0].index_buffer_gpu_address;
    software_indirect_draw_command.rasterize_parameters_gpu_address = rasterize_parameters_gpu_address;
    software_indirect_draw_command.thread_group_x = queue_pass_state[0].global_dispatch_indirect_size;
    software_indirect_draw_command.thread_group_y = 1;
    software_indirect_draw_command.thread_group_z = 1;
    software_indirect_draw_commands.Append(software_indirect_draw_command);
}