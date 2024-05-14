#include "SimNaniteCommon.hlsl"

ByteAddressBuffer hardware_indirect_draw_num : register(t0);
RWStructuredBuffer<SIndirectDrawParameters>  hardware_draw_indirect: register(u0);

[numthreads(1, 1, 1)]
void HardwareIndirectDrawGen(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    SIndirectDrawParameters indirect_draw_parameters;
    indirect_draw_parameters.vertex_count = 1701;
    indirect_draw_parameters.instance_count = hardware_indirect_draw_num.Load(0);
    indirect_draw_parameters.start_vertex_location = 0;
    indirect_draw_parameters.start_instance_location = 0;
    hardware_draw_indirect[0] = indirect_draw_parameters;
}