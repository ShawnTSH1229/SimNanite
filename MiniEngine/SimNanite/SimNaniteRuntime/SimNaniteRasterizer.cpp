#include "SimNaniteRasterizer.h"
using namespace Graphics;

void CSimHardwareRasterizer::Init(SNaniteRasterizerInitDesc& init_desc)
{
	CreatePSO(init_desc);
}

void CSimHardwareRasterizer::Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context)
{
	GraphicsContext& HardwareRasterization = GraphicsContext::Begin(L"Render UI");

	D3D12_VIEWPORT m_view_port;
	m_view_port.Width = (float)g_VisibilityBuffer.GetWidth();
	m_view_port.Height = (float)g_VisibilityBuffer.GetHeight();
	m_view_port.MinDepth = 0.0f;
	m_view_port.MaxDepth = 1.0f;

	D3D12_RECT m_scissor;
	m_scissor.left = 0;
	m_scissor.top = 0;
	m_scissor.right = (LONG)g_VisibilityBuffer.GetWidth();
	m_scissor.bottom = (LONG)g_VisibilityBuffer.GetHeight();

	//{
	//	Context.SetRootSignature(depth_resolve_root_sig);
	//	Context.SetPipelineState(depth_resolvepso);
	//	Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//
	//	Context.TransitionResource(g_RasterizationDepthUAV, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	//	Context.TransitionResource(g_RasterizationDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//	Context.ClearDepth(g_RasterizationDepth);
	//	Context.SetDynamicDescriptor(1, 0, g_RasterizationDepthUAV.GetSRV());
	//
	//	Context.SetDepthStencilTarget(g_RasterizationDepth.GetDSV());
	//	Context.SetViewportAndScissor(m_view_port, m_scissor);
	//	Context.Draw(3);
	//}
	//
	//{
	//	Context.SetRootSignature(nanite_draw_indirect_root_sig);
	//	Context.SetPipelineState(nanite_draw_indirect_pso);
	//	Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//
	//	Context.TransitionResource(g_VisibilityBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//	Context.TransitionResource(g_RasterizationDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//
	//	Context.SetRenderTarget(g_VisibilityBuffer.GetRTV(), g_RasterizationDepth.GetDSV());
	//	Context.SetViewportAndScissor(m_view_port, m_scissor);
	//
	//	StructuredBuffer* indirect_cmd = rasterization_context->m_hardware_indirect_draw_command;
	//	Context.ExecuteIndirect(nanite_draw_indirect_command_sig, *indirect_cmd, 0, 1024, &indirect_cmd->GetCounterBuffer(), indirect_cmd->GetBufferSize());
	//}


	HardwareRasterization.Finish();
}

