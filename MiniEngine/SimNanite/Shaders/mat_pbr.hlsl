struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
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
}

struct InstanceData
{
    float4x4 WorldMatrix;   // Object to world
    float3x3 WorldIT;       // Object normal to world normal
    float3 padding0;
};
StructuredBuffer<InstanceData> gInstanceData : register(t0);

VSOutput vs_main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData  instance_data =  gInstanceData[instanceID];

    VSOutput vsOutput;

    float4 position = float4(vsInput.position, 1.0);
    float3 normal = vsInput.normal;

    vsOutput.worldPos = mul(instance_data.WorldMatrix, position).xyz;
    vsOutput.position = mul(ViewProjMatrix, float4(vsOutput.worldPos, 1.0));
    //vsOutput.normal = mul(WorldIT, normal);
    vsOutput.normal = mul(instance_data.WorldIT, float3(0,1,0));
    vsOutput.uv0 = vsInput.uv0;
    return vsOutput;
}


Texture2D<float4> baseColorTexture          : register(t0);
SamplerState baseColorSampler               : register(s0);


float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    float3 normal = vsOutput.normal;
    float2 tex_uv = vsOutput.uv0;
    tex_uv.y = 1.0 - tex_uv.y;
    float3 viewDir = normalize(CameraPos - vsOutput.worldPos);

    float4 baseColor = baseColorTexture.Sample(baseColorSampler, tex_uv);
    float diff = max(dot(normal, SunDirection), 0.0);
    float3 diffuse = diff * SunIntensity;

    float3 reflectDir = reflect(-SunDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float3 specular = spec * SunIntensity;

    float3 result = (diffuse + specular) * baseColor.xyz;

    return float4(tex_uv,diff, 1.0);
}