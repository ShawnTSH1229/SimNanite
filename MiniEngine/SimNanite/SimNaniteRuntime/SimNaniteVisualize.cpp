#include "SimNaniteVisualize.h"

using namespace Graphics;

void CSimNaniteVisualizer::Init()
{
    DXGI_FORMAT color_fornat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT depth_format = g_SceneDepthBuffer.GetFormat();

    // visualize cluster root signature
    {
        m_vis_cluster_root_sig.Reset(3, 0);
        m_vis_cluster_root_sig[0].InitAsConstantBuffer(0);
        m_vis_cluster_root_sig[1].InitAsConstantBuffer(1);
        m_vis_cluster_root_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
        m_vis_cluster_root_sig.Finalize(L"m_vis_cluster_root_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        D3D12_INPUT_ELEMENT_DESC pos_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeCluster.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeCluster.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_vis_cluster_pso = GraphicsPSO(L"vis cluster cull");
        m_vis_cluster_pso.SetRootSignature(m_vis_cluster_root_sig);
        m_vis_cluster_pso.SetRasterizerState(RasterizerDefault);
        m_vis_cluster_pso.SetBlendState(BlendDisable);
        m_vis_cluster_pso.SetDepthStencilState(DepthStateReadWrite);
        m_vis_cluster_pso.SetInputLayout(_countof(pos_layout), pos_layout);
        m_vis_cluster_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_vis_cluster_pso.SetRenderTargetFormats(1, &color_fornat, depth_format);
        m_vis_cluster_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_vis_cluster_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_vis_cluster_pso.Finalize();
    }

    // visualize instance culling root signature
    {
        m_vis_inst_cull_cmd_sig.Reset(1);
        m_vis_inst_cull_cmd_sig[0].Draw();
        m_vis_inst_cull_cmd_sig.Finalize();

        {
            m_vis_ins_cull_root_sig.Reset(2, 0);
            m_vis_ins_cull_root_sig[0].InitAsConstantBuffer(0);
            m_vis_ins_cull_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_VERTEX);
            m_vis_ins_cull_root_sig.Finalize(L"m_vis_ins_cull_root_sig");

            std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeInstanceCull.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
            std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeInstanceCull.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

            m_vis_ins_cull_pso = GraphicsPSO(L"vis instance cull");
            m_vis_ins_cull_pso.SetRootSignature(m_vis_ins_cull_root_sig);
            m_vis_ins_cull_pso.SetRasterizerState(RasterizerDefault);
            m_vis_ins_cull_pso.SetBlendState(BlendDisable);
            m_vis_ins_cull_pso.SetDepthStencilState(DepthStateReadWrite);
            m_vis_ins_cull_pso.SetInputLayout(0, nullptr);
            m_vis_ins_cull_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
            m_vis_ins_cull_pso.SetRenderTargetFormats(1, &color_fornat, depth_format);
            m_vis_ins_cull_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
            m_vis_ins_cull_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
            m_vis_ins_cull_pso.Finalize();
        }

        {
            m_gen_vis_ins_cull_root_sig.Reset(2, 0);
            m_gen_vis_ins_cull_root_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
            m_gen_vis_ins_cull_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
            m_gen_vis_ins_cull_root_sig.Finalize(L"m_gen_vis_ins_cull_root_sig");

            std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeInstanceCull.hlsl", L"GenerateInstaceCullVisCmd", L"cs_5_1", nullptr, 0);

            m_gen_vis_ins_cull_pso = ComputePSO(L"m_gen_vis_ins_cull_pso");
            m_gen_vis_ins_cull_pso.SetRootSignature(m_gen_vis_ins_cull_root_sig);
            m_gen_vis_ins_cull_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
            m_gen_vis_ins_cull_pso.Finalize();
        }

        {
            m_gen_vis_ins_cull_cmd_root_sig.Reset(2, 0);
            m_gen_vis_ins_cull_cmd_root_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            m_gen_vis_ins_cull_cmd_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            m_gen_vis_ins_cull_cmd_root_sig.Finalize(L"m_gen_vis_ins_cull_cmd_root_sig");

            std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeInstanceCull.hlsl", L"GenerateInstaceCullVisIndirectCmd", L"cs_5_1", nullptr, 0);

            m_gen_vis_ins_cull_cmd_pso = ComputePSO(L"m_gen_vis_ins_cull_cmd_pso");
            m_gen_vis_ins_cull_cmd_pso.SetRootSignature(m_gen_vis_ins_cull_cmd_root_sig);
            m_gen_vis_ins_cull_cmd_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
            m_gen_vis_ins_cull_cmd_pso.Finalize();
        }

        {
            std::vector<SNaniteMeshInstance>& mesh_instances = GetSimNaniteGlobalResource().m_mesh_instances;

            int total_instance_size = 0;
            for (int mesh_idx = 0; mesh_idx < GetSimNaniteGlobalResource().m_mesh_instances.size(); mesh_idx++)
            {
                CSimNaniteMeshResource nanite_mesh_resource = mesh_instances[mesh_idx].m_nanite_mesh_resource;
                uint32_t clu_group_idx = nanite_mesh_resource.m_nanite_lods[nanite_mesh_resource.m_nanite_lods.size() - 1].m_cluster_group_start;
                total_instance_size += GetSimNaniteGlobalResource().m_mesh_instances[mesh_idx].m_instance_datas.size() * nanite_mesh_resource.m_cluster_groups[clu_group_idx].m_cluster_num;
            }

            vis_instance_cull_cluster_draw.Create(L"vis_instance_culled_cluster", total_instance_size * 4, sizeof(SSimNaniteClusterDraw));
            vis_instance_cull_cmd_num.Create(L"vis_instance_cull_cmd_num", 1, sizeof(uint32_t));
            vis_instance_cull_indirect_draw_cmd.Create(L"vis_instance_cull_indirect_draw_cmd", 1, sizeof(D3D12_DRAW_ARGUMENTS));

            uint32_t global_cluster_group_num = 1;
            std::vector<SSimNaniteMeshLastLOD> meshes_last_lod;

            
            for (int mesh_idx = 0; mesh_idx < GetSimNaniteGlobalResource().m_mesh_instances.size(); mesh_idx++)
            {
                CSimNaniteMeshResource nanite_mesh_resource = mesh_instances[mesh_idx].m_nanite_mesh_resource;
                
                //todo: fix me
                SSimNaniteMeshLastLOD mesh_last_lod;
                mesh_last_lod.cluster_group_start_index = global_cluster_group_num + nanite_mesh_resource.m_nanite_lods[nanite_mesh_resource.m_nanite_lods.size() - 1].m_cluster_group_start;
                mesh_last_lod.cluster_group_num = nanite_mesh_resource.m_nanite_lods[nanite_mesh_resource.m_nanite_lods.size()-1].m_cluster_group_num;
                meshes_last_lod.push_back(mesh_last_lod);
                global_cluster_group_num += nanite_mesh_resource.m_cluster_groups.size();
            }

            vis_instance_cull_mesh_last_lod.Create(L"vis_instance_cull_mesh_last_lod", meshes_last_lod.size(), sizeof(SSimNaniteMeshLastLOD), meshes_last_lod.data());
        }
    }

#if CLUGROUP_CULL_DEBUG
    {
        m_vis_node_cull_cmd_sig.Reset(1);
        m_vis_node_cull_cmd_sig[0].Draw();
        m_vis_node_cull_cmd_sig.Finalize();

        {
            m_gen_vis_node_cull_root_sig.Reset(2, 0);
            m_gen_vis_node_cull_root_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
            m_gen_vis_node_cull_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
            m_gen_vis_node_cull_root_sig.Finalize(L"m_gen_vis_node_cull_root_sig");

            std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeClusterGroupCull.hlsl", L"GenerateClusterGroupCullVisCmd", L"cs_5_1", nullptr, 0);

            m_gen_vis_node_cull_pso = ComputePSO(L"m_gen_vis_node_cull_pso");
            m_gen_vis_node_cull_pso.SetRootSignature(m_gen_vis_node_cull_root_sig);
            m_gen_vis_node_cull_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
            m_gen_vis_node_cull_pso.Finalize();
        }

        vis_clu_group_culled_cluster.Create(L"vis_clu_group_culled_cluster", 4096, sizeof(SSimNaniteClusterDraw));
        vis_cul_group_cmd_num.Create(L"vis_cul_group_cmd_num", 1, sizeof(uint32_t));
        vis_node_cull_indirect_draw_cmd.Create(L"vis_node_cull_indirect_draw_cmd", 1, sizeof(D3D12_DRAW_ARGUMENTS));
    }
#endif

    {
        m_copy_buffer_sig.Reset(3);
        m_copy_buffer_sig[0].InitAsConstantBuffer(0);
        m_copy_buffer_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        m_copy_buffer_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_copy_buffer_sig.Finalize(L"m_copy_buffer_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteVisualizeRenderTarget.hlsl", L"CopyVisBufferToDestVisualizeRT", L"cs_5_1", nullptr, 0);

        m_copy_buffer_pso = ComputePSO(L"m_copy_buffer_pso");
        m_copy_buffer_pso.SetRootSignature(m_copy_buffer_sig);
        m_copy_buffer_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_copy_buffer_pso.Finalize();
    }
}

