#include "SimNaniteCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    uint instance_id : TEXCOORD0;
};

cbuffer SNaniteGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;
};

StructuredBuffer<SSimNaniteClusterDraw> vis_instance_cluster: register(t0);
ByteAddressBuffer global_vertex_pos_buffer : register(t1);
ByteAddressBuffer global_index_buffer : register(t2);

VSOutput vs_main(uint vertex_id: SV_VertexID, uint instance_id : SV_InstanceID)
{
    SSimNaniteClusterDraw cluster_draw = vis_instance_cluster[instance_id];

    uint index_count = cluster_draw.index_count;
    uint start_index_location = cluster_draw.start_index_location;
    uint start_vertex_location = cluster_draw.start_vertex_location;

    VSOutput vsOutput;
    vsOutput.position = float4(1e10f, 1e10f, 1, 1);
    vsOutput.instance_id = 0;

    if(vertex_id < index_count)
    {
        uint index_read_pos = start_index_location + vertex_id;

        uint vertex_index_idx = global_index_buffer.Load(index_read_pos * 4/*sizeof int*/);
        uint vertex_idx = vertex_index_idx + start_vertex_location;

        float3 vertex_position = asfloat(global_vertex_pos_buffer.Load3((vertex_idx * 3)* 4/*sizeof int*/));

        float4 position = float4(vertex_position, 1.0);
        float3 worldPos = mul(cluster_draw.world_matrix, position).xyz;
        vsOutput.position = mul(ViewProjMatrix, float4(worldPos, 1.0));
        vsOutput.instance_id = instance_id;
    }

    return vsOutput;
}


float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    return float4(float(vsOutput.instance_id / 64) / 20.0, float(vsOutput.instance_id % 64) / 64.0, 0, 1.0);
}

struct SSimNaniteMeshLastLOD
{
    uint cluster_group_num;
    uint cluster_group_start_index;
};

ByteAddressBuffer culled_instance_num : register(t0);
StructuredBuffer<SSimNaniteMeshLastLOD> mesh_last_lod : register(t1);
StructuredBuffer<SSimNaniteClusterGroup> scene_cluster_group_infos : register(t2);
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos : register(t3);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t4);

RWStructuredBuffer<SSimNaniteClusterDraw> vis_instance_culled_cluster: register(u0);
RWByteAddressBuffer vis_instance_cmd_num : register(u1); //todo: clear

// N * 1 * 1, dispatch size
[numthreads(8, 8, 1)]
void GenerateInstaceCullVisCmd(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 dipatch_thread_id : SV_DispatchThreadID)
{
    uint instance_idx = (groupId.x * 8 * 8) + groupIndex;
    uint total_ins_num = culled_instance_num.Load(0);
    if(instance_idx < total_ins_num)
    {
        SNaniteInstanceSceneData instance_data = culled_instance_scene_data[instance_idx];
        uint mesh_index = instance_data.nanite_res_id;

        SSimNaniteMeshLastLOD mesh_lod = mesh_last_lod[mesh_index];
        uint cluster_group_num = mesh_lod.cluster_group_num;
        uint cluster_group_start_index = mesh_lod.cluster_group_start_index;

        uint clu_total_num = 0;

        for(uint clu_group_idx_count = cluster_group_start_index; clu_group_idx_count < (cluster_group_start_index + cluster_group_num); clu_group_idx_count++)
        {
            SSimNaniteClusterGroup clu_group = scene_cluster_group_infos[clu_group_idx_count];

            clu_total_num += clu_group.cluster_num;
        }

        uint write_index = 0;
        vis_instance_cmd_num.InterlockedAdd(0, clu_total_num, write_index);

        for(uint clu_group_idx = cluster_group_start_index; clu_group_idx < (cluster_group_start_index + cluster_group_num); clu_group_idx++)
        {
            SSimNaniteClusterGroup clu_group = scene_cluster_group_infos[clu_group_idx];

            uint clu_num = clu_group.cluster_num;
            uint clu_start_idx = clu_group.cluster_start_index;

            for(uint clu_idx = clu_start_idx; clu_idx < (clu_start_idx + clu_num); clu_idx++)
            {
                SSimNaniteCluster cluster_info = scene_cluster_infos[clu_idx];

                SSimNaniteClusterDraw cluster_draw = (SSimNaniteClusterDraw)0;
                cluster_draw.world_matrix = instance_data.world_matrix;
                cluster_draw.index_count = cluster_info.index_count;
                cluster_draw.start_index_location = cluster_info.start_index_location;
                cluster_draw.start_vertex_location = cluster_info.start_vertex_location;

                vis_instance_culled_cluster[write_index] = cluster_draw;
                write_index++;
            }
        }
    }
}

ByteAddressBuffer vis_instance_cmd_num_srv : register(t0);
RWStructuredBuffer<SIndirectDrawParameters> indirect_draw_cmd: register(u0);
[numthreads(1, 1, 1)]
void GenerateInstaceCullVisIndirectCmd(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 dipatch_thread_id : SV_DispatchThreadID)
{
    SIndirectDrawParameters indirect_draw_parameters;
    indirect_draw_parameters.vertex_count = 1701;
    indirect_draw_parameters.instance_count = vis_instance_cmd_num_srv.Load(0);
    indirect_draw_parameters.start_vertex_location = 0;
    indirect_draw_parameters.start_instance_location = 0;
    indirect_draw_cmd[0] = indirect_draw_parameters;
}
