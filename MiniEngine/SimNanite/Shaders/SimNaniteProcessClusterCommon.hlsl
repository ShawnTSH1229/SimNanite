void ProcessCluster(uint instance_index,uint clu_idx)
{
    SSimNaniteCluster scene_clu = scene_cluster_infos[clu_idx];

    const float4x4 world_matrix = culled_instance_scene_data[instance_index].world_matrix;
    float3 bounding_center = mul(world_matrix,float4(scene_clu.bound_sphere_center, 1.0)).xyz;

    float3 scale = float3(world_matrix[0][0], world_matrix[1][1], world_matrix[1][1]);
    float max_scale = max(max(scale.x, scale.y), scale.z);
    float bounding_radius = scene_clu.bound_sphere_radius * max_scale;

    bool is_culled = false;

    // frustum culling
    {
        bool in_frustum = true;
        for(int i = 0; i < 6; i++)
        {
            in_frustum = in_frustum && (DistanceFromPoint(planes[i],bounding_center) + bounding_radius > 0.0f);
        }

        if(in_frustum == false) { is_culled = true; }
    }

    // hzb culling
    { }

    if(!is_culled)
    {
        bool is_software_rasterrization = false;
        float dist = distance(bounding_center, camera_world_pos);
        if(dist > 1000)
        {
            is_software_rasterrization = true;
        }

        if(is_software_rasterrization)
        {
            uint software_write_offset = 0;
            software_indirect_draw_num.InterlockedAdd(0, 1, software_write_offset);
            
            SSimNaniteClusterDraw cluster_draw = (SSimNaniteClusterDraw)0;
            cluster_draw.world_matrix = world_matrix;
            cluster_draw.index_count = scene_clu.index_count;
            cluster_draw.start_index_location = scene_clu.start_index_location;
            cluster_draw.start_vertex_location = scene_clu.start_vertex_location;
            cluster_draw.material_idx = scene_clu.mesh_index + 1;
            scene_indirect_draw_cmd[software_write_offset + SIMNANITE_SOFTWARE_OFFSET] = cluster_draw;
        }
        else
        {
            uint hardware_write_offset = 0;
            hardware_indirect_draw_num.InterlockedAdd(0, 1, hardware_write_offset);

            SSimNaniteClusterDraw cluster_draw = (SSimNaniteClusterDraw)0;
            cluster_draw.world_matrix = world_matrix;
            cluster_draw.index_count = scene_clu.index_count;
            cluster_draw.start_index_location = scene_clu.start_index_location;
            cluster_draw.start_vertex_location = scene_clu.start_vertex_location;
            cluster_draw.material_idx = scene_clu.mesh_index + 1;
            scene_indirect_draw_cmd[hardware_write_offset] = cluster_draw;
        }
    }
};