#include "SimNaniteCommon.hlsl"
#define GROUP_SIZEX 32
#define GROUP_SIZEY 32

cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;

    float2 rendertarget_size;
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

bool IsBackFace(float3 v0, float3 v1, float3 v2)
{
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 normal = cross(e1, e2);
    return normal.z > 0;
}

StructuredBuffer<SSimNaniteClusterDraw> scene_instance_cluster: register(t0);
ByteAddressBuffer global_vertex_pos_buffer : register(t1);
ByteAddressBuffer global_index_buffer : register(t2);

RWTexture2D<uint> intermediate_depth_buffer: register(u0);
RWTexture2D<uint> out_vis_buffer: register(u1);
RWTexture2D<uint> out_mat_id_buffer: register(u2);
RWTexture2D<uint> visualize_softrasterization: register(u3); //debug buffer

// cluster instance num * 1 * 1
[numthreads(GROUP_SIZEX, GROUP_SIZEY, 1)]
void SoftwareRasterization(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint instance_id = groupId.x + SIMNANITE_SOFTWARE_OFFSET;
    uint triangle_index = groupIndex;

    SSimNaniteClusterDraw cluster_draw = scene_instance_cluster[instance_id];
    
    uint index_count = cluster_draw.index_count;
    uint start_index_location = cluster_draw.start_index_location;
    uint start_vertex_location = cluster_draw.start_vertex_location;    

    if(triangle_index * 3 < index_count)
    {
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

        //https://zhuanlan.zhihu.com/p/657809816
        //https://www.cnblogs.com/straywriter/articles/15889297.html

        float4 screen_pos_a = float4((ndc_pos_a.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_a.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_a.z, clip_pos_a.w); 
        float4 screen_pos_b = float4((ndc_pos_b.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_b.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_b.z, clip_pos_b.w); 
        float4 screen_pos_c = float4((ndc_pos_c.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_c.y + 1.0f) * 0.5f * rendertarget_size.y, ndc_pos_c.z, clip_pos_c.w); 

        if(IsBackFace(ndc_pos_a, ndc_pos_b, ndc_pos_c))
        {
            return;
        }

        int2 bbox_min = (int2)min(screen_pos_a.xy,  min(screen_pos_b.xy, screen_pos_c.xy));
        int2 bbox_max = (int2)max(screen_pos_a.xy,  max(screen_pos_b.xy, screen_pos_c.xy));

        if(bbox_max.x > 0 && bbox_max.y > 0 && bbox_min.x < rendertarget_size.x && bbox_min.y < rendertarget_size.y)
        {
            for (int y = bbox_min.y; y <= bbox_max.y; y++)
            {
                for (int x = bbox_min.x; x <= bbox_max.x; x++)
                {
                    float3 barycentric = BarycentricCoordinate(float2(x, y), screen_pos_a.xy, screen_pos_b.xy, screen_pos_c.xy);
                    if(barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0)
                    {
                        float depth = barycentric.x * screen_pos_a.z + barycentric.y * screen_pos_b.z + barycentric.z * screen_pos_c.z;

                        uint depth_uint = depth * 0x7FFFFFFFu;
                        uint2 pixel_pos = uint2(x, rendertarget_size.y - y);
                        uint pre_depth;
                        InterlockedMax(intermediate_depth_buffer[pixel_pos], depth_uint, pre_depth);
                        if (depth_uint > pre_depth)
                        {
                            uint triangle_id = triangle_index;
                            uint visibility_value = triangle_id & 0x0000FFFFu;
                            visibility_value = visibility_value | uint(instance_id << 16u);

                            out_vis_buffer[pixel_pos] = visibility_value;
                            out_mat_id_buffer[pixel_pos] = cluster_draw.material_idx;
                            visualize_softrasterization[pixel_pos] = 1;
                        }
                    }
                }
            }
        }
    }
}

Texture2D<float> src_depth_buffer: register(t0);

void intermediate_depth_buffer_gen_vs(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position)
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0.0, 1);
}

uint intermediate_depth_buffer_gen_ps(in float2 Tex: TexCoord0) : SV_Target0
{
    uint2 load_pos = Tex.xy * rendertarget_size;
    float depth_value = src_depth_buffer.Load(int3(load_pos.xy,0));
    uint depth_uint =  depth_value * 0x7FFFFFFFu;
    return depth_uint;
}