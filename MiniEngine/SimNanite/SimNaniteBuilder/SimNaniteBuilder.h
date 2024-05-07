#pragma once
#include "../SimNaniteCommon/MeshResource.h"
#include "metis.h"
#include <set>
#include <string>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <VectorMath.h>
#include <Math/SimpleMath.h>

// The numbering of the nodes starts from 1
struct SAjacentVert
{
	std::set<idx_t> m_ajacent_vertices;
};

struct SBuildCluster
{
	std::vector<DirectX::XMFLOAT3> m_positions;
	std::vector<DirectX::XMFLOAT3> m_normals;
	std::vector<DirectX::XMFLOAT2> m_uvs;
	
	DirectX::BoundingBox m_bounding_box;
	DirectX::XMFLOAT3 m_min_pos = DirectX::XMFLOAT3( 1e30f,  1e30f,  1e30f);
	DirectX::XMFLOAT3 m_max_pos = DirectX::XMFLOAT3(-1e30f, -1e30f, -1e30f);

	std::vector<int> face_parent_cluster_group; //size == m_positions.size / 3
	std::set<int> m_linked_cluster;
};

struct SGroupIndex
{
	std::set<int> m_group_idex;
};

struct SBuildClusterGroup
{
	std::vector<int> m_cluster_indices;
	DirectX::BoundingBox m_group_bounding_box;
};

struct SBuildClusterLevel
{
	std::vector<SBuildClusterGroup> m_cluster_groups;
	std::vector<SBuildCluster> m_clusters;
	std::unordered_map<uint32_t, SGroupIndex> m_vtx_to_last_level_group_map;
	SBuildCluster m_merged_cluster; // merged cluster, used to mesh simplify
};

class CSimNaniteMeshSimplifier
{
public:
	bool SimplifiyMergedClusterGroup(SBuildCluster& merged_cluster, SBuildCluster& simplified_cluster);
};



class CSimNanitePartioner
{
public:
	void PartionTriangles(const SBuildCluster& cluster_to_partition, const std::unordered_map<uint32_t, SGroupIndex>& vtx_group_map, std::vector<SBuildCluster>& out_clusters);
	void PartionClusters(std::vector<SBuildCluster>& m_clusters_ptr,std::vector<SBuildClusterGroup>& out_cluster_group, std::unordered_map<uint32_t, SGroupIndex>& out_vtx_group_map);
private:
	void AddEdge(uint32_t hashed_pos0, uint32_t hashed_pos1);
	std::unordered_map<uint32_t, uint32_t> m_vtx_index_map;
	std::vector<SAjacentVert>m_vertex_ajacent_vertices;
};

class CSimNaniteBuilder
{
public:
	void Build(SBuildCluster& total_mesh_cluster, CSimNaniteMeshResource& out_nanite_reousource);
private:
	void BuildDAG(CSimNaniteMeshResource& out_nanite_reousource);

	std::vector<SBuildClusterLevel> m_cluster_levels;
	SBuildClusterGroup m_init_cluster_groups;
	
	void SaveClusterToObj(SBuildCluster& cluster,const std::string cluster_obj_name);
};