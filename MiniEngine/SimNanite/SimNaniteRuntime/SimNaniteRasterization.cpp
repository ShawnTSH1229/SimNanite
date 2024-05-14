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
}

void CRasterizationPass::Rasterization()
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"HardwareRasterization");

    {
        gfxContext.SetRootSignature(m_hardware_indirect_draw_sig);
        gfxContext.SetPipelineState(m_hardware_indirect_draw_pso);

        gfxContext.TransitionResource(GetSimNaniteGlobalResource().hardware_draw_indirect, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().hardware_indirect_draw_cmds, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_pos_vertex_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.TransitionResource(GetSimNaniteGlobalResource().m_scene_index_buffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        gfxContext.FlushResourceBarriers();

        gfxContext.SetConstantBuffer(0, GetSimNaniteGlobalResource().m_view_constant_address);
        gfxContext.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().hardware_indirect_draw_cmds.GetSRV());
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


    gfxContext.Finish();
}
