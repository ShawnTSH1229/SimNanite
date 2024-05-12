struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

cbuffer SNaniteMeshClusterVisualize : register(b0)
{
    float3 VisualizeColor;
};

cbuffer SNanitrGlobalConstants : register(b1)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;
};

struct InstanceData
{
    float4x4 WorldMatrix;
    float3x3 WorldIT;
    float3 padding0;
};
StructuredBuffer<InstanceData> gInstanceData : register(t0);

VSOutput vs_main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData  instance_data =  gInstanceData[instanceID];
    VSOutput vsOutput;
    float4 position = float4(vsInput.position, 1.0);
    float3 worldPos = mul(instance_data.WorldMatrix, position).xyz;
    vsOutput.position = mul(ViewProjMatrix, float4(worldPos, 1.0));
    return vsOutput;
}

float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    return float4(VisualizeColor.xyz, 1.0);
}