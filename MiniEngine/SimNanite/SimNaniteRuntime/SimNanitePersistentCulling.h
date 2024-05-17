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

	// left cluster group culling
	RootSignature left_clu_group_sig;
	ComputePSO left_clu_group_pso;

	// cluster cull
	RootSignature cluster_cull_sig;
	ComputePSO cluster_cull_pso;

	// hardware indirect dispatch
	RootSignature indirect_cmd_gen_sig;
	ComputePSO indirect_cmd_gen_pso;
};