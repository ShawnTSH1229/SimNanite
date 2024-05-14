#include "SimNaniteCommon.hlsl"
StructuredBuffer<SClusterGroupCullVis> cluster_group_cull_vis: register(t0);
ByteAddressBuffer cluster_group_culled_num: register(t1);
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos : register(t2);
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t3);

RWStructuredBuffer<SSimNaniteClusterDraw> vis_clu_group_culled_cluster: register(u0);
RWByteAddressBuffer vis_cul_group_cmd_num : register(u1); //todo: clear

[numthreads(8, 8, 1)]
void GenerateClusterGroupCullVisCmd(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 dipatch_thread_id : SV_DispatchThreadID)
{
    uint clu_group_idx = (groupId.x * 8 * 8) + groupIndex;
    uint total_clu_group_num = cluster_group_culled_num.Load(0);
    if(clu_group_idx < total_clu_group_num)
    {
        SClusterGroupCullVis cluster_group = cluster_group_cull_vis[clu_group_idx];
        uint instance_idx = cluster_group.culled_instance_index;
        uint cluster_num = cluster_group.cluster_num;
        uint cluster_start_index = cluster_group.cluster_start_index;

        SNaniteInstanceSceneData instance_data = culled_instance_scene_data[instance_idx];

        uint write_index = 0;
        vis_cul_group_cmd_num.InterlockedAdd(0, cluster_num, write_index);

        for(uint clu_idx = cluster_start_index; clu_idx < (cluster_start_index + cluster_num); clu_idx++)
        {
            SSimNaniteCluster cluster_info = scene_cluster_infos[clu_idx];

            SSimNaniteClusterDraw cluster_draw = (SSimNaniteClusterDraw)0;
            cluster_draw.world_matrix = instance_data.world_matrix;
            cluster_draw.index_count = cluster_info.index_count;
            cluster_draw.start_index_location = cluster_info.start_index_location;
            cluster_draw.start_vertex_location = cluster_info.start_vertex_location;

            vis_clu_group_culled_cluster[write_index] = cluster_draw;
            write_index++;
        }
    }
}

