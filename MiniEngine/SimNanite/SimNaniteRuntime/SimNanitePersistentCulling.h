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
};