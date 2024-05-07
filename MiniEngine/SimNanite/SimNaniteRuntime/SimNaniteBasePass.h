#pragma once
#include "SimNaniteCullCommon.h"

struct CSimNaniteBasePassInitDesc
{
	CShaderCompiler* m_shader_compiler;
};

class CSimNaniteBasePass
{
public:
	void Init(CSimNaniteBasePassInitDesc* nanite_init_desc);
	void Render(SNaniteBasePassContext* context);
private:
	void CreatePSO(CSimNaniteBasePassInitDesc* nanite_init_desc);

	RootSignature m_depth_resolve_root_sig;
	GraphicsPSO m_depth_resolve_pso;

	RootSignature m_base_pass_root_sig;
	GraphicsPSO m_base_pass_pso;
};