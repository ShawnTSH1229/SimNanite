#include "SimNaniteCommon.hlsl"

cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
};

cbuffer CBasePassParam: register(b1)
{
    uint material_index;
}

StructuredBuffer<SSimNaniteClusterDraw> scene_instance_cluster: register(t0);
ByteAddressBuffer global_vertex_pos_buffer : register(t1);
ByteAddressBuffer global_vertex_uv_buffer : register(t2);
ByteAddressBuffer global_index_buffer : register(t3);
Texture2D<uint> in_vis_buffer: register(t4);
Texture2D<uint> in_depth_buffer: register(t5);

Texture2D<float4> baseColorTexture          : register(t6);
SamplerState baseColorSampler               : register(s0);

void vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position)
{
    float depth = float(material_index) / 1024.0;

    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), depth, 1);
};

float3 BarycentricCoordinate(float2 p, float2 v0, float2 v1, float2 v2)
{
    float3 v0v1 = float3(v1.xy - v0.xy, 0);
    float3 v1v2 = float3(v2.xy - v1.xy, 0);
    float3 v2v0 = float3(v0.xy - v2.xy, 0);

    float3 v0p = float3(p - v0.xy, 0);
    float3 v1p = float3(p - v1.xy, 0);
    float3 v2p = float3(p - v2.xy, 0);

    float3 normal = cross(v2v0, v0v1);
    float area = abs(normal.z);

    normal = normalize(normal);

    float area0 = cross(v1v2, v1p).z * normal.z;
    float area1 = cross(v2v0, v2p).z * normal.z;
    float area2 = cross(v0v1, v0p).z * normal.z;


    return float3(area0 / area, area1 / area, area2 / area);
}

float4 ps_main(in float2 tex: TexCoord0): SV_Target0
{
    uint2 load_pos = tex * rendertarget_size;
    uint depth_buffer = in_depth_buffer.Load(int3(load_pos.xy,0));
    float4 output_color = float4(0, 0, 0, 0);
    if(depth_buffer != 0)
    {
        uint visibility_value = in_vis_buffer.Load(int3(load_pos.xy,0));
        uint cluster_index = visibility_value >> 16;
        uint triangle_index = visibility_value & 0x0000FFFFu;

        SSimNaniteClusterDraw cluster_draw = scene_instance_cluster[cluster_index];

        uint index_count = cluster_draw.index_count;
        uint start_index_location = cluster_draw.start_index_location;
        uint start_vertex_location = cluster_draw.start_vertex_location;

        uint start_index_this_triangle = start_index_location + triangle_index * 3;
        uint3 indices = global_index_buffer.Load3((start_index_this_triangle / 3) * 3 * 4 /* 3 * sizeof(uint) */);

        float3 vertex_pos_a = asfloat(global_vertex_pos_buffer.Load3((start_vertex_location + indices.x) * 3 * 4 /* 3 * sizeof(float)*/));
        float3 vertex_pos_b = asfloat(global_vertex_pos_buffer.Load3((start_vertex_location + indices.y) * 3 * 4 /* 3 * sizeof(float)*/));
        float3 vertex_pos_c = asfloat(global_vertex_pos_buffer.Load3((start_vertex_location + indices.z) * 3 * 4 /* 3 * sizeof(float)*/));

        float4 position_a = float4(vertex_pos_a, 1.0);
        float4 position_b = float4(vertex_pos_b, 1.0);
        float4 position_c = float4(vertex_pos_c, 1.0);

        float4x4 world_matrix = cluster_draw.world_matrix;
        
        float4 worldPos_a = mul(world_matrix, position_a).xyzw;
        float4 worldPos_b = mul(world_matrix, position_b).xyzw;
        float4 worldPos_c = mul(world_matrix, position_c).xyzw;

        float4 clip_pos_a = mul(ViewProjMatrix, worldPos_a);
        float4 clip_pos_b = mul(ViewProjMatrix, worldPos_b);
        float4 clip_pos_c = mul(ViewProjMatrix, worldPos_c);

        float3 ndc_pos_a = float3(clip_pos_a.xyz / clip_pos_a.w);
        float3 ndc_pos_b = float3(clip_pos_b.xyz / clip_pos_b.w);
        float3 ndc_pos_c = float3(clip_pos_c.xyz / clip_pos_c.w);

        float4 screen_pos_a = float4((ndc_pos_a.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_a.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_a.z, clip_pos_a.w); 
        float4 screen_pos_b = float4((ndc_pos_b.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_b.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_b.z, clip_pos_b.w); 
        float4 screen_pos_c = float4((ndc_pos_c.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_c.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_c.z, clip_pos_c.w); 

        float3 barycentric = BarycentricCoordinate(float2(load_pos.x, rendertarget_size.y - load_pos.y), screen_pos_a.xy, screen_pos_b.xy, screen_pos_c.xy);
        float z = 1 / (barycentric.x / position_a.w + barycentric.y / position_b.w + barycentric.z / position_c.w);
        barycentric = barycentric / float3(position_a.w, position_b.w, position_c.w) * z;

        float2 vertex_uv_a = asfloat(global_vertex_uv_buffer.Load2((start_vertex_location + indices.x) * 2 * 4 /* 2 * sizeof(float)*/));
        float2 vertex_uv_b = asfloat(global_vertex_uv_buffer.Load2((start_vertex_location + indices.y) * 2 * 4 /* 2 * sizeof(float)*/));
        float2 vertex_uv_c = asfloat(global_vertex_uv_buffer.Load2((start_vertex_location + indices.z) * 2 * 4 /* 2 * sizeof(float)*/));

        float2 pix_uv = vertex_uv_a * barycentric.x + vertex_uv_b * barycentric.y + vertex_uv_c * barycentric.z;
        float4 baseColor = baseColorTexture.Sample(baseColorSampler, pix_uv);
        output_color = float4(baseColor.xyz, 1.0);
    }
    return output_color;
}