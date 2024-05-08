#include "../SimNaniteCommon/NaniteUtils.h"
#include "SimNaniteBuilder.h"
#include "meshoptimizer.h"
#include <fstream>
#include <iostream>
#include <xhash>

void CSimNaniteBuilder::Build(SBuildCluster& total_mesh_cluster, CSimNaniteMeshResource& out_nanite_reousource)
{
	m_cluster_levels.resize(4096);
	m_cluster_levels[0].m_merged_cluster = total_mesh_cluster;

	{
		DirectX::XMFLOAT3 mesh_min_pos = DirectX::XMFLOAT3(1e30f, 1e30f, 1e30f);
		DirectX::XMFLOAT3 mesh_max_pos = DirectX::XMFLOAT3(-1e30f, -1e30f, -1e30f);
		for (DirectX::XMFLOAT3 pos : total_mesh_cluster.m_positions)
		{
			mesh_min_pos = DirectX::XMFLOAT3((std::min)(mesh_min_pos.x, pos.x), (std::min)(mesh_min_pos.y, pos.y), (std::min)(mesh_min_pos.z, pos.z));
			mesh_max_pos = DirectX::XMFLOAT3((std::max)(mesh_max_pos.x, pos.x), (std::max)(mesh_max_pos.y, pos.y), (std::max)(mesh_max_pos.z, pos.z));
		}

		{
			DirectX::XMFLOAT3 center = DirectX::XMFLOAT3((mesh_min_pos.x + mesh_max_pos.x) * 0.5, (mesh_min_pos.y + mesh_max_pos.y) * 0.5, (mesh_min_pos.z + mesh_max_pos.z) * 0.5);
			DirectX::XMFLOAT3 extents = DirectX::XMFLOAT3(mesh_max_pos.x - center.x, mesh_max_pos.y - center.y, mesh_max_pos.z - center.z);
			out_nanite_reousource.m_bouding_box = DirectX::BoundingBox(center, extents);
		}
	}

	{
		int clu_level_idx = 0;
		while (m_cluster_levels[clu_level_idx].m_merged_cluster.m_positions.size() > 128 * 3 * 8 + 10)
		{
			SBuildClusterLevel& current_cluster_level = m_cluster_levels[clu_level_idx];
			SBuildCluster& last_level_merged_cluster = current_cluster_level.m_merged_cluster;

			CSimNanitePartitioner nanite_partitioner;
			nanite_partitioner.PartionTriangles(last_level_merged_cluster, m_cluster_levels[clu_level_idx].m_vtx_to_last_level_group_map, current_cluster_level.m_clusters);
			nanite_partitioner.PartionClusters(current_cluster_level.m_clusters, current_cluster_level.m_cluster_groups, m_cluster_levels[clu_level_idx + 1].m_vtx_to_last_level_group_map);

			CSimNaniteMeshSimplifier nanite_mesh_simplifier;

			bool simplify_failed = nanite_mesh_simplifier.SimplifiyMergedClusterGroup(m_cluster_levels[clu_level_idx].m_merged_cluster, m_cluster_levels[clu_level_idx + 1].m_merged_cluster);

			if (simplify_failed)
			{
				clu_level_idx -= 2;
				break;
			}

			bool b_terminated = current_cluster_level.m_clusters.size() < 24;
			if (b_terminated)
			{
				break;
			}

			clu_level_idx++;
		}

		{
			SBuildClusterLevel& current_cluster_level = m_cluster_levels[clu_level_idx];
			current_cluster_level.m_cluster_groups.resize(0);
			current_cluster_level.m_clusters.resize(0);
			SBuildCluster& last_level_merged_cluster = current_cluster_level.m_merged_cluster;

			CSimNanitePartitioner nanite_partioner;
			nanite_partioner.PartionTriangles(last_level_merged_cluster, current_cluster_level.m_vtx_to_last_level_group_map, current_cluster_level.m_clusters);

			SBuildClusterGroup cluster_group;
			for (int clu_idx = 0; clu_idx < current_cluster_level.m_clusters.size(); clu_idx++)
			{
				cluster_group.m_cluster_indices.push_back(clu_idx);
			}
			current_cluster_level.m_cluster_groups.push_back(cluster_group);
			clu_level_idx++;
		}

		m_cluster_levels.resize(clu_level_idx);
	}


	// Build DAG
	BuildDAG(out_nanite_reousource);

	out_nanite_reousource.m_header.m_index_count = out_nanite_reousource.m_indices.size();
	out_nanite_reousource.m_header.m_cluster_group_size = out_nanite_reousource.m_cluster_groups.size();
	out_nanite_reousource.m_header.m_cluster_size = out_nanite_reousource.m_clusters.size();
	out_nanite_reousource.m_header.m_lod_size = out_nanite_reousource.m_nanite_lods.size();
}

void CSimNaniteBuilder::SaveClusterToObj(SBuildCluster& cluster, const std::string cluster_obj_name)
{
	//std::string obj_file_name = "H:/SimNanite/MiniEngine/Build/obj_out/cluster_";
	//obj_file_name += std::to_string(idx);
	//obj_file_name += ".obj";
	
	std::ofstream out_obj(cluster_obj_name);
	for (int vtx_idx = 0; vtx_idx < cluster.m_positions.size(); vtx_idx ++)
	{
		out_obj << "v ";
		out_obj << std::to_string(cluster.m_positions[vtx_idx].x);
		out_obj << ' ';
		out_obj << std::to_string(cluster.m_positions[vtx_idx].y);
		out_obj << ' ';
		out_obj << std::to_string(cluster.m_positions[vtx_idx].z);
		out_obj << '\n';
	}

	for (int vtx_idx = 0; vtx_idx < cluster.m_uvs.size(); vtx_idx++)
	{
		out_obj << "vt ";
		out_obj << std::to_string(cluster.m_uvs[vtx_idx].x);
		out_obj << ' ';
		out_obj << std::to_string(cluster.m_uvs[vtx_idx].y);
		out_obj << '\n';
	}

	for (int vtx_idx = 0; vtx_idx < cluster.m_positions.size(); vtx_idx += 3)
	{
		out_obj << "f ";
		out_obj << std::to_string(vtx_idx + 1);
		out_obj << ' ';
		out_obj << std::to_string(vtx_idx + 2);
		out_obj << ' ';
		out_obj << std::to_string(vtx_idx + 3);
		out_obj << '\n';
	}
}



