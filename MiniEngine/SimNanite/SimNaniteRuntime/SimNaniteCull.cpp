#include "SimNaniteCull.h"
using namespace Graphics;

void CNaniteCuller::Init(SNaniteCullInitDesc& nanite_cull_init_desc)
{
	m_instance_cull_pass.Init(nanite_cull_init_desc, &m_nanite_cull_context);
	m_persistent_cull_pass.Init(nanite_cull_init_desc, &m_nanite_cull_context);
}

void CNaniteCuller::UpdataCullingParameters(const Frustum& frustum, DirectX::XMFLOAT3 camera_world_pos)
{
	m_instance_cull_pass.UpdataCullingParameters(frustum);
	m_persistent_cull_pass.UpdataCullingParameters(frustum, camera_world_pos, &m_nanite_cull_context);
}

void CNaniteCuller::GPUCull(ComputeContext& Context, uint64_t gloabl_view_constant_gpu_address)
{
	ScopedTimer _prof(L"GPU Cull", Context);
	m_nanite_cull_context.mesh_global_constant_gpu_address = gloabl_view_constant_gpu_address;
	m_instance_cull_pass.GPUCull(Context, &m_nanite_cull_context);
	m_persistent_cull_pass.GPUCull(Context, &m_nanite_cull_context);
}

