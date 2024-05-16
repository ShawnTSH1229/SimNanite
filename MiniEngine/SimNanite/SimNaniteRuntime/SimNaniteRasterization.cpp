#include "SimNaniteRasterization.h"

using namespace Graphics;

void CRasterizationPass::Init()
{
	// hardware command signature
	{
		hardware_indirect_cmd_sig.Reset(1);
		hardware_indirect_cmd_sig[0].Draw();
		hardware_indirect_cmd_sig.Finalize();

		m_hardware_indirect_draw_sig.Reset(2, 0);
		m_hardware_indirect_draw_sig[0].InitAsConstantBuffer(0);
		m_hardware_indirect_draw_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_VERTEX);
		m_hardware_indirect_draw_sig.Finalize(L"m_hardware_indirect_draw_sig");

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteHardwareRasterization.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteHardwareRasterization.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        DXGI_FORMAT color_formats[2];
        color_formats[0] = g_VisibilityBuffer.GetFormat();
        color_formats[1] = g_MatIdBuffer.GetFormat();

        m_hardware_indirect_draw_pso = GraphicsPSO(L"vis instance cull");
        m_hardware_indirect_draw_pso.SetRootSignature(m_hardware_indirect_draw_sig);
        m_hardware_indirect_draw_pso.SetRasterizerState(RasterizerDefault);
        m_hardware_indirect_draw_pso.SetBlendState(BlendDisable);
        m_hardware_indirect_draw_pso.SetDepthStencilState(DepthStateReadWrite);
        m_hardware_indirect_draw_pso.SetInputLayout(0, nullptr);
        m_hardware_indirect_draw_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_hardware_indirect_draw_pso.SetRenderTargetFormats(2, color_formats, g_RasterizationDepth.GetFormat());
        m_hardware_indirect_draw_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_hardware_indirect_draw_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_hardware_indirect_draw_pso.Finalize();
	}

    {
        m_intermediate_depth_gen_sig.Reset(2);
        m_intermediate_depth_gen_sig[0].InitAsConstantBuffer(0);
        m_intermediate_depth_gen_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_intermediate_depth_gen_sig.Finalize(L"m_intermediate_depth_gen_sig");

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteSoftwareRasterization.hlsl", L"intermediate_depth_buffer_gen_vs", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteSoftwareRasterization.hlsl", L"intermediate_depth_buffer_gen_ps", L"ps_5_1", nullptr, 0);

        m_intermediate_depth_gen_pso = GraphicsPSO(L"vis instance cull");
        m_intermediate_depth_gen_pso.SetRootSignature(m_intermediate_depth_gen_sig);
        m_intermediate_depth_gen_pso.SetRasterizerState(RasterizerDefault);
        m_intermediate_depth_gen_pso.SetBlendState(BlendDisable);
        m_intermediate_depth_gen_pso.SetDepthStencilState(DepthStateDisabled);
        m_intermediate_depth_gen_pso.SetInputLayout(0, nullptr);
        m_intermediate_depth_gen_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_intermediate_depth_gen_pso.SetRenderTargetFormat(g_IntermediateDepth.GetFormat(), DXGI_FORMAT_UNKNOWN);
        m_intermediate_depth_gen_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_intermediate_depth_gen_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_intermediate_depth_gen_pso.Finalize();
    }

    {
        m_software_indirect_draw_sig.Reset(3);
        m_software_indirect_draw_sig[0].InitAsConstantBuffer(0);
        m_software_indirect_draw_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
        m_software_indirect_draw_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
        m_software_indirect_draw_sig.Finalize(L"m_software_indirect_draw_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteSoftwareRasterization.hlsl", L"SoftwareRasterization", L"cs_5_1", nullptr, 0);

        m_software_indirect_draw_pso = ComputePSO(L"m_software_indirect_draw_pso");
        m_software_indirect_draw_pso.SetRootSignature(m_software_indirect_draw_sig);
        m_software_indirect_draw_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_software_indirect_draw_pso.Finalize();
    }
}

