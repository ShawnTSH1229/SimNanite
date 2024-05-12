#include "SimNanitePersistentCulling.h"

void CPersistentCulling::Init()
{
	{
		m_persistent_cull_init_sig.Reset(2);
		m_persistent_cull_init_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
		m_persistent_cull_init_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		m_persistent_cull_init_sig.Finalize(L"PersistentCullInit");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetSimNaniteGlobalResource().m_shader_compiler.Compile(L"Shaders/SimNanitePersistentCullInit.hlsl", L"InitPersistentCull", L"cs_5_1", nullptr, 0);
		m_persistent_cull_init_pso.SetRootSignature(m_persistent_cull_init_sig);
		m_persistent_cull_init_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_persistent_cull_init_pso.Finalize();
	}
}

void CPersistentCulling::GPUCull(ComputeContext& Context)
{
}
