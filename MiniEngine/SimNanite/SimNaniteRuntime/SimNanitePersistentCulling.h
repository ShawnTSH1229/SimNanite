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

	ByteAddressBuffer cluster_group_init_task_queue;

	RootSignature persistent_cull_sig;
	ComputePSO persistent_cull_pso;

	ByteAddressBuffer cluster_group_task_queue;

};