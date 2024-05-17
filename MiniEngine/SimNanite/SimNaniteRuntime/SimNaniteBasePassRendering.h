#pragma once
#include "SimNaniteGlobalResource.h"

class CBasePassRendering
{
public:
	void Init();
	void Rendering();
private:
	RootSignature m_resolve_material_idx_sig;
	GraphicsPSO m_resolve_material_idx_pso;

	RootSignature m_basepass_sig;
	GraphicsPSO m_basepass_pso;
};