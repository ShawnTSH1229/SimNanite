#pragma once
#include "SimNaniteGlobalResource.h"

class CPersistentCulling
{
public:
	void Init();
	void GPUCull(ComputeContext& Context);
private:

	// init persisitent cull
	RootSignature m_persistent_cull_init_sig;
	ComputePSO m_persistent_cull_init_pso;

	// generate indirect dispatch
	RootSignature gen_indirect_dispacth_sig;
	ComputePSO gen_indirect_dispacth_pso;
	StructuredBuffer persistent_cull_indirect_cmd;

	// persistent cull
	RootSignature persistent_cull_sig;
	ComputePSO persistent_cull_pso;

	ByteAddressBuffer m_node_task_queue;

	ByteAddressBuffer m_cluster_task_queue;
	ByteAddressBuffer m_cluster_task_batch_size;

	// hardware indirect dispatch
	RootSignature hardware_indirect_root_sig;
	ComputePSO hardware_indirect_pso;
};