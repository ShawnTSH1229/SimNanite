Texture2D<uint> vis_buffer : register(t0);
Texture2D<uint> software_raster_buffer : register(t1);

RWTexture2D<float4> dest_buffer : register(u0);

cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
};

[numthreads(8, 8, 1)]
void CopyVisBufferToDestVisualizeRT(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 dipatch_thread_id : SV_DispatchThreadID)
{
    if(dipatch_thread_id.x < rendertarget_size.x && dipatch_thread_id.y < rendertarget_size.y)
    {
        uint vis_value = vis_buffer.Load(int3(dipatch_thread_id.xy, 0)).x;
        uint soft_rast = software_raster_buffer.Load(int3(dipatch_thread_id.xy, 0)).x;
        
        uint clu_idx = vis_value >> 16;
        uint vtx_idx = vis_value & 0x0000FFFFu;

        float4 vis_color = float4(0,0,0,1);
        if(vtx_idx % 5 == 0)
        {
            vis_color.xy = float2(1,1);
        }
        if(vtx_idx % 5 == 1)
        {
            vis_color.xy = float2(0,1);
        }
        if(vtx_idx % 5 == 2)
        {
            vis_color.xy = float2(1,0);
        }
        if(vtx_idx % 5 == 3)
        {
            vis_color.xyz = float3(0,0,1);
        }
        if(vtx_idx % 5 == 4)
        {
            vis_color.xyz = float3(1,0,1);
        }

        if(soft_rast == 1)
        {
            vis_color.y = 1;
        }

        if(vis_value == 0)
        {
            vis_color = float4(0,0,0,1);
        }

        dest_buffer[uint2(dipatch_thread_id.xy)] = vis_color;
    }
}