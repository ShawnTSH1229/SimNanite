#include "SimNaniteCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    uint visibility_value : TEXCOORD0;
    uint material_idx : TEXCOORD1;
};

cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;
};

StructuredBuffer<SSimNaniteClusterDraw> hardware_instance_cluster: register(t0);
ByteAddressBuffer global_vertex_pos_buffer : register(t1);
ByteAddressBuffer global_index_buffer : register(t2);

VSOutput vs_main(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
{
    SSimNaniteClusterDraw cluster_draw = hardware_instance_cluster[instance_id];
    
    uint index_count = cluster_draw.index_count;
    uint start_index_location = cluster_draw.start_index_location;
    uint start_vertex_location = cluster_draw.start_vertex_location;

    VSOutput vsOutput;
    vsOutput.position = float4(1e10f, 1e10f, 1, 1);
    vsOutput.material_idx = 0;

    if(vertex_id < index_count)
    {
        uint index_read_pos = start_index_location + vertex_id;

        uint vertex_index_idx = global_index_buffer.Load(index_read_pos * 4/*sizeof int*/);
        uint vertex_idx = vertex_index_idx + start_vertex_location;

        float3 vertex_position = asfloat(global_vertex_pos_buffer.Load3((vertex_idx * 3)* 4/*sizeof int*/));

        uint triangle_id = vertex_id / 3;
        uint visibility_value = triangle_id & 0x0000FFFFu;
        visibility_value = visibility_value | uint(instance_id << 16u);

        float4 position = float4(vertex_position, 1.0);
        float3 worldPos = mul(cluster_draw.world_matrix, position).xyz;
        vsOutput.position = mul(ViewProjMatrix, float4(worldPos, 1.0));
        vsOutput.visibility_value = visibility_value;
        vsOutput.material_idx = cluster_draw.material_idx;
    }

    return vsOutput;
}

struct PSOutput
{
    uint visibility : SV_Target0;
    uint material_idx : SV_Target1;
};

PSOutput ps_main(VSOutput vsOutput)
{
    PSOutput psOutput;
    psOutput.visibility = vsOutput.visibility_value;
    psOutput.material_idx = vsOutput.material_idx;
    return psOutput;
}

