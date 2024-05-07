#include "../SimNaniteCommon/NaniteUtils.h"
#include "SimNaniteBuilder.h"
#include "meshoptimizer.h"
#include <fstream>
#include <iostream>
#include <xhash>

void CSimNanitePartioner::AddEdge(uint32_t hashed_pos0, uint32_t hashed_pos1)
{
	if (m_vtx_index_map.find(hashed_pos0) == m_vtx_index_map.end())
	{
		m_vertex_ajacent_vertices.push_back(SAjacentVert());
		m_vtx_index_map[hashed_pos0] = m_vertex_ajacent_vertices.size() - 1;
	}

	if (m_vtx_index_map.find(hashed_pos1) == m_vtx_index_map.end())
	{
		m_vertex_ajacent_vertices.push_back(SAjacentVert());
		m_vtx_index_map[hashed_pos1] = m_vertex_ajacent_vertices.size() - 1;
	}

	m_vertex_ajacent_vertices[m_vtx_index_map[hashed_pos1]].m_ajacent_vertices.insert(m_vtx_index_map[hashed_pos0]);
	m_vertex_ajacent_vertices[m_vtx_index_map[hashed_pos0]].m_ajacent_vertices.insert(m_vtx_index_map[hashed_pos1]);
}

void CSimNanitePartioner::PartionTriangles(const SBuildCluster& cluster_to_partition, const std::unordered_map<uint32_t, SGroupIndex>& vtx_group_map, std::vector<SBuildCluster>& out_clusters)
{
	m_vertex_ajacent_vertices.reserve(cluster_to_partition.m_positions.size() / 6);
	for (int index = 0; index < cluster_to_partition.m_positions.size(); index += 3)
	{
		DirectX::XMFLOAT3 vtx_a = cluster_to_partition.m_positions[index + 0];
		DirectX::XMFLOAT3 vtx_b = cluster_to_partition.m_positions[index + 1];
		DirectX::XMFLOAT3 vtx_c = cluster_to_partition.m_positions[index + 2];

		uint32_t hashed_pos0 = HashPosition(vtx_a);
		uint32_t hashed_pos1 = HashPosition(vtx_b);
		uint32_t hashed_pos2 = HashPosition(vtx_c);

		AddEdge(hashed_pos0, hashed_pos1);
		AddEdge(hashed_pos1, hashed_pos2);
		AddEdge(hashed_pos2, hashed_pos0);
	}

	std::vector<idx_t> vtx_ajacent_offsets;
	std::vector<idx_t> vtx_ajacents;
	vtx_ajacent_offsets.reserve(m_vertex_ajacent_vertices.size());
	vtx_ajacents.reserve(vtx_ajacent_offsets.size() * 6);

	for (int idx = 0; idx < m_vertex_ajacent_vertices.size(); idx++)
	{
		vtx_ajacent_offsets.push_back(vtx_ajacents.size());
		for (const auto& vtx : m_vertex_ajacent_vertices[idx].m_ajacent_vertices)
		{
			vtx_ajacents.push_back(vtx);
		}
	}
	vtx_ajacent_offsets.push_back(vtx_ajacents.size());

	idx_t vtx_count = vtx_ajacent_offsets.size() - 1;

	idx_t objval = 0;
	idx_t n_weight = 1;
	idx_t n_part = (vtx_count + 127) / 128;

	std::vector<idx_t> part_result;
	part_result.resize(vtx_count);

	idx_t options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_UFACTOR] = 200;

	int r = METIS_PartGraphKway(&vtx_count, &n_weight, vtx_ajacent_offsets.data(), vtx_ajacents.data(), nullptr, nullptr, nullptr, &n_part, nullptr, nullptr, nullptr, &objval, part_result.data());
	if (r != METIS_OK)
	{
		return;
	}

	out_clusters.resize(n_part);

	for (idx_t cluster_idx = 0; cluster_idx < out_clusters.size(); cluster_idx++)
	{
		out_clusters[cluster_idx].m_positions.reserve(128 * 3);
		out_clusters[cluster_idx].m_normals.reserve(128 * 3);
		out_clusters[cluster_idx].m_uvs.reserve(128 * 3);
	}

	for (int index = 0; index < cluster_to_partition.m_positions.size(); index += 3)
	{
		DirectX::XMFLOAT3 vtx_a = cluster_to_partition.m_positions[index + 0];
		DirectX::XMFLOAT3 vtx_b = cluster_to_partition.m_positions[index + 1];
		DirectX::XMFLOAT3 vtx_c = cluster_to_partition.m_positions[index + 2];

		uint32_t hashed_pos_a = HashPosition(vtx_a);
		uint32_t hashed_pos_b = HashPosition(vtx_b);
		uint32_t hashed_pos_c = HashPosition(vtx_c);

		uint32_t vtx_idx_a = m_vtx_index_map[hashed_pos_a];
		uint32_t vtx_idx_b = m_vtx_index_map[hashed_pos_b];
		uint32_t vtx_idx_c = m_vtx_index_map[hashed_pos_c];

		idx_t part_a = part_result[vtx_idx_a];
		idx_t part_b = part_result[vtx_idx_b];
		idx_t part_c = part_result[vtx_idx_c];

		idx_t tri_parts[3];
		tri_parts[0] = part_a;

		int triangle_part_num = 1;
		if (part_b != part_a)
		{
			tri_parts[triangle_part_num] = part_b;
			triangle_part_num++;
		}

		if ((part_c != part_a) && (part_c != part_b))
		{
			tri_parts[triangle_part_num] = part_c;
			triangle_part_num++;
		}

		if (triangle_part_num == 2)
		{
			out_clusters[tri_parts[0]].m_linked_cluster.insert(tri_parts[1]);
			out_clusters[tri_parts[1]].m_linked_cluster.insert(tri_parts[0]);
		}
		else if (triangle_part_num == 3)
		{
			out_clusters[tri_parts[0]].m_linked_cluster.insert(tri_parts[1]);
			out_clusters[tri_parts[0]].m_linked_cluster.insert(tri_parts[2]);

			out_clusters[tri_parts[1]].m_linked_cluster.insert(tri_parts[0]);
			out_clusters[tri_parts[1]].m_linked_cluster.insert(tri_parts[2]);

			out_clusters[tri_parts[2]].m_linked_cluster.insert(tri_parts[0]);
			out_clusters[tri_parts[2]].m_linked_cluster.insert(tri_parts[1]);
		}

		// avoid overlap
		idx_t min_part = (std::min)((std::min)(part_a, part_b), part_c);

		out_clusters[min_part].m_positions.push_back(vtx_a);
		out_clusters[min_part].m_positions.push_back(vtx_b);
		out_clusters[min_part].m_positions.push_back(vtx_c);

		DirectX::XMFLOAT3& min_pos = out_clusters[min_part].m_min_pos;
		min_pos = DirectX::XMFLOAT3((std::min)(min_pos.x, vtx_a.x), (std::min)(min_pos.y, vtx_a.y), (std::min)(min_pos.z, vtx_a.z));
		min_pos = DirectX::XMFLOAT3((std::min)(min_pos.x, vtx_b.x), (std::min)(min_pos.y, vtx_b.y), (std::min)(min_pos.z, vtx_b.z));
		min_pos = DirectX::XMFLOAT3((std::min)(min_pos.x, vtx_c.x), (std::min)(min_pos.y, vtx_c.y), (std::min)(min_pos.z, vtx_c.z));

		DirectX::XMFLOAT3& max_pos = out_clusters[min_part].m_max_pos;
		max_pos = DirectX::XMFLOAT3((std::max)(max_pos.x, vtx_a.x), (std::max)(max_pos.y, vtx_a.y), (std::max)(max_pos.z, vtx_a.z));
		max_pos = DirectX::XMFLOAT3((std::max)(max_pos.x, vtx_b.x), (std::max)(max_pos.y, vtx_b.y), (std::max)(max_pos.z, vtx_b.z));
		max_pos = DirectX::XMFLOAT3((std::max)(max_pos.x, vtx_c.x), (std::max)(max_pos.y, vtx_c.y), (std::max)(max_pos.z, vtx_c.z));

		out_clusters[min_part].m_normals.push_back(cluster_to_partition.m_normals[index + 0]);
		out_clusters[min_part].m_normals.push_back(cluster_to_partition.m_normals[index + 1]);
		out_clusters[min_part].m_normals.push_back(cluster_to_partition.m_normals[index + 2]);

		out_clusters[min_part].m_uvs.push_back(cluster_to_partition.m_uvs[index + 0]);
		out_clusters[min_part].m_uvs.push_back(cluster_to_partition.m_uvs[index + 1]);
		out_clusters[min_part].m_uvs.push_back(cluster_to_partition.m_uvs[index + 2]);



		if (vtx_group_map.size() != 0)
		{
			const SGroupIndex group_index_a = vtx_group_map.find(hashed_pos_a)->second;
			const SGroupIndex group_index_b = vtx_group_map.find(hashed_pos_b)->second;
			const SGroupIndex group_index_c = vtx_group_map.find(hashed_pos_c)->second;

			for (auto iter_a = group_index_a.m_group_idex.begin(); iter_a != group_index_a.m_group_idex.end(); iter_a++)
			{
				const int grp_idx = *iter_a;
				bool b_find_b = false;
				bool b_find_c = false;
				for (auto iter_b = group_index_b.m_group_idex.begin(); iter_b != group_index_b.m_group_idex.end(); iter_b++)
				{
					if (grp_idx == *iter_b)
					{
						b_find_b = true;
						break;
					}
				}

				for (auto iter_c = group_index_c.m_group_idex.begin(); iter_c != group_index_c.m_group_idex.end(); iter_c++)
				{
					if (grp_idx == *iter_c)
					{
						b_find_c = true;
						break;
					}
				}

				if (b_find_b && b_find_c)
				{
					out_clusters[min_part].face_parent_cluster_group.push_back(grp_idx);
					break;
				}
			}
		}
	}

	for (auto& clu : out_clusters)
	{
		DirectX::XMFLOAT3 center = DirectX::XMFLOAT3((clu.m_min_pos.x + clu.m_max_pos.x) * 0.5, (clu.m_min_pos.y + clu.m_max_pos.y) * 0.5, (clu.m_min_pos.z + clu.m_max_pos.z) * 0.5);
		DirectX::XMFLOAT3 extents = DirectX::XMFLOAT3(clu.m_max_pos.x - center.x, clu.m_max_pos.y - center.y, clu.m_max_pos.z - center.z);
		clu.m_bounding_box = DirectX::BoundingBox(center, extents);
	}

	m_vtx_index_map.clear();
	m_vertex_ajacent_vertices.clear();
}

