#include "SimNaniteBasePass.h"

using namespace Graphics;

void CSimNaniteBasePass::Init(CSimNaniteBasePassInitDesc* nanite_init_desc)
{
    CreatePSO(nanite_init_desc);
}

void CSimNaniteBasePass::Render(SNaniteBasePassContext* context)
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Sim Nanite BasePass");

    D3D12_VIEWPORT view_port;
    view_port.Width = (float)g_VisibilityBuffer.GetWidth();
    view_port.Height = (float)g_VisibilityBuffer.GetHeight();
    view_port.MinDepth = 0.0f;
    view_port.MaxDepth = 1.0f;

    D3D12_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = (LONG)g_VisibilityBuffer.GetWidth();
    scissor.bottom = (LONG)g_VisibilityBuffer.GetHeight();

    // depth resolve
    {
        gfxContext.SetRootSignature(m_depth_resolve_root_sig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetConstantBuffer(0, context->m_nanite_gloabl_constant_address);

        gfxContext.TransitionResource(g_MatIdBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(g_MaterialIDDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        gfxContext.ClearDepth(g_MaterialIDDepth);

        gfxContext.SetDepthStencilTarget(g_SceneDepthBuffer.GetDSV());

        gfxContext.SetViewportAndScissor(view_port, scissor);
        gfxContext.SetDynamicDescriptor(1, 0, g_MatIdBuffer.GetSRV());
        gfxContext.FlushResourceBarriers();

        gfxContext.SetPipelineState(m_depth_resolve_pso);
        gfxContext.Draw(3);
    }

    {
        gfxContext.SetRootSignature(m_base_pass_root_sig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
       
        gfxContext.SetConstantBuffer(0, context->m_nanite_gloabl_constant_address);

        gfxContext.TransitionResource(*context->culled_instance_scene_data, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);//t4
        gfxContext.TransitionResource(*context->scene_cluster_infos, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);//t5
        gfxContext.TransitionResource(g_VisibilityBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);//t7
        gfxContext.TransitionResource(g_RasterizationDepth, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);//t8
        gfxContext.TransitionResource(g_MaterialIDDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);//depth

        gfxContext.SetViewportAndScissor(view_port, scissor);
        
        gfxContext.SetDynamicDescriptor(2, 5, context->culled_instance_scene_data->GetSRV());//t5
        gfxContext.SetDynamicDescriptor(2, 6, context->scene_cluster_infos->GetSRV());//t6
        gfxContext.SetDynamicDescriptor(2, 7, g_VisibilityBuffer.GetSRV());//t7
        gfxContext.SetDynamicDescriptor(2, 8, g_RasterizationDepth.GetDSV_ReadOnly());//t8

        // mesh material draw command loop
        std::vector<SNaniteMeshInstance>& mesh_instances = *context->mesh_instances;
        for (int idx = 0; idx < mesh_instances.size(); idx++)
        {
            SNaniteMeshInstance& mesh_instance = mesh_instances[idx];
            gfxContext.SetDynamicDescriptor(2, 0, mesh_instance.m_vertex_pos_buffer.GetSRV());//t0
            gfxContext.SetDynamicDescriptor(2, 1, mesh_instance.m_vertex_norm_buffer.GetSRV());//t1
            gfxContext.SetDynamicDescriptor(2, 2, mesh_instance.m_vertex_uv_buffer.GetSRV());//t2
            gfxContext.SetDynamicDescriptor(2, 3, mesh_instance.m_index_buffer.GetSRV());//t3

            gfxContext.SetDynamicDescriptor(2, 6, mesh_instance.m_texture.GetSRV());//t6
            gfxContext.SetDescriptorTable(3, (*context->s_SamplerHeap)[mesh_instance.m_sampler_table_idx]);

            //SBasePassParam base_pass_params;
            //base_pass_params.material_index = idx;
            //DynAlloc cb = gfxContext.ReserveUploadMemory(sizeof(SBasePassParam));
            //memcpy(cb.DataPtr, &base_pass_params, sizeof(SBasePassParam));
            //gfxContext.SetConstantBuffer(1, cb.GpuAddress);

            gfxContext.FlushResourceBarriers();

            gfxContext.SetPipelineState(m_depth_resolve_pso);
            gfxContext.Draw(3);
        }

    }

    gfxContext.Finish();
}

void CSimNaniteBasePass::CreatePSO(CSimNaniteBasePassInitDesc* nanite_init_desc)
{
    {
        m_depth_resolve_root_sig.Reset(4);
        m_depth_resolve_root_sig[0].InitAsConstantBuffer(0);
        m_depth_resolve_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        m_depth_resolve_root_sig.Finalize(L"m_depth_mat_id_resolve_root_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        CShaderCompiler& shader_compiler = *nanite_init_desc->m_shader_compiler;
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"depth_resolve_vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"depth_resolve_ps_main", L"ps_5_1", nullptr, 0);

        DXGI_FORMAT DepthFormat = g_MaterialIDDepth.GetFormat();

        m_depth_resolve_pso = GraphicsPSO(L"depth resolve pso");
        m_depth_resolve_pso.SetRootSignature(m_depth_resolve_root_sig);
        m_depth_resolve_pso.SetRasterizerState(RasterizerDefault);
        m_depth_resolve_pso.SetBlendState(BlendDisable);
        m_depth_resolve_pso.SetInputLayout(0, nullptr);
        m_depth_resolve_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_depth_resolve_pso.SetRenderTargetFormats(0, nullptr, DepthFormat);
        m_depth_resolve_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_depth_resolve_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_depth_resolve_pso.Finalize();
    }

    {
        m_base_pass_root_sig.Reset(4);
        m_base_pass_root_sig[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
        m_base_pass_root_sig[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_base_pass_root_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 9);
        m_base_pass_root_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_base_pass_root_sig.Finalize(L"m_base_pass_root_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        CShaderCompiler& shader_compiler = *nanite_init_desc->m_shader_compiler;
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_base_pass_pso = GraphicsPSO(L"Base pass pso");
        m_base_pass_pso.SetRootSignature(m_base_pass_root_sig);
        m_base_pass_pso.SetRasterizerState(RasterizerDefault);
        m_base_pass_pso.SetBlendState(BlendDisable);

        D3D12_DEPTH_STENCIL_DESC depth_state = DepthStateTestEqual;
        depth_state.DepthEnable = TRUE;
        depth_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        depth_state.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        depth_state.StencilEnable = FALSE;

        DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
        DXGI_FORMAT DepthFormat = g_MaterialIDDepth.GetFormat();

        m_base_pass_pso.SetDepthStencilState(depth_state);
        m_base_pass_pso.SetInputLayout(0, nullptr);
        m_base_pass_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_base_pass_pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
        m_base_pass_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_base_pass_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_base_pass_pso.Finalize();
    }
}
