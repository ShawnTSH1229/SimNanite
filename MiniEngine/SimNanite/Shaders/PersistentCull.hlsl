#include "CullingCommon.hlsl"

struct SGroupTask
{
    uint instance_index;
    uint clu_group_index;
};

struct SClusterTask
{
    uint clu_start_index;
    uint instance_index;
};

struct SClusterTaskBachSize
{
    int clu_num;
};

cbuffer PersistentCullParameter : register(b0)
{
    float4 planes[6];
    float3 camera_world_pos;
    uint2 global_constant_gpu_address;
}

ByteAddressBuffer cluster_group_init_task_queue: register(t0);
StructuredBuffer<SSimNaniteCluster> scene_cluster_infos: register(t1);//cluster
StructuredBuffer<SSimNaniteClusterGroup> scene_cluster_group_infos: register(t2);//cluster group
StructuredBuffer<SSimNaniteMesh> scene_mesh_infos: register(t3);//mesh
StructuredBuffer<SNaniteInstanceSceneData> culled_instance_scene_data: register(t4);//instance

RWByteAddressBuffer cluster_group_task_queue: register(u0); // todo: we should add a init/clear pass
RWByteAddressBuffer cluster_task_queue: register(u1); 
RWByteAddressBuffer cluster_task_batch_size: register(u2); 
RWStructuredBuffer<SQueuePassState> queue_pass_state: register(u3);//todo: we should add a clear pass ,and set the clu_group_num to init size

AppendStructuredBuffer<SHardwareRasterizationIndirectCommand> hardware_indirect_draw_command: register(u4);
AppendStructuredBuffer<SHardwareRasterizationIndirectCommand> software_indirect_draw_prarameters: register(u5);



#define GROUP_SIZE 32
#define GROUP_PROCESS_GROUP_NUM GROUP_SIZE
#define GROUP_PROCESS_CLUSTER_NUM GROUP_SIZE

groupshared uint group_start_cul_group_idx;
groupshared uint group_clu_group_processed_mask;

groupshared uint group_start_clu_index;
groupshared uint group_clu_task_size;

groupshared int group_task_num;

void ProcessClusterBatch(uint instance_index,uint clu_idx)
{
    const float4x4 world_matrix = culled_instance_scene_data[instance_index].worlt_mat;
    float3 bounding_center = mul(world_matrix,float4(scene_cluster_infos[clu_idx].bound_sphere_center,1.0)).xyz;

    float3 scale = float3(world_matrix[0][0],world_matrix[1][1],world_matrix[1][1]);
    float max_scale = max(max(scale.x,scale.y),scale.z);
    float bounding_radius = scene_cluster_infos[clu_idx].bound_sphere_radius * max_scale;

    bool is_culled = false;
    // frustum culling
    {
        bool in_frustum = true;
        for(int i = 0; i < 6; i++)
        {
            in_frustum = in_frustum && (DistanceFromPoint(planes[i],bounding_center) + bounding_radius > 0.0f);
        }

        if(in_frustum == false)
        {
           is_culled = true;
        }
    }

    // hzb culling
    {

    }

    if(is_culled)
    {
        return;
    }

    uint mesh_idx = scene_cluster_infos[clu_idx].mesh_index;
    uint2 mesh_constant_gpu_address = scene_mesh_infos[mesh_idx].mesh_constant_gpu_address;
    //uint2 global_constant_gpu_address = scene_mesh_infos[mesh_idx].global_constant_gpu_address;
    uint2 instance_data_gpu_address = scene_mesh_infos[mesh_idx].instance_data_gpu_address;
    uint2 pos_vertex_buffer_gpu_address = scene_mesh_infos[mesh_idx].pos_vertex_buffer_gpu_address;
    uint2 index_buffer_gpu_address = scene_mesh_infos[mesh_idx].index_buffer_gpu_address;

    bool is_software_rasterrization = false;
    float dist = distance(bounding_center,camera_world_pos);
    if(dist > 200 && mesh_idx == 1) // todo: we should use a global scene buffer
    {
        //debug
        //is_software_rasterrization = true;
    }

    uint IndexCountPerInstance = scene_cluster_infos[clu_idx].index_count;
    uint InstanceCount = 1;
    
    // | 16bit cluster index | 16 bit instance index |
    uint StartIndexLocation = scene_cluster_infos[clu_idx].start_index_location;
    StartIndexLocation |= (clu_idx << 16);
    
    int BaseVertexLocation = scene_cluster_infos[clu_idx].start_vertex_location;
    uint StartInstanceLocation = instance_index;

    SHardwareRasterizationIndirectCommand hareware_indirect_draw_cmd = (SHardwareRasterizationIndirectCommand)0;
    hareware_indirect_draw_cmd.mesh_constant_gpu_address = mesh_constant_gpu_address;
    hareware_indirect_draw_cmd.global_constant_gpu_address = global_constant_gpu_address;
    hareware_indirect_draw_cmd.instance_data_gpu_address = instance_data_gpu_address;

    hareware_indirect_draw_cmd.pos_vertex_buffer_gpu_address = pos_vertex_buffer_gpu_address;
    hareware_indirect_draw_cmd.index_buffer_gpu_address = index_buffer_gpu_address;

    hareware_indirect_draw_cmd.InstanceCount = InstanceCount;
    hareware_indirect_draw_cmd.StartIndexLocation = StartIndexLocation;
    hareware_indirect_draw_cmd.BaseVertexLocation = BaseVertexLocation;
    hareware_indirect_draw_cmd.StartInstanceLocation = StartInstanceLocation;

    if(is_software_rasterrization)
    {
        InterlockedAdd(queue_pass_state[0].global_dispatch_indirect_size, 1);
        software_indirect_draw_prarameters.Append(hareware_indirect_draw_cmd);
    }
    else
    {
        hardware_indirect_draw_command.Append(hareware_indirect_draw_cmd);
    }
}

