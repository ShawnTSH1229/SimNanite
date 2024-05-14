#include "../SimNaniteCommon/NaniteUtils.h"
#include "SimNaniteBuilder.h"
#include "meshoptimizer.h"
using namespace DirectX::SimpleMath;

float ComputeBoundsScreenSize(const Vector3 bounds_origin, const float sphere_radius, const Vector3 view_origin, const Matrix proj_matrix)
{
	const float dist = Vector3::Distance(bounds_origin, view_origin);

	// Get projection multiple accounting for view scaling.
	const float ScreenMultiple = (std::max)(0.5f * proj_matrix.m[0][0], 0.5f * proj_matrix.m[1][1]);

	// Calculate screen-space projected radius
	const float ScreenRadius = ScreenMultiple * sphere_radius / (std::max)(1.0f, dist);

	// For clarity, we end up comparing the diameter
	return ScreenRadius * 2.0f;
}

void CSimNaniteBuilder::BuildDAG(CSimNaniteMeshResource& out_nanite_reousource)
{
	int total_vertex_count = 0;
	for (int clu_level_idx = 0; clu_level_idx < m_cluster_levels.size(); clu_level_idx++)
	{
		for (const auto& clu : m_cluster_levels[clu_level_idx].m_clusters)
		{
			total_vertex_count += clu.m_positions.size();
		}
	}

	out_nanite_reousource.m_positions.resize(total_vertex_count);
	out_nanite_reousource.m_normals.resize(total_vertex_count);
	out_nanite_reousource.m_uvs.resize(total_vertex_count);
	out_nanite_reousource.m_indices.resize(total_vertex_count);

	int start_index_location = 0;
	int start_vertex_location = 0;
	int start_cluster_location = 0;
	int start_cluster_group_location = 0;

	bool remap_vertex_buffer = true;

	for (int clu_level_idx = 0; clu_level_idx < m_cluster_levels.size(); clu_level_idx++)
	{
		SBuildClusterLevel& build_cluster_level = m_cluster_levels[clu_level_idx];

		// Build cluster
		for (int clu_idx = 0; clu_idx < m_cluster_levels[clu_level_idx].m_clusters.size(); clu_idx++)
		{
			const SBuildCluster& clu = m_cluster_levels[clu_level_idx].m_clusters[clu_idx];

			if (remap_vertex_buffer)
			{
				size_t index_count = clu.m_positions.size();
				size_t unindexed_vertex_count = clu.m_positions.size();
				std::vector<unsigned int> remap_index_buffer(index_count);

				size_t vertex_count = meshopt_generateVertexRemap(remap_index_buffer.data(), nullptr, index_count, clu.m_positions.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3));
				meshopt_remapVertexBuffer(out_nanite_reousource.m_positions.data() + start_vertex_location, clu.m_positions.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3), remap_index_buffer.data());
				meshopt_remapVertexBuffer(out_nanite_reousource.m_normals.data() + start_vertex_location, clu.m_normals.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3), remap_index_buffer.data());
				meshopt_remapVertexBuffer(out_nanite_reousource.m_uvs.data() + start_vertex_location, clu.m_uvs.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT2), remap_index_buffer.data());


				for (int index_idx = 0; index_idx < index_count; index_idx++)
				{
					out_nanite_reousource.m_indices[index_idx + start_index_location] = remap_index_buffer[index_idx];
				}

				CSimNaniteClusterResource cluster_resource;
				cluster_resource.m_bouding_box = clu.m_bounding_box;
				cluster_resource.m_index_count = index_count;
				cluster_resource.m_start_vertex_location = start_vertex_location;
				cluster_resource.m_start_index_location = start_index_location;
				out_nanite_reousource.m_clusters.push_back(cluster_resource);

				start_vertex_location += vertex_count;
				start_index_location += index_count;
			}
			else
			{
				size_t index_count = clu.m_positions.size();
				size_t vertex_count = clu.m_positions.size();
				
				CSimNaniteClusterResource cluster_resource;
				cluster_resource.m_bouding_box = clu.m_bounding_box;
				cluster_resource.m_index_count = index_count;
				cluster_resource.m_start_vertex_location = start_vertex_location;
				cluster_resource.m_start_index_location = start_index_location;
				out_nanite_reousource.m_clusters.push_back(cluster_resource);

				memcpy(out_nanite_reousource.m_positions.data() + start_vertex_location, clu.m_positions.data(), clu.m_positions.size() * sizeof(DirectX::XMFLOAT3));
				memcpy(out_nanite_reousource.m_normals.data() + start_vertex_location, clu.m_normals.data(), clu.m_normals.size() * sizeof(DirectX::XMFLOAT3));
				memcpy(out_nanite_reousource.m_uvs.data() + start_vertex_location, clu.m_uvs.data(), clu.m_uvs.size() * sizeof(DirectX::XMFLOAT2));

				for (int index_idx = 0; index_idx < index_count; index_idx++)
				{
					out_nanite_reousource.m_indices[index_idx + start_index_location] = index_idx;
				}

				start_vertex_location += vertex_count;
				start_index_location += index_count;
			}

		}

		// Build Cluster Group
		for (int clu_group_idx = 0; clu_group_idx < build_cluster_level.m_cluster_groups.size(); clu_group_idx++)
		{
			const SBuildClusterGroup& clu_group = build_cluster_level.m_cluster_groups[clu_group_idx];
			DirectX::XMFLOAT3 min_pos(1e30f, 1e30f, 1e30f);
			DirectX::XMFLOAT3 max_pos(-1e30f, -1e30f, -1e30f);

			CSimNaniteClusterGrpupResource cluster_group_resource;
			for (const auto& clu_idx : clu_group.m_cluster_indices)
			{
				SBuildCluster& build_cluster = build_cluster_level.m_clusters[clu_idx];
				min_pos = DirectX::XMFLOAT3((std::min)(min_pos.x, build_cluster.m_min_pos.x), (std::min)(min_pos.y, build_cluster.m_min_pos.y), (std::min)(min_pos.z, build_cluster.m_min_pos.z));
				max_pos = DirectX::XMFLOAT3((std::max)(max_pos.x, build_cluster.m_max_pos.x), (std::max)(max_pos.y, build_cluster.m_max_pos.y), (std::max)(max_pos.z, build_cluster.m_max_pos.z));
			}

			DirectX::XMFLOAT3 center = DirectX::XMFLOAT3((min_pos.x + max_pos.x) * 0.5, (min_pos.y + max_pos.y) * 0.5, (min_pos.z + max_pos.z) * 0.5);
			DirectX::XMFLOAT3 extents = DirectX::XMFLOAT3(max_pos.x - center.x, max_pos.y - center.y, max_pos.z - center.z);

			constexpr int lod_dist_range = 20;

			cluster_group_resource.m_bouding_box = DirectX::BoundingBox(center, extents);
			cluster_group_resource.cluster_pre_lod_dist = clu_level_idx * lod_dist_range;
			cluster_group_resource.cluster_next_lod_dist = (clu_level_idx + 1) * lod_dist_range;

			cluster_group_resource.m_cluster_num = clu_group.m_cluster_indices.size();
			for (const auto& clu_idx : clu_group.m_cluster_indices)
			{
				cluster_group_resource.m_clusters_indices.push_back(clu_idx + start_cluster_location);
			}

			if (clu_level_idx == ( m_cluster_levels.size() - 1))
			{
				cluster_group_resource.cluster_pre_lod_dist = clu_level_idx * lod_dist_range;
				cluster_group_resource.cluster_next_lod_dist = 1e25f;
			}

			out_nanite_reousource.m_cluster_groups.push_back(cluster_group_resource);
		}

		// Build Cluster Group Child Index
		if (clu_level_idx < m_cluster_levels.size() - 1)
		{
			std::vector<std::set<int>>cluster_group_child_sets;
			cluster_group_child_sets.resize(build_cluster_level.m_cluster_groups.size());
			int next_level_cluster_group_start_location = start_cluster_group_location + build_cluster_level.m_cluster_groups.size();

			for (int next_level_cluster_group_idx = 0; next_level_cluster_group_idx < m_cluster_levels[clu_level_idx + 1].m_cluster_groups.size(); next_level_cluster_group_idx++)
			{
				const SBuildClusterLevel& next_level = m_cluster_levels[clu_level_idx + 1];
				const SBuildClusterGroup& clu_group = next_level.m_cluster_groups[next_level_cluster_group_idx];
				for (int next_level_clu_idx = 0; next_level_clu_idx < clu_group.m_cluster_indices.size(); next_level_clu_idx++)
				{
					int next_clu_idx = clu_group.m_cluster_indices[next_level_clu_idx];
					const SBuildCluster& next_cluster = next_level.m_clusters[next_clu_idx];
					for (int face_group_idx = 0; face_group_idx < next_cluster.face_parent_cluster_group.size(); face_group_idx++)
					{
						int parent_clu_group_idx = next_cluster.face_parent_cluster_group[face_group_idx];
						cluster_group_child_sets[parent_clu_group_idx].insert(next_level_cluster_group_idx + next_level_cluster_group_start_location);
					}
				}
			}
		}

		CSimNaniteLodResource nanite_lod_resource;
		nanite_lod_resource.m_cluster_group_num = build_cluster_level.m_cluster_groups.size();
		nanite_lod_resource.m_cluster_group_start = start_cluster_group_location;

		out_nanite_reousource.m_nanite_lods.push_back(nanite_lod_resource);

		start_cluster_location += m_cluster_levels[clu_level_idx].m_clusters.size();
		start_cluster_group_location += m_cluster_levels[clu_level_idx].m_cluster_groups.size();
	}

	out_nanite_reousource.m_positions.resize(start_vertex_location);
	out_nanite_reousource.m_normals.resize(start_vertex_location);
	out_nanite_reousource.m_uvs.resize(start_vertex_location);
	out_nanite_reousource.m_header.m_vertex_count = start_vertex_location;
}
