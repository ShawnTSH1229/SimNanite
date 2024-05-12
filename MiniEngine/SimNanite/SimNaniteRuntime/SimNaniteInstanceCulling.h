#pragma once
#include "SimNaniteGlobalResource.h"

class CSimNaniteInstanceCulling
{
public:
	void Init();
	void Culling(ComputeContext& Context);
private:
	RootSignature m_inst_cull_root_sig;
	ComputePSO m_inst_cull_pso;
};