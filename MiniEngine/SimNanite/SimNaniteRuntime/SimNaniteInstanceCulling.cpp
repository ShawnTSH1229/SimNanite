#include "SimNaniteInstanceCulling.h"

void CSimNaniteInstanceCulling::Init()
{
	m_inst_cull_root_sig.Reset(3);
	m_inst_cull_root_sig[0].InitAsConstantBuffer(0);
	m_inst_cull_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
	m_inst_cull_root_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_inst_cull_root_sig.Finalize(L"InstanceCull");

	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNaniteInstanceCull.hlsl", L"CSMain", L"cs_5_1", nullptr, 0);
	m_inst_cull_pso.SetRootSignature(m_inst_cull_root_sig);
	m_inst_cull_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	m_inst_cull_pso.Finalize();
}

void CSimNaniteInstanceCulling::Culling(ComputeContext& Context)
{
	
	Context.SetRootSignature(m_inst_cull_root_sig);
	Context.SetPipelineState(m_inst_cull_pso);
	Context.SetDynamicConstantBufferView(0, sizeof(SCullingParameters), &GetSimNaniteGlobalResource().m_culling_parameters);

	Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	Context.TransitionResource(GetSimNaniteGlobalResource().m_culled_instance_num, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	Context.ClearUAV(GetSimNaniteGlobalResource().m_culled_instance_num);

	Context.SetDynamicDescriptors(1, 0, 1, &GetSimNaniteGlobalResource().m_culled_ins_scene_data_gpu.GetUAV());
	Context.SetDynamicDescriptors(1, 1, 1, &GetSimNaniteGlobalResource().m_culled_instance_num.GetUAV());
	Context.SetDynamicDescriptors(1, 2, 1, &GetSimNaniteGlobalResource().m_queue_pass_state.GetUAV());

	Context.TransitionResource(GetSimNaniteGlobalResource().m_inst_scene_data_gpu, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	Context.SetDynamicDescriptors(2, 0, 1, &GetSimNaniteGlobalResource().m_inst_scene_data_gpu.GetSRV());

	Context.Dispatch((GetSimNaniteGlobalResource().m_culling_parameters.m_total_instance_num + 64 - 1) / 64, 1, 1);
}
