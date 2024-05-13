#pragma once
#include <stdint.h>
#include <vector>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>

struct SClusterGroupBVHNode
{
	bool m_is_leaf_node;
	uint32_t m_cluster_group_index;
	uint32_t m_child_node_index[4];
	DirectX::BoundingBox m_bouding_box;
};

struct CSimNaniteClusterResource
{
	DirectX::BoundingBox m_bouding_box;
	
	int m_index_count;
	int m_start_index_location;
	int m_start_vertex_location;
};

struct CSimNaniteClusterGrpupResource
{
	DirectX::BoundingBox m_bouding_box;
	float cluster_pre_lod_dist;
	float cluster_next_lod_dist;

	int m_cluster_num;
	std::vector<int>m_clusters_indices;

	typedef void (*group_serialize_fun)(char*, long long, void* streaming);
	void Serialize(group_serialize_fun func, void* streaming);
};

struct CSimNaniteLodResource
{
	int m_cluster_group_num;
	int m_cluster_group_start;
	uint32_t m_root_node_index;
};

struct SSimNaniteMeshHeader
{
	int m_vertex_count;
	int m_index_count;
	int m_bvh_nodes_size;
	int m_cluster_group_size;
	int m_cluster_size;
	int m_lod_size;
};

class CSimNaniteMeshResource
{
public:
	SSimNaniteMeshHeader m_header;

	DirectX::BoundingBox m_bouding_box;
	
	std::vector<DirectX::XMFLOAT3> m_positions;
	std::vector<DirectX::XMFLOAT3> m_normals;
	std::vector<DirectX::XMFLOAT2> m_uvs;
	std::vector<unsigned int> m_indices;

	std::vector<SClusterGroupBVHNode> m_bvh_nodes;
	std::vector<CSimNaniteClusterGrpupResource> m_cluster_groups;
	std::vector<CSimNaniteClusterResource>m_clusters;
	std::vector<CSimNaniteLodResource> m_nanite_lods;

	void LoadFrom(const std::string& source_file_an, const std::wstring& source_file, bool bforce_rebuild = false);
};