void CRasterizationPass::Rasterization()
{
    {
        GraphicsContext& gfxContext = GraphicsContext::Begin(L"HardwareRasterization");

        {
            gfxContext.SetRootSignature(m_hardware_indirect_draw_sig);
            gfxContext.SetPipelineState(m_hardware_indirect_draw_pso);

            gfxContext.TransitionResource(GetSimNaniteGlobalResource().hardware_draw_indirect, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            gfxContext.TransitionResource(GetSimNaniteGlobalResource().scene_indirect_draw_cmds, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            gfxContext.FlushResourceBarriers();

            gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
            gfxContext.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().scene_indirect_draw_cmds.GetSRV());
            gfxContext.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer.GetSRV());
            gfxContext.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_scene_index_buffer.GetSRV());

            gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gfxContext.TransitionResource(g_VisibilityBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gfxContext.TransitionResource(g_MatIdBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gfxContext.TransitionResource(g_RasterizationDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            D3D12_CPU_DESCRIPTOR_HANDLE hardware_rasterizarion_rts[2];
            hardware_rasterizarion_rts[0] = g_VisibilityBuffer.GetRTV();
            hardware_rasterizarion_rts[1] = g_MatIdBuffer.GetRTV();

            gfxContext.SetRenderTargets(2, hardware_rasterizarion_rts, g_RasterizationDepth.GetDSV());
            gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);

            gfxContext.ClearColor(g_VisibilityBuffer);
            gfxContext.ClearColor(g_MatIdBuffer);
            gfxContext.ClearDepth(g_RasterizationDepth);

            gfxContext.ExecuteIndirect(hardware_indirect_cmd_sig, GetSimNaniteGlobalResource().hardware_draw_indirect, 0, 1, nullptr);
        }

        // generate intermidiate buffer
        {
            gfxContext.SetRootSignature(m_intermediate_depth_gen_sig);
            gfxContext.SetPipelineState(m_intermediate_depth_gen_pso);

            gfxContext.TransitionResource(g_RasterizationDepth, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            gfxContext.TransitionResource(g_IntermediateDepth, D3D12_RESOURCE_STATE_RENDER_TARGET);
            gfxContext.FlushResourceBarriers();

            D3D12_CPU_DESCRIPTOR_HANDLE intermediate_rt[1];
            intermediate_rt[0] = g_IntermediateDepth.GetRTV();
            gfxContext.SetRenderTargets(1, intermediate_rt);
            gfxContext.SetViewportAndScissor(GetSimNaniteGlobalResource().m_MainViewport, GetSimNaniteGlobalResource().m_MainScissor);
            gfxContext.ClearColor(g_IntermediateDepth);

            gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
            gfxContext.SetDynamicDescriptors(1, 0, 1, &g_RasterizationDepth.GetDepthSRV());
            gfxContext.Draw(3);
        }

        gfxContext.Finish();
    }


    // software rasterization
    {
        ComputeContext& cptContext = ComputeContext::Begin(L"Software rasterization");
        cptContext.SetRootSignature(m_software_indirect_draw_sig);
        cptContext.SetPipelineState(m_software_indirect_draw_pso);

        cptContext.TransitionResource(GetSimNaniteGlobalResource().software_draw_indirect, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().scene_indirect_draw_cmds, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        cptContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        cptContext.TransitionResource(g_IntermediateDepth, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.TransitionResource(g_VisibilityBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cptContext.TransitionResource(g_MatIdBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        cptContext.FlushResourceBarriers();

        cptContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);

        cptContext.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().scene_indirect_draw_cmds.GetSRV());
        cptContext.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer.GetSRV());
        cptContext.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_scene_index_buffer.GetSRV());

        cptContext.SetDynamicDescriptors(2, 0, 1, &g_IntermediateDepth.GetUAV());
        cptContext.SetDynamicDescriptors(2, 1, 1, &g_VisibilityBuffer.GetUAV());
        cptContext.SetDynamicDescriptors(2, 2, 1, &g_MatIdBuffer.GetUAV());
        
        cptContext.DispatchIndirect(GetSimNaniteGlobalResource().software_draw_indirect);
        
        cptContext.Finish();
    }

}