void CSimNaniteVisualizer::Render()
{
    //if (GetSimNaniteGlobalResource().m_need_update_freezing_data)
    {
        UpdateFreezingData();
        GetSimNaniteGlobalResource().m_need_update_freezing_data = false;
    }

    if (GetSimNaniteGlobalResource().vis_type == 7)
    {
        RenderInstanceCullingVisualize();
    }
#if DEBUG_CLUSTER_GROUP
    else if (GetSimNaniteGlobalResource().vis_type == 6)
    {
        RenderNodeCullingVisualize();
    }
#endif
    else if (GetSimNaniteGlobalResource().vis_type == 4)
    {
        RenderVisualizeBuffer();
    }
    else
    {
        RenderClusterVisualize();
    }
}

void CSimNaniteVisualizer::RenderClusterVisualize()
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    gfxContext.SetRootSignature(m_vis_cluster_root_sig);
    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetSimNaniteGlobalResource().s_TextureHeap.GetHeapPointer());
    gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetSimNaniteGlobalResource().s_SamplerHeap.GetHeapPointer());

    gfxContext.SetConstantBuffer(1, GetSimNaniteGlobalResource().m_view_constant_address);

    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
    gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    gfxContext.ClearColor(g_SceneColorBuffer);
    gfxContext.ClearDepth(g_SceneDepthBuffer);

    gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());

    gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);
    gfxContext.FlushResourceBarriers();

    std::srand(42);

    for (int idx = 0; idx < GetSimNaniteGlobalResource().m_mesh_instances.size(); idx++)
    {
        SNaniteMeshInstance& mesh_instance = GetSimNaniteGlobalResource().m_mesh_instances[idx];

        gfxContext.SetDescriptorTable(2, GetSimNaniteGlobalResource().s_TextureHeap[mesh_instance.m_instance_buffer_idx]);

        gfxContext.SetPipelineState(m_vis_cluster_pso);

        gfxContext.SetVertexBuffer(0, mesh_instance.m_vertex_pos_buffer.VertexBufferView());
        gfxContext.SetIndexBuffer(mesh_instance.m_index_buffer.IndexBufferView());

        int instance_count = mesh_instance.m_instance_datas.size();

        CSimNaniteMeshResource& nanite_mesh_res = mesh_instance.m_nanite_mesh_resource;
        uint32_t want_visualize_lod = GetSimNaniteGlobalResource().m_vis_cluster_lod;
        want_visualize_lod = want_visualize_lod > 0 ? want_visualize_lod : 0;
        uint32_t visualize_lod = (nanite_mesh_res.m_nanite_lods.size() > want_visualize_lod) ? want_visualize_lod : nanite_mesh_res.m_nanite_lods.size() - 1;
        const CSimNaniteLodResource& nanite_lod = nanite_mesh_res.m_nanite_lods[visualize_lod];
        for (int nanite_group_idx = 0; nanite_group_idx < nanite_lod.m_cluster_group_num; nanite_group_idx++)
        {
            int sub_group_idx = nanite_lod.m_cluster_group_start + nanite_group_idx;
            const CSimNaniteClusterGrpupResource& nanite_group = nanite_mesh_res.m_cluster_groups[sub_group_idx];

            for (int clu_idx = 0; clu_idx < nanite_group.m_cluster_num; clu_idx++)
            {
                int sub_clu_idx = nanite_group.m_clusters_indices[clu_idx];
                const CSimNaniteClusterResource& nanite_clu = nanite_mesh_res.m_clusters[sub_clu_idx];

                SNaniteMeshClusterVisualize visualize_color;
                visualize_color.m_visualize_color = Math::Vector3(float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX, float(std::rand()) / RAND_MAX);

                DynAlloc cb = gfxContext.ReserveUploadMemory(sizeof(SNaniteMeshClusterVisualize));
                memcpy(cb.DataPtr, &visualize_color, sizeof(SNaniteMeshClusterVisualize));
                gfxContext.SetConstantBuffer(0, cb.GpuAddress);

                gfxContext.DrawIndexedInstanced(nanite_clu.m_index_count, instance_count, nanite_clu.m_start_index_location, nanite_clu.m_start_vertex_location, 0);
            }
        }
    }

    gfxContext.Finish();
}

