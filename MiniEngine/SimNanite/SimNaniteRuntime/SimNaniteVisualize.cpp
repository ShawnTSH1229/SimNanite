#include "SimNaniteVisualize.h"

using namespace Graphics;

void CSimNaniteVisualizer::Init()
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

    DXGI_FORMAT color_fornat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT depth_format = g_SceneDepthBuffer.GetFormat();

    std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/VisualizeNaniteCluster.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
    std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/VisualizeNaniteCluster.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

    m_vis_cluster_pso = GraphicsPSO(L"pbr pbr");
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

void CSimNaniteVisualizer::Render()
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
            int sub_group_idx = nanite_lod.m_cluster_group_index[nanite_group_idx];
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
