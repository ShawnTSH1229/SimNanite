#define GROUP_SIZE 128

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

//todo: merge all scene info indirect buffer together
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t0);
ByteAddressBuffer vertex_pos_buffer : register(t1);
ByteAddressBuffer index_pos_buffer : register(t2);
StructuredBuffer<SSoftwareRasterizationParameters> software_indirect_draw_command: register(t3);
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t4);//cluster

RWTexture2D<uint> out_depth_buffer: register(u0);
RWTexture2D<uint> out_vis_buffer: register(u1);
RWTexture2D<uint> out_mat_id_buffer: register(u2);

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

// one thread per triangle
[numthreads(GROUP_SIZE, 1, 1)]
void SoftwareRasterization(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * GROUP_SIZE) + groupIndex;

    // get indirect draw infomation
    uint index_count_per_instance = software_indirect_draw_command[index].IndexCountPerInstance;
    uint instance_count = software_indirect_draw_command[index].InstanceCount;
    uint start_index_location = software_indirect_draw_command[index].StartIndexLocation;
    uint base_vertex_location = software_indirect_draw_command[index].BaseVertexLocation;
    uint start_instance_location = software_indirect_draw_command[index].StartInstanceLocation;

    // the index of current triangle
    uint start_index_this_triangle = start_index_location + index;

    // 
    uint3 indices = index_pos_buffer.Load3(start_index_this_triangle / 3);

    uint instance_id = start_instance_location;
    uint culled_instance_index = instance_id & 0x0000FFFF;
    uint cluster_idx = (instance_id >> 16) & 0x0000FFFF;

    float3 vertex_pos_a = asfloat(vertex_pos_buffer.Load3(base_vertex_location + indices.x));
    float3 vertex_pos_b = asfloat(vertex_pos_buffer.Load3(base_vertex_location + indices.y));
    float3 vertex_pos_c = asfloat(vertex_pos_buffer.Load3(base_vertex_location + indices.z));

    InstanceData  instance_data =  culled_instance_scene_data[culled_instance_index];

    float4 position_a = float4(vertex_pos_a, 1.0);
    float4 position_b = float4(vertex_pos_b, 1.0);
    float4 position_c = float4(vertex_pos_c, 1.0);

    float4 worldPos_a = mul(instance_data.WorldMatrix, position_a).xyzw;
    float4 worldPos_b = mul(instance_data.WorldMatrix, position_b).xyzw;
    float4 worldPos_c = mul(instance_data.WorldMatrix, position_c).xyzw;

    float4 clip_pos_a = mul(ViewProjMatrix, worldPos_a);
    float4 clip_pos_b = mul(ViewProjMatrix, worldPos_b);
    float4 clip_pos_c = mul(ViewProjMatrix, worldPos_c);

    float3 ndc_pos_a = float3(clip_pos_a.xyz / clip_pos_a.w);
    float3 ndc_pos_b = float3(clip_pos_b.xyz / clip_pos_b.w);
    float3 ndc_pos_c = float3(clip_pos_c.xyz / clip_pos_c.w);

    //https://zhuanlan.zhihu.com/p/657809816
    //https://www.cnblogs.com/straywriter/articles/15889297.html

    float4 screen_pos_a = float4((ndc_pos_a.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_a.y + 1.0f) * 0.5f * rendertarget_size.y,ndc_pos_a.z,clip_pos_a.w); 
    float4 screen_pos_b = float4((ndc_pos_b.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_b.y + 1.0f) * 0.5f * rendertarget_size.y,ndc_pos_b.z,clip_pos_b.w); 
    float4 screen_pos_c = float4((ndc_pos_c.x + 1.0f) * 0.5f * rendertarget_size.x, (ndc_pos_c.y + 1.0f) * 0.5f * rendertarget_size.y,ndc_pos_c.z,clip_pos_c.w); 

    if(IsBackFace(ndc_pos_a,ndc_pos_b,ndc_pos_c))
    {
        return;
    }

    int2 bbox_min = (int2)min(screen_pos_a.xy,min(screen_pos_b.xy, screen_pos_c.xy));
    int2 bbox_max = (int2)max(screen_pos_a.xy,max(screen_pos_b.xy, screen_pos_c.xy));

    for (int y = bbox_min.y; y <= bbox_max.y; y++)
    {
        for (int x = bbox_min.x; x <= bbox_max.x; x++)
        {
            float3 barycentric = BarycentricCoordinate(float2(x,y),screen_pos_a.xy,screen_pos_b.xy,screen_pos_c.xy);
            if(barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0)
            {
                float depth = barycentric.x * screen_pos_a.z + barycentric.y * screen_pos_b.z + barycentric.z * screen_pos_c.z;

                uint depth_int = depth; //asuint ?
                uint2 pixel_pos = uint2(x,y);
                uint pre_depth;
                InterlockedMax(out_depth_buffer[pixel_pos], depth_int, pre_depth);
                if (depth_int > pre_depth) //todo:!
	            {
                    uint out_tri_index = start_index_this_triangle / 3;
                    uint out_clu_index = ( cluster_idx  << 8);
                    uint out_ins_index = ( culled_instance_index << 20 );

                    uint mat_index = scene_cluster_infos[out_clu_index].mesh_index;

                    uint out_vis_result = out_ins_index | out_clu_index | tri_index;
	            	OutBuffer[pixel_pos] = out_vis_result;
                    out_mat_id_buffer[pixel_pos] = mat_index;
	            }
            }
        }
    }
}



