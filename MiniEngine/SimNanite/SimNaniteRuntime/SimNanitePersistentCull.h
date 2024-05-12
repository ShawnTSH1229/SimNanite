#pragma once
#include "SimNaniteCullCommon.h"
#include "SimNaniteInstanceCull.h"

struct SNanitePersistentCullInitDesc
{
    CShaderCompiler* shader_compiler;
};

__declspec(align(256)) struct SPersistentCullParameter
{
    Math::BoundingPlane planes[6];
    DirectX::XMFLOAT3 camera_world_pos;
    uint64_t global_constant_gpu_address;
};


__declspec(align(256))  struct SoftwareIndirectCmdGen
{
    uint64_t rasterize_parameters_gpu_address;
};

class CPersistentCullPass
{
public:
	void Init(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context);
    void GPUCull(ComputeContext& Context, SNaniteCullingContext* nanite_cull_context);
    void UpdataCullingParameters(const Frustum& frustum, DirectX::XMFLOAT3 camera_world_pos, SNaniteCullingContext* nanite_cull_context);
private:
    void InitBuffer(SNaniteCullInitDesc& nanite_cull_init_desc, SNaniteCullingContext* nanite_cull_context);
    void BuildScneneData(SNaniteCullInitDesc& nanite_cull_init_desc);

    void CreatePSO(CShaderCompiler& shader_compiler);

    RootSignature PersistentCullRootSig;
    ComputePSO PersistentCullPSO;
    
    RootSignature PersistentCullInitRootSig;
    ComputePSO PersistentCullInitPSO;

    RootSignature IndirectCmdGenRootSig;
    ComputePSO IndirectCmdGenPSO;


    std::vector<SSimNaniteMesh_deprecated>m_scene_mesh_infos_cpu;
    std::vector<SSimNaniteCluster_Deprecated>m_scene_cluster_infos_cpu;
    std::vector<SSimNaniteClusterGroup_deprecated> m_scene_cluster_group_infos_cpu;
    
    SPersistentCullParameter m_persistentCullParameter;

    //srv
    ByteAddressBuffer m_cluster_group_init_task_queue;
    StructuredBuffer m_scene_cluster_infos;
    StructuredBuffer m_scene_cluster_group_infos;
    StructuredBuffer m_scene_mesh_infos;
    
    //uav
    ByteAddressBuffer cluster_group_task_queue;
    ByteAddressBuffer cluster_task_queue;
    ByteAddressBuffer cluster_task_batch_size;
    StructuredBuffer m_queue_pass_state;

    StructuredBuffer m_hardware_indirect_draw_command;
    StructuredBuffer m_software_indirect_draw_cmd_param;

    StructuredBuffer m_software_dispatch_indirect_command;
};