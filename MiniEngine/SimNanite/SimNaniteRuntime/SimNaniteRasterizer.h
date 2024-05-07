#pragma once
#include "SimNaniteCullCommon.h"

struct SNaniteRasterizerInitDesc
{
	CShaderCompiler* shader_compiler;
};

class CSimHardwareRasterizer
{
public:
	void Init(SNaniteRasterizerInitDesc& init_desc);
	void Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context);
private:
	void CreatePSO(SNaniteRasterizerInitDesc& init_desc);

	RootSignature nanite_draw_indirect_root_sig;
	CommandSignature nanite_draw_indirect_command_sig;
	GraphicsPSO nanite_draw_indirect_pso;

	RootSignature depth_resolve_root_sig;
	GraphicsPSO depth_resolvepso;
};

class CSimSoftwareRasterizer
{
public:
	void Init(SNaniteRasterizerInitDesc& init_desc);
	void Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context);
private:
	void CreatePSO(SNaniteRasterizerInitDesc& init_desc);

	RootSignature nanite_dispatchIndirectRootSig;
	CommandSignature nanite_dispatch_command_sig;
	ComputePSO nanite_dispatch_indirect_pso;
};


class CSimNaniteRasterizer
{
public:
	void Init(SNaniteRasterizerInitDesc& init_desc);
	void Rasterizer(GraphicsContext& Context, SNaniteRasterizationContext* rasterization_context);
private:
	CSimHardwareRasterizer m_hardware_rasterizer;
	CSimSoftwareRasterizer m_software_rasterizer;
};