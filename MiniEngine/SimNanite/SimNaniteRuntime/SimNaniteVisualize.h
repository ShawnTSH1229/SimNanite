#pragma once
#include "SimNaniteVisualize.h"
#include "SimNaniteCullCommon.h"
#include "SimNaniteGlobalResource.h"

__declspec(align(256)) struct SNaniteMeshClusterVisualize
{
	Math::Vector3 m_visualize_color;
};

struct SSimNaniteMeshLastLOD
{
	uint32_t cluster_group_num;
	uint32_t cluster_group_start_index;
};

// viualize cluster: 7  
// viualize instance cull: 8
// key 5: increase lod level, 2: decrease lod level
class CSimNaniteVisualizer
{
public:
	void Init();
	void Render();
private:
	void UpdateFreezingData();

	// visualize mesh cluster
	void RenderClusterVisualize();
	RootSignature m_vis_cluster_root_sig;
	GraphicsPSO m_vis_cluster_pso;
	
	// visualize instance culling
	
	void RenderInstanceCullingVisualize();

	CommandSignature m_vis_inst_cull_cmd_sig;

	RootSignature m_vis_ins_cull_root_sig;
	GraphicsPSO m_vis_ins_cull_pso;

	RootSignature m_gen_vis_ins_cull_root_sig;
	ComputePSO m_gen_vis_ins_cull_pso;

	RootSignature m_gen_vis_ins_cull_cmd_root_sig;
	ComputePSO m_gen_vis_ins_cull_cmd_pso;

	StructuredBuffer vis_instance_cull_mesh_last_lod;
	StructuredBuffer vis_instance_cull_cluster_draw;
	ByteAddressBuffer vis_instance_cull_cmd_num;
	StructuredBuffer vis_instance_cull_indirect_draw_cmd;

	// visualize node culling
#if CLUGROUP_CULL_DEBUG
	RootSignature m_gen_vis_node_cull_root_sig;
	ComputePSO m_gen_vis_node_cull_pso;
	CommandSignature m_vis_node_cull_cmd_sig;

	StructuredBuffer vis_clu_group_culled_cluster;
	ByteAddressBuffer vis_cul_group_cmd_num;
	StructuredBuffer vis_node_cull_indirect_draw_cmd;

	void RenderNodeCullingVisualize();
#endif
};