void ProcessClusterGroup(uint instance_index,uint clu_group_index)
{
    // get instance infomation
    const float4x4 world_matrix = culled_instance_scene_data[instance_index].worlt_mat;
    
    float3 bounding_center = mul(world_matrix,float4(scene_cluster_group_infos[clu_group_index].bound_sphere_center,1.0)).xyz;

    float3 scale = float3(world_matrix[0][0],world_matrix[1][1],world_matrix[1][1]);
    float max_scale = max(max(scale.x,scale.y),scale.z);
    float bounding_radius = scene_cluster_group_infos[clu_group_index].bound_sphere_radius * max_scale;

    bool is_culled = false;
    bool is_use_next_lod = false;
    
    // frustum culling
    {
        bool in_frustum = true;
        for(int i = 0; i < 6; i++)
        {
            in_frustum = in_frustum && (DistanceFromPoint(planes[i],bounding_center) + bounding_radius > 0.0f);
        }

        if(in_frustum == false)
        {
           is_culled = true;
        }
    }

    // hzb culling
    {

    }

    // compute the lod
    if(!is_culled)
    {
        float dist = distance(bounding_center,camera_world_pos);
        if(dist > scene_cluster_group_infos[clu_group_index].cluster_next_lod_dist)
        {
            is_use_next_lod = true;
        }
    }

    int group_task_change_num = -1;

    if(is_use_next_lod)
    {
        // add cluster group
        uint child_group_num = scene_cluster_group_infos[clu_group_index].child_group_num;
        uint write_offset;

        //todo: wave vote optimization
        InterlockedAdd(queue_pass_state[0].group_task_write_offset, child_group_num, write_offset);

        for(uint idx = 0; idx < child_group_num; idx++)
        {
            uint child_group_idx = scene_cluster_group_infos[clu_group_index].child_group_index[idx];
            uint2 write_group_task = uint2(instance_index,child_group_idx);
            cluster_group_task_queue.Store2(write_offset + idx,write_group_task);
        }

        group_task_change_num += child_group_num;
    }
    else
    {
        // add cluster
        uint child_cluster_num = scene_cluster_group_infos[clu_group_index].child_group_num;
        uint cluster_batch_size = (child_cluster_num + GROUP_SIZE - 1) / GROUP_SIZE;
        
        uint write_offset;
        InterlockedAdd(queue_pass_state[0].cluster_task_write_offset, cluster_batch_size, write_offset);

        for(uint idx = 0; idx < cluster_batch_size - 1; idx +1)
        {
            uint clu_start_idx = scene_cluster_group_infos[clu_group_index].cluster_start_index + idx * GROUP_SIZE;
            uint2 write_cluster_task = uint2(clu_start_idx,instance_index);

            cluster_task_batch_size.Store(write_offset + idx,GROUP_SIZE);
            cluster_task_queue.Store2(write_offset + idx,write_cluster_task);
        }

        {
            int idx = cluster_batch_size - 1;
            int cluster_num = child_cluster_num - (cluster_batch_size -1)* GROUP_SIZE;

            uint clu_start_idx = scene_cluster_group_infos[clu_group_index].cluster_start_index + idx * GROUP_SIZE;
            uint2 write_cluster_task = uint2(clu_start_idx,instance_index);

            cluster_task_batch_size.Store(write_offset + idx, cluster_num);
            cluster_task_queue.Store2(write_offset + idx,write_cluster_task);            
        }
    }

    InterlockedAdd(queue_pass_state[0].clu_group_num, group_task_change_num);
}