void CSimNaniteVisualizer::UpdateFreezingData()
{
    ComputeContext& cptContext = ComputeContext::Begin(L"Visualize Instance Cull");

    {
        cptContext.SetRootSignature(m_gen_vis_ins_cull_root_sig);
        cptContext.SetPipelineState(m_gen_vis_ins_cull_pso);

        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_culled_instance_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(vis_instance_cull_mesh_last_lod, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_group_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        cptContext.TransitionResource(vis_instance_cull_cluster_draw, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.TransitionResource(vis_instance_cull_cmd_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.ClearUAV(vis_instance_cull_cmd_num);
        cptContext.FlushResourceBarriers();

        cptContext.SetDynamicDescriptors(0, 0, 1, &GetSimNaniteGlobalResource().m_culled_instance_num.GetSRV());
        cptContext.SetDynamicDescriptors(0, 1, 1, &vis_instance_cull_mesh_last_lod.GetSRV());
        cptContext.SetDynamicDescriptors(0, 2, 1, &GetSimNaniteGlobalResource().m_scene_cluster_group_infos.GetSRV());
        cptContext.SetDynamicDescriptors(0, 3, 1, &GetSimNaniteGlobalResource().m_scene_cluster_infos.GetSRV());
        cptContext.SetDynamicDescriptors(0, 4, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());

        cptContext.SetDynamicDescriptors(1, 0, 1, &vis_instance_cull_cluster_draw.GetUAV());
        cptContext.SetDynamicDescriptors(1, 1, 1, &vis_instance_cull_cmd_num.GetUAV());

        cptContext.Dispatch((GetSimNaniteGlobalResource().m_culling_parameters.m_total_instance_num + 64 - 1) / 64, 1, 1);
    }

    {
        cptContext.SetRootSignature(m_gen_vis_ins_cull_cmd_root_sig);
        cptContext.SetPipelineState(m_gen_vis_ins_cull_cmd_pso);

        cptContext.TransitionResource(vis_instance_cull_cmd_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(vis_instance_cull_indirect_draw_cmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cptContext.FlushResourceBarriers();

        cptContext.SetDynamicDescriptors(0, 0, 1, &vis_instance_cull_cmd_num.GetSRV());
        cptContext.SetDynamicDescriptors(1, 0, 1, &vis_instance_cull_indirect_draw_cmd.GetUAV());

        cptContext.Dispatch(1, 1, 1);
    }
#if CLUGROUP_CULL_DEBUG
    {
        cptContext.SetRootSignature(m_gen_vis_node_cull_root_sig);
        cptContext.SetPipelineState(m_gen_vis_node_cull_pso);

        cptContext.TransitionResource(GetSimNaniteGlobalResource(). m_cluster_group_cull_vis, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_cluster_group_culled_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        cptContext.TransitionResource(vis_clu_group_culled_cluster, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.TransitionResource(vis_cul_group_cmd_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.ClearUAV(vis_cul_group_cmd_num);
        cptContext.FlushResourceBarriers();

        cptContext.SetDynamicDescriptors(0, 0, 1, &GetSimNaniteGlobalResource().m_cluster_group_cull_vis.GetSRV());
        cptContext.SetDynamicDescriptors(0, 1, 1, &GetSimNaniteGlobalResource().m_cluster_group_culled_num.GetSRV());
        cptContext.SetDynamicDescriptors(0, 2, 1, &GetSimNaniteGlobalResource().m_scene_cluster_infos.GetSRV());
        cptContext.SetDynamicDescriptors(0, 3, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetSRV());

        cptContext.SetDynamicDescriptors(1, 0, 1, &vis_clu_group_culled_cluster.GetUAV());
        cptContext.SetDynamicDescriptors(1, 1, 1, &vis_cul_group_cmd_num.GetUAV());

        cptContext.Dispatch((GetSimNaniteGlobalResource().m_culling_parameters.m_total_instance_num * 32 + 64 - 1) / 64, 1, 1);
    }

    {
        cptContext.SetRootSignature(m_gen_vis_ins_cull_cmd_root_sig);
        cptContext.SetPipelineState(m_gen_vis_ins_cull_cmd_pso);

        cptContext.TransitionResource(vis_cul_group_cmd_num, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(vis_node_cull_indirect_draw_cmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cptContext.FlushResourceBarriers();

        cptContext.SetDynamicDescriptors(0, 0, 1, &vis_cul_group_cmd_num.GetSRV());
        cptContext.SetDynamicDescriptors(1, 0, 1, &vis_node_cull_indirect_draw_cmd.GetUAV());

        cptContext.Dispatch(1, 1, 1);
    }
#endif
    cptContext.Finish();
}

void CSimNaniteVisualizer::RenderInstanceCullingVisualize()
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"RenderInstanceCullingVisualize");

    {
        gfxContext.SetRootSignature(m_vis_ins_cull_root_sig);
        gfxContext.SetPipelineState(m_vis_ins_cull_pso);

        gfxContext.TransitionResource(vis_instance_cull_indirect_draw_cmd, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        gfxContext.TransitionResource(vis_instance_cull_cluster_draw, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.FlushResourceBarriers();

        gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
        gfxContext.SetDynamicDescriptors(1, 0, 1, &vis_instance_cull_cluster_draw.GetSRV());
        gfxContext.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer.GetSRV());
        gfxContext.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_scene_index_buffer.GetSRV());

        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
        gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);

        gfxContext.ClearColor(g_SceneColorBuffer);
        gfxContext.ClearDepth(g_SceneDepthBuffer);

        gfxContext.ExecuteIndirect(m_vis_inst_cull_cmd_sig, vis_instance_cull_indirect_draw_cmd, 0, 1, nullptr);
    }

    
    gfxContext.Finish();
}

#if CLUGROUP_CULL_DEBUG
void CSimNaniteVisualizer::RenderNodeCullingVisualize()
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"RenderNodeCullingVisualize");

    {
        gfxContext.SetRootSignature(m_vis_ins_cull_root_sig);
        gfxContext.SetPipelineState(m_vis_ins_cull_pso);

        gfxContext.TransitionResource(vis_node_cull_indirect_draw_cmd, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        gfxContext.TransitionResource(vis_clu_group_culled_cluster, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.FlushResourceBarriers();

        gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
        gfxContext.SetDynamicDescriptors(1, 0, 1, &vis_clu_group_culled_cluster.GetSRV());
        gfxContext.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer.GetSRV());
        gfxContext.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_scene_index_buffer.GetSRV());

        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
        gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);

        gfxContext.ClearColor(g_SceneColorBuffer);
        gfxContext.ClearDepth(g_SceneDepthBuffer);

        gfxContext.ExecuteIndirect(m_vis_inst_cull_cmd_sig, vis_node_cull_indirect_draw_cmd, 0, 1, nullptr);
    }


    gfxContext.Finish();
}

#endif

void CSimNaniteVisualizer::RenderVisualizeBuffer()
{
    ComputeContext& cptContext = ComputeContext::Begin(L"RenderVisualizeBuffer");

    cptContext.SetRootSignature(m_copy_buffer_sig);
    cptContext.SetPipelineState(m_copy_buffer_pso);

    cptContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);

    cptContext.SetDynamicDescriptors(1, 0, 1, &g_VisibilityBuffer.GetSRV());
    cptContext.SetDynamicDescriptors(2, 0, 1, &g_SceneColorBuffer.GetUAV());

    cptContext.Dispatch((GetSimNaniteGlobalResource().m_MainViewport.Width + 7) / 8, (GetSimNaniteGlobalResource().m_MainViewport.Height + 7) / 8, 1);
    cptContext.Finish();
}