void CSimNanitePartioner::PartionClusters(std::vector<SBuildCluster>& m_clusters, std::vector<SBuildClusterGroup>& out_cluster_group, std::unordered_map<uint32_t, SGroupIndex>& out_vtx_group_map)
{
	std::vector<idx_t> cluster_ajacent_offsets;
	std::vector<idx_t> cluster_ajacents;

	for (int idx = 0; idx < m_clusters.size(); idx++)
	{
		cluster_ajacent_offsets.push_back(cluster_ajacents.size());
		for (const auto& clt : m_clusters[idx].m_linked_cluster)
		{
			cluster_ajacents.push_back(clt);
		}
	}
	cluster_ajacent_offsets.push_back(cluster_ajacents.size());

	idx_t n_clu_weight = 1;
	idx_t n_clu_part = 8;
	idx_t clu_obj_val;

	idx_t clu_count = cluster_ajacent_offsets.size() - 1;
	std::vector<idx_t> clu_part_result;
	clu_part_result.resize(clu_count);

	idx_t options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_UFACTOR] = 200;

	int r = METIS_PartGraphKway(&clu_count, &n_clu_weight, cluster_ajacent_offsets.data(), cluster_ajacents.data(), nullptr, nullptr, nullptr, &n_clu_part, nullptr, nullptr, nullptr, &clu_obj_val, clu_part_result.data());

	if (r != METIS_OK)
	{
		return;
	}

	out_cluster_group.resize(n_clu_part);
	for (int idx = 0; idx < m_clusters.size(); idx++)
	{
		int group_idx = clu_part_result[idx];
		out_cluster_group[group_idx].m_cluster_indices.push_back(idx);

		SBuildCluster& cluster = m_clusters[idx];
		for (int vtx_idx = 0; vtx_idx < cluster.m_positions.size(); vtx_idx += 3)
		{
			DirectX::XMFLOAT3 vtx_a = cluster.m_positions[vtx_idx + 0];
			DirectX::XMFLOAT3 vtx_b = cluster.m_positions[vtx_idx + 1];
			DirectX::XMFLOAT3 vtx_c = cluster.m_positions[vtx_idx + 2];

			uint32_t hashed_pos_a = HashPosition(vtx_a);
			uint32_t hashed_pos_b = HashPosition(vtx_b);
			uint32_t hashed_pos_c = HashPosition(vtx_c);

			out_vtx_group_map[hashed_pos_a].m_group_idex.insert(group_idx);
			out_vtx_group_map[hashed_pos_b].m_group_idex.insert(group_idx);
			out_vtx_group_map[hashed_pos_c].m_group_idex.insert(group_idx);

		}
	}
}
