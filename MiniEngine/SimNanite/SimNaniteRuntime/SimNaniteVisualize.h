#pragma once
#include "SimNaniteVisualize.h"
#include "SimNaniteCullCommon.h"
#include "SimNaniteGlobalResource.h"

__declspec(align(256)) struct SNaniteMeshClusterVisualize
{
	Math::Vector3 m_visualize_color;
};

class CSimNaniteVisualizer
{
public:
	void Init();
	void Render();
private:
	RootSignature m_vis_cluster_root_sig;
	GraphicsPSO m_vis_cluster_pso;
};