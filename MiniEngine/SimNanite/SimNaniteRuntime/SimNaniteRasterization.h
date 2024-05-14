#pragma once
#include "SimNaniteGlobalResource.h"

class CRasterizationPass
{
public:
	void Init();
	void Rasterization();
private:
	CommandSignature hardware_indirect_cmd_sig;
	RootSignature m_hardware_indirect_draw_sig;
	GraphicsPSO m_hardware_indirect_draw_pso;
};