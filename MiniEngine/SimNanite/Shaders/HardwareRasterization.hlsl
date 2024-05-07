#include "RasterizationCommon.hlsl"

struct VSInput
{
    float3 position : POSITION;
};

cbuffer SNaniteMeshConstants : register(b0)
{
    float4x4 WorldMatrix;   // Object to world
    float3x3 WorldIT;       // Object normal to world normal
};

cbuffer SNanitrGlobalConstants : register(b1)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
};

StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t0);

struct VSOutput
{
    float4 position : SV_POSITION;
    nointerpolation uint packed_cluster_and_tri_index : TEXCOORD1;
};

VSOutput vs_main(VSInput vsInput, uint instanceID : SV_InstanceID, uint ib_id : SV_VertexID)
{
    // cluster index ( 12 bit )
    // tri index ( 8 bit )
    // instance index (12 bit )
    uint culled_instance_index = instanceID & 0x0000FFFF;
    uint cluster_idx = (instanceID >> 16) & 0x0000FFFF;

    VSOutput vsOutput;

    InstanceData  instance_data =  culled_instance_scene_data[culled_instance_index];
    float4 position = float4(vsInput.position, 1.0);
    float3 worldPos = mul(instance_data.WorldMatrix, position).xyz;
    vsOutput.position = mul(ViewProjMatrix, float4(vsOutput.worldPos, 1.0));

    uint out_tri_index = ib_id / 3;
    uint out_clu_index =  (cluster_idx << 8);
    uint out_ins_index =  (culled_instance_index << 20);

    vsOutput.packed_cluster_and_tri_index = out_ins_index | out_clu_index | out_tri_index;
    return vsOutput;
}

struct SPsOutput
{
    uint packed_index : SV_Target0;
    uint mat_index : SV_Target1;
};

StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t1);//cluster

SPsOutput ps_main(VSOutput vsOutput)
{
    SPsOutput ps_out;
    ps_out.packed_index = vsOutput.packed_cluster_and_tri_index;

    uint in_clu_index = (ps_out.packed_index >> 8) & (0x00000FFFu);
    
    uint mat_index = scene_cluster_infos[in_clu_index].mesh_index;
    ps_out.mat_index = mat_index;

    return ps_out;
} 



void depth_resolve_vs_main(
    in uint VertID : SV_VertexID,
    out float4 Pos : SV_Position,
    out float2 Tex : TexCoord0
)
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}

cbuffer SDepthResolveParams : register(b1)
{
    float2 rendertarget_size;
};

Texture2D<uint> hardware_depth_texture : register(t0);

uint depth_resolve_ps_main(in float4 Pos : SV_Position,in float2 Tex : TexCoord0) : SV_DEPTH
{
    int2 dest_pos = rendertarget_size * Tex;
    uint depth = hardware_depth_texture.Load(dest_pos);
    return asfloat(dest_pos);
} 

