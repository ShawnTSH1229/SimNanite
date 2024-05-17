#include "SimNaniteBasePassRendering.h"

using namespace Graphics;

void CBasePassRendering::Init()
{
    {
        m_resolve_material_idx_sig.Reset(2);
        m_resolve_material_idx_sig[0].InitAsConstantBuffer(0);
        m_resolve_material_idx_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_resolve_material_idx_sig.Finalize(L"m_resolve_material_idx_sig");

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteResolveMaterialIndex.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteResolveMaterialIndex.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_resolve_material_idx_pso = GraphicsPSO(L"m_resolve_material_idx_pso");
        m_resolve_material_idx_pso.SetRootSignature(m_resolve_material_idx_sig);
        m_resolve_material_idx_pso.SetRasterizerState(RasterizerDefault);
        m_resolve_material_idx_pso.SetBlendState(BlendDisable);
        m_resolve_material_idx_pso.SetDepthStencilState(DepthStateReadWrite);
        m_resolve_material_idx_pso.SetInputLayout(0, nullptr);
        m_resolve_material_idx_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_resolve_material_idx_pso.SetDepthTargetFormat(g_MaterialIDDepth.GetFormat());
        m_resolve_material_idx_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_resolve_material_idx_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_resolve_material_idx_pso.Finalize();
    }

    {
        m_basepass_sig.Reset(5);
        m_basepass_sig[0].InitAsConstantBuffer(0);
        m_basepass_sig[1].InitAsConstantBuffer(1);
        m_basepass_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
        m_basepass_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_basepass_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_basepass_sig.Finalize(L"m_basepass_sig");

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteBasePass.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteBasePass.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_basepass_pso = GraphicsPSO(L"m_basepass_pso");
        m_basepass_pso.SetRootSignature(m_basepass_sig);
        m_basepass_pso.SetRasterizerState(RasterizerDefault);
        m_basepass_pso.SetBlendState(BlendDisable);
        m_basepass_pso.SetDepthStencilState(DepthStateTestEqual);
        m_basepass_pso.SetInputLayout(0, nullptr);
        m_basepass_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_basepass_pso.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat() , g_MaterialIDDepth.GetFormat());
        m_basepass_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_basepass_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_basepass_pso.Finalize();
    }
}

void CBasePassRendering::Rendering()
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"BasePassRendering");
    // resolve material index
    {
        gfxContext.SetRootSignature(m_resolve_material_idx_sig);
        gfxContext.SetPipelineState(m_resolve_material_idx_pso);

        gfxContext.TransitionResource(g_MatIdBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(g_MaterialIDDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        gfxContext.FlushResourceBarriers();

        gfxContext.SetRenderTargets(0,nullptr, g_MaterialIDDepth.GetDSV());
        gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);
        gfxContext.ClearDepth(g_MaterialIDDepth);

        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
        gfxContext.SetDynamicDescriptors(1, 0, 1, &g_MatIdBuffer.GetSRV());
        gfxContext.Draw(3);
    }
    // base pass rendering

    {
        gfxContext.SetRootSignature(m_basepass_sig);
        gfxContext.SetPipelineState(m_basepass_pso);

        gfxContext.TransitionResource(GetSimNaniteGlobalResource().scene_indirect_draw_cmds, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_uv_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(g_VisibilityBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(g_IntermediateDepth, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.FlushResourceBarriers();


        D3D12_CPU_DESCRIPTOR_HANDLE render_target[1];
        render_target[0] = g_SceneColorBuffer.GetRTV();
        gfxContext.SetRenderTargets(1, render_target, g_MaterialIDDepth.GetDSV());
        gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);
        gfxContext.ClearColor(g_SceneColorBuffer);
        
        gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetSimNaniteGlobalResource().s_TextureHeap.GetHeapPointer());
        gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetSimNaniteGlobalResource().s_SamplerHeap.GetHeapPointer());

        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        gfxContext.SetDynamicDescriptors(2, 0, 1, &GetSimNaniteGlobalResource().scene_indirect_draw_cmds.GetSRV());
        gfxContext.SetDynamicDescriptors(2, 1, 1, &GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer.GetSRV());
        gfxContext.SetDynamicDescriptors(2, 2, 1, &GetSimNaniteGlobalResource().m_scene_uv_vertex_buffer.GetSRV());
        gfxContext.SetDynamicDescriptors(2, 3, 1, &GetSimNaniteGlobalResource().m_scene_index_buffer.GetSRV());
        gfxContext.SetDynamicDescriptors(2, 4, 1, &g_VisibilityBuffer.GetSRV());
        gfxContext.SetDynamicDescriptors(2, 5, 1, &g_IntermediateDepth.GetSRV());

        gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);

        std::vector<SNaniteMeshInstance>& scene_data = GetSimNaniteGlobalResource().m_mesh_instances;
        
        // todo: fix me
        {
            SNaniteMeshInstance& mesh_data = scene_data[0];
            gfxContext.SetDescriptorTable(3, GetSimNaniteGlobalResource().s_TextureHeap[mesh_data.m_tex_table_index]);
            gfxContext.SetDescriptorTable(4, GetSimNaniteGlobalResource().s_SamplerHeap[mesh_data.m_sampler_table_idx]);
        }


        for (int mesh_index = 0; mesh_index < scene_data.size(); mesh_index++)
        {
            SNaniteMeshInstance& mesh_data = scene_data[mesh_index];

            SBasePassParam basepass_params;
            basepass_params.material_index = mesh_index + 1;
            DynAlloc cb = gfxContext.ReserveUploadMemory(sizeof(SBasePassParam));
            memcpy(cb.DataPtr, &basepass_params, sizeof(SBasePassParam));
            gfxContext.SetConstantBuffer(1, cb.GpuAddress);
            

            gfxContext.Draw(3);
        }
        

    }
    gfxContext.Finish();
}
