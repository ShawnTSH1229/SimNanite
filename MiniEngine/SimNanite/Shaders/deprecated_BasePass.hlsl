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

// vertex buffer
ByteAddressBuffer vertex_pos_buffer : register(t0);
ByteAddressBuffer vertex_norm_buffer : register(t1);
ByteAddressBuffer vertex_uv_buffer : register(t2);

// index buffer
ByteAddressBuffer index_buffer : register(t3);

// instance data and cluster data
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t4);
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t5);//cluster

// texture data
Texture2D<float4> baseColorTexture          : register(t6);
SamplerState baseColorSampler               : register(s0);

// vis buffer and depth buffer
Texture2D<uint> in_vis_buffer: register(t7);
Texture2D<uint> in_depth_buffer: register(t8);


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
}

void ps_main(in float2 tex:TexCoord0): SV_Target0
{
    uint2 load_pos = tex.xy * rendertarget_size;

    uint depth_value = in_depth_buffer.load(load_pos);
    uint packed_vis_index = in_vis_buffer.load(load_pos);
    
    uint in_tri_index = packed_vis_index & 0x000000FFu;
    uint in_clu_index = (packed_vis_index >> 8) & 0x00000FFFu;
    uint in_ins_index = (packed_vis_index >> 20) & 0x00000FFFu;

    SSimNaniteCluster scene_cluster_info = scene_cluster_infos[cluster_index];

    uint IndexCountPerInstance = scene_cluster_info.index_count;
    uint StartIndexLocation = scene_cluster_info.start_index_location;
    int BaseVertexLocation = scene_cluster_info.start_vertex_location;

    uint indices = index_buffer.Load3(StartIndexLocation + tri_index * 3);

    float3 position_a = asfloat(vertex_pos_buffer.Load3(indices.x));
    float3 position_b = asfloat(vertex_pos_buffer.Load3(indices.y));
    float3 position_c = asfloat(vertex_pos_buffer.Load3(indices.z));

    InstanceData  instance_data =  culled_instance_scene_data[in_ins_index];
    
    float4 worldPos_a = mul(instance_data.WorldMatrix, float4(position_a,1.0)).xyzw;
    float4 worldPos_b = mul(instance_data.WorldMatrix, float4(position_b,1.0)).xyzw;
    float4 worldPos_c = mul(instance_data.WorldMatrix, float4(position_c,1.0)).xyzw;

    float4 clip_pos_a = mul(ViewProjMatrix, worldPos_a);
    float4 clip_pos_b = mul(ViewProjMatrix, worldPos_b);
    float4 clip_pos_c = mul(ViewProjMatrix, worldPos_c);

    float3 ndc_pos_a = float3(clip_pos_a.xyz / clip_pos_a.w);
    float3 ndc_pos_b = float3(clip_pos_b.xyz / clip_pos_b.w);
    float3 ndc_pos_c = float3(clip_pos_c.xyz / clip_pos_c.w);

    float3 barycentric = BarycentricCoordinate(float2(load_pos.x,load_pos.y),screen_pos_a.xy,screen_pos_b.xy,screen_pos_c.xy);
    float z = 1 / (barycentric.x / position_a.w + barycentric.y / position_b.w + barycentric.z / position_c.w);
    barycentric = barycentric / float3(position_a.w, position_b.w, position_c.w) * z;

    float3 vertex_norm_a = asfloat(vertex_norm_buffer.Load3(indices.x));
    float3 vertex_norm_b = asfloat(vertex_norm_buffer.Load3(indices.y));
    float3 vertex_norm_c = asfloat(vertex_norm_buffer.Load3(indices.z));

    float3 vertex_uv_a = asfloat(vertex_uv_buffer.Load3(indices.x));
    float3 vertex_uv_b = asfloat(vertex_uv_buffer.Load3(indices.y));
    float3 vertex_uv_c = asfloat(vertex_uv_buffer.Load3(indices.z));

    float pix_normal = vertex_norm_a * barycentric.x + vertex_norm_b * barycentric.y + vertex_norm_c * barycentric.z;
    float pix_uv = vertex_uv_a * barycentric.x + vertex_uv_b * barycentric.y + vertex_uv_c * barycentric.z;

    float4 baseColor = baseColorTexture.Sample(baseColorSampler, pix_uv);

    return float4(baseColor.xyz,1.0);
}

//cbuffer CBasePassParam:register(b0)
//{
//    uint material_index;
//    float2 render_tagrte_size;
//}


Texture2D<uint> in_mat_id_buffer: register(t0);

void depth_resolve_vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position,
)
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0.0, 1);
}

void depth_resolve_ps_main(in float2 Tex: TexCoord0) : SV_DEPTH
{
    uint2 load_pos = tex.xy * rendertarget_size;
    uint depth_value = in_mat_id_buffer.load(load_pos);
    return asfloat(depth_value);
}



