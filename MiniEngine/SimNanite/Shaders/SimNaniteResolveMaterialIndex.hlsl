cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
};

Texture2D<uint> mat_id_buffer: register(t0);

void vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position
)
{
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
};

float ps_main(in float2 tex: TexCoord0): SV_Depth
{
    uint2 load_pos = tex * rendertarget_size;
    uint material_index = mat_id_buffer.Load(int3(load_pos.xy, 0));
    float depth = float(material_index) / 1024.0;
    return depth;
}