[numthreads(GROUP_SIZE, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint init_clu_group_num = queue_pass_state[0].init_clu_group_num;

    uint group_index = groupIndex;

    // cluster group
    bool has_clu_group_task = true;
    uint clu_group_start_idx = 0;
    uint clu_group_processed_size = GROUP_PROCESS_GROUP_NUM;

    // cluster
    uint clu_task_start_index = 0xFFFFFFFFu;

    while(true)
    {
        GroupMemoryBarrierWithGroupSync();
        
        if (group_index == 0)
		{
			group_clu_group_processed_mask = 0;
		}

        GroupMemoryBarrierWithGroupSync();

        uint clu_group_processed_mask = 0;
        if(has_clu_group_task)
        {
            if(clu_group_processed_size == GROUP_PROCESS_GROUP_NUM)
            {
                if(group_index == 0)
                {
                    InterlockedAdd(queue_pass_state[0].group_task_offset, GROUP_PROCESS_GROUP_NUM, group_start_cul_group_idx);
                }
                GroupMemoryBarrierWithGroupSync();

                clu_group_processed_size = 0;
                clu_group_start_idx = group_start_cul_group_idx;
            }

            const uint clu_group_task_index = clu_group_start_idx + clu_group_processed_size + group_index;
            bool is_clu_group_ready = (clu_group_processed_size + group_index) < GROUP_PROCESS_GROUP_NUM;

            uint2 group_task;
            if(clu_group_task_index < init_clu_group_num)// load from init cluster group task queue
            {
                group_task = cluster_group_init_task_queue.Load2(clu_group_task_index * 8); //uint2 = 8 bytes
            }
            else
            {
                group_task = cluster_group_task_queue.Load2(clu_group_task_index * 8);//uint2 = 8 bytes
                is_clu_group_ready = is_clu_group_ready && (group_task.x != 0) && (group_task.y != 0);
            }

            const uint group_task_instance_index = group_task.x;
            const uint group_task_clu_group_instance_index = group_task.y;

            // some cluster group maybe not ready
            
            if(is_clu_group_ready)
            {
                InterlockedOr(group_clu_group_processed_mask, 1u << group_index);
            }
            AllMemoryBarrierWithGroupSync();
            clu_group_processed_mask = group_clu_group_processed_mask;

            if(clu_group_processed_mask & 1u)
            {
                int batch_group_size = firstbitlow(~clu_group_processed_mask);
                batch_group_size = batch_group_size == -1 ? GROUP_SIZE : batch_group_size;
                if (group_index < uint(batch_group_size))
                {
                    ProcessClusterGroup(group_task_instance_index,group_task_clu_group_instance_index);
                }
                clu_group_processed_size += batch_group_size;
                continue;
            }
        }
        
        if(clu_task_start_index == 0xFFFFFFFFu)
        {
            if(group_index == 0)
            {
                InterlockedAdd(queue_pass_state[0].cluster_task_offset, 1, group_start_clu_index);
            }
            GroupMemoryBarrierWithGroupSync();
            clu_task_start_index = group_start_clu_index;
        }

        if(group_index == 0)
        {
            group_task_num = queue_pass_state[0].clu_group_num;
            group_clu_task_size = cluster_task_batch_size.Load(clu_task_start_index);;
        }
        GroupMemoryBarrierWithGroupSync();

        uint clu_task_ready_size = group_clu_task_size;

        if(!has_clu_group_task && clu_task_ready_size == 0)
        {
            break;
        }

        if(clu_task_ready_size > 0)
        {
            if(group_index < clu_task_ready_size)
            {
                const uint2 cluster_task = cluster_task_queue.Load2(clu_task_start_index);
                const uint cluster_task_instance_index = cluster_task.x;
                const uint cluster_task_clu_start_index = cluster_task.y;
                ProcessClusterBatch(cluster_task_clu_start_index + group_index, cluster_task_instance_index);
            }
            clu_task_start_index = 0xFFFFFFFFu;
        }

        if (has_clu_group_task && group_task_num == 0)
		{
			has_clu_group_task = false;
		}
    }
}


