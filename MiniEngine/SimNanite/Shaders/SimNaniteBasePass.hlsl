// 0: MAT_TYPE_UNLIT
// 1: MAT_TYPE_SIMPLE_PBR

#define MAT_TYPE 0
cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
};

cbuffer CBasePassParam:register(b1)
{
    uint material_index;
}

void vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position,
)
{
    float depth = asfloat(material_index);

    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), depth, 1);
};

void ps_main(in float2 tex:TexCoord0): SV_Target0
{
    
}