void CSimHardwareRasterizer::CreatePSO(SNaniteRasterizerInitDesc& init_desc)
{
	nanite_draw_indirect_command_sig.Reset(6);
	nanite_draw_indirect_command_sig[0].ConstantBufferView(0);
	nanite_draw_indirect_command_sig[1].ConstantBufferView(1);
	nanite_draw_indirect_command_sig[2].ShaderResourceView(2);
	nanite_draw_indirect_command_sig[3].VertexBufferView(0);
	nanite_draw_indirect_command_sig[4].IndexBufferView();
	nanite_draw_indirect_command_sig[5].DrawIndexed();
	nanite_draw_indirect_command_sig.Finalize();

	nanite_draw_indirect_root_sig.Reset(3);
	nanite_draw_indirect_root_sig[0].InitAsConstantBuffer(0);
	nanite_draw_indirect_root_sig[1].InitAsConstantBuffer(1);
	nanite_draw_indirect_root_sig[2].InitAsBufferSRV(0);
	nanite_draw_indirect_root_sig.Finalize(L"DrawIndirectRootSig");

	D3D12_INPUT_ELEMENT_DESC pos_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DXGI_FORMAT vis_buffer_foramt = g_VisibilityBuffer.GetFormat();
	DXGI_FORMAT rasterization_depth_format = g_RasterizationDepth.GetFormat();

	{
		std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = init_desc.shader_compiler->Compile(L"Shaders/HardwareRasterization.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
		std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = init_desc.shader_compiler->Compile(L"Shaders/HardwareRasterization.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

		nanite_draw_indirect_pso = GraphicsPSO(L"DrawIndirectPSO");
		nanite_draw_indirect_pso.SetRootSignature(nanite_draw_indirect_root_sig);
		nanite_draw_indirect_pso.SetRasterizerState(RasterizerDefault);
		nanite_draw_indirect_pso.SetBlendState(BlendDisable);
		nanite_draw_indirect_pso.SetDepthStencilState(DepthStateReadWrite);
		nanite_draw_indirect_pso.SetInputLayout(_countof(pos_desc), pos_desc);
		nanite_draw_indirect_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		nanite_draw_indirect_pso.SetRenderTargetFormats(1, &vis_buffer_foramt, rasterization_depth_format);
		nanite_draw_indirect_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
		nanite_draw_indirect_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
		nanite_draw_indirect_pso.Finalize();
	}


	{
		std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = init_desc.shader_compiler->Compile(L"Shaders/HardwareRasterization.hlsl", L"depth_resolve_vs_main", L"vs_5_1", nullptr, 0);
		std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = init_desc.shader_compiler->Compile(L"Shaders/HardwareRasterization.hlsl", L"depth_resolve_ps_main", L"ps_5_1", nullptr, 0);

		depth_resolve_root_sig.Reset(2);
		depth_resolve_root_sig[0].InitAsConstantBuffer(0);
		depth_resolve_root_sig[1].InitAsBufferSRV(0);
		depth_resolve_root_sig.Finalize(L"depth_resolve_root_sig");
		
		depth_resolvepso = nanite_draw_indirect_pso;
		depth_resolvepso.SetInputLayout(0, nullptr);
		depth_resolvepso.SetRenderTargetFormats(0, nullptr, rasterization_depth_format);
		depth_resolvepso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
		depth_resolvepso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
		depth_resolvepso.Finalize();
	}
}

void CSimSoftwareRasterizer::Init(SNaniteRasterizerInitDesc& init_desc)
{
	CreatePSO(init_desc);
}

void CSimSoftwareRasterizer::Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context)
{
	//ComputeContext& cptContext = ComputeContext::Begin(L"Software Rasterization");
	//
	//StructuredBuffer* indirect_cmd = rasterization_context->m_software_indirect_draw_command;
	//
	//cptContext.TransitionResource(g_RasterizationDepthUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//cptContext.ClearUAV(g_RasterizationDepthUAV);
	//cptContext.ExecuteIndirect(nanite_dispatch_command_sig, *indirect_cmd, 0, 1024, &indirect_cmd->GetCounterBuffer(), indirect_cmd->GetBufferSize());
	//cptContext.Finish();
}

void CSimSoftwareRasterizer::CreatePSO(SNaniteRasterizerInitDesc& init_desc)
{
	nanite_dispatch_command_sig.Reset(8);
	nanite_dispatch_command_sig[0].ConstantBufferView(0);
	nanite_dispatch_command_sig[1].ConstantBufferView(1);
	nanite_dispatch_command_sig[2].ShaderResourceView(2);
	nanite_dispatch_command_sig[3].ShaderResourceView(3);
	nanite_dispatch_command_sig[4].ShaderResourceView(4);
	nanite_dispatch_command_sig[5].ShaderResourceView(5);
	nanite_dispatch_command_sig[6].UnorderedAccessView(6);
	nanite_dispatch_command_sig[7].UnorderedAccessView(7);
	nanite_dispatch_command_sig.Finalize();
	
	nanite_dispatchIndirectRootSig.Reset(8);
	nanite_dispatchIndirectRootSig[0].InitAsConstantBuffer(0);
	nanite_dispatchIndirectRootSig[1].InitAsConstantBuffer(1);
	nanite_dispatchIndirectRootSig[2].InitAsBufferSRV(0);
	nanite_dispatchIndirectRootSig[3].InitAsBufferSRV(1);
	nanite_dispatchIndirectRootSig[4].InitAsBufferSRV(2);
	nanite_dispatchIndirectRootSig[5].InitAsBufferSRV(3);
	nanite_dispatchIndirectRootSig[6].InitAsBufferUAV(0);
	nanite_dispatchIndirectRootSig[7].InitAsBufferUAV(1);
	nanite_dispatchIndirectRootSig.Finalize(L"nanite_dispatchIndirectRootSig");

	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = init_desc.shader_compiler->Compile(L"Shaders/InstanceCulling.hlsl", L"SoftwareRasterization", L"cs_5_1", nullptr, 0);
	nanite_dispatch_indirect_pso.SetRootSignature(nanite_dispatchIndirectRootSig);
	nanite_dispatch_indirect_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	nanite_dispatch_indirect_pso.Finalize();
}

void CSimNaniteRasterizer::Init(SNaniteRasterizerInitDesc& init_desc)
{
	m_hardware_rasterizer.Init(init_desc);
	m_software_rasterizer.Init(init_desc);
}

void CSimNaniteRasterizer::Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context)
{
	m_software_rasterizer.Rasterizer(Context,rasterization_context);
	m_hardware_rasterizer.Rasterizer(Context,rasterization_context);
}
