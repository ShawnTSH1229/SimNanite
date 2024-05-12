#pragma once
#include "SimNaniteMeshInstance.h"
#include "SimNaniteInstanceCull.h"
#include "SimNanitePersistentCull.h"
#include "SimNaniteCullCommon.h"

class CNaniteCuller
{
public:
	void Init(SNaniteCullInitDesc& nanite_cull_init_desc);
	void UpdataCullingParameters(const Frustum& frustum, DirectX::XMFLOAT3 camera_world_pos);
	void GPUCull(ComputeContext& Context, uint64_t gloabl_view_constant_gpu_address);
private:
	CInstanceCullPass_Deprecated m_instance_cull_pass;
	CPersistentCullPass m_persistent_cull_pass;
	SNaniteCullingContext m_nanite_cull_context;
};