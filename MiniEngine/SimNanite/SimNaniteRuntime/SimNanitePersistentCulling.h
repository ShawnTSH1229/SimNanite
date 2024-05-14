#pragma once
#include "SimNaniteGlobalResource.h"

class CPersistentCulling
{
public:
	void Init();
	void GPUCull(ComputeContext& Context);
private:

	RootSignature m_persistent_cull_init_sig;
	ComputePSO m_persistent_cull_init_pso;

	RootSignature gen_indirect_dispacth_sig;
	ComputePSO gen_indirect_dispacth_pso;
	StructuredBuffer persistent_cull_indirect_cmd;

	RootSignature persistent_cull_sig;
	ComputePSO persistent_cull_pso;

	ByteAddressBuffer m_node_task_queue;

};