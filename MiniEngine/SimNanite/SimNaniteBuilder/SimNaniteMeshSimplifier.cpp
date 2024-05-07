#include "SimNaniteBuilder.h"
#include "../SimNaniteCommon/NaniteUtils.h"
#include <fstream>
#include <iostream>
#include <xhash>

#include "meshoptimizer.h"

bool CSimNaniteMeshSimplifier::SimplifiyMergedClusterGroup(SBuildCluster& merged_cluster, SBuildCluster& simplified_cluster)
{
	std::vector<unsigned int>simplified_index;
	simplified_index.resize(merged_cluster.m_positions.size());

	//generate vertex buffer
	{
		size_t index_count = merged_cluster.m_positions.size();
		size_t unindexed_vertex_count = merged_cluster.m_positions.size();
		std::vector<unsigned int> remap_index_buffer(index_count);

		std::vector<DirectX::XMFLOAT3> remapped_vertex_pos_buffer;
		std::vector<DirectX::XMFLOAT3> remapped_vertex_norm_buffer;
		std::vector<DirectX::XMFLOAT2> remapped_vertex_uv_buffer;

		remapped_vertex_pos_buffer.resize(unindexed_vertex_count);
		remapped_vertex_norm_buffer.resize(unindexed_vertex_count);
		remapped_vertex_uv_buffer.resize(unindexed_vertex_count);

		size_t vertex_count = meshopt_generateVertexRemap(remap_index_buffer.data(), nullptr, index_count, merged_cluster.m_positions.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3));
		meshopt_remapVertexBuffer(remapped_vertex_pos_buffer.data(), merged_cluster.m_positions.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3), remap_index_buffer.data());
		meshopt_remapVertexBuffer(remapped_vertex_norm_buffer.data(), merged_cluster.m_normals.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT3), remap_index_buffer.data());
		meshopt_remapVertexBuffer(remapped_vertex_uv_buffer.data(), merged_cluster.m_uvs.data(), unindexed_vertex_count, sizeof(DirectX::XMFLOAT2), remap_index_buffer.data());

		float* result = nullptr;
		size_t dest_index_num = meshopt_simplify(
			simplified_index.data(), remap_index_buffer.data(), remap_index_buffer.size(),
			(const float*)remapped_vertex_pos_buffer.data(), remapped_vertex_pos_buffer.size(),
			sizeof(DirectX::XMFLOAT3), simplified_index.size() / 4, 1, meshopt_SimplifyLockBorder, result);

		int simplified_index_num = simplified_index.size();
		simplified_cluster.m_positions.resize(dest_index_num);
		simplified_cluster.m_normals.resize(dest_index_num);
		simplified_cluster.m_uvs.resize(dest_index_num);

		for (int index_idx = 0; index_idx < dest_index_num; index_idx++)
		{
			simplified_cluster.m_positions[index_idx] = remapped_vertex_pos_buffer[simplified_index[index_idx]];
			simplified_cluster.m_normals[index_idx] = remapped_vertex_norm_buffer[simplified_index[index_idx]];
			simplified_cluster.m_uvs[index_idx] = remapped_vertex_uv_buffer[simplified_index[index_idx]];
		}

		return dest_index_num == remap_index_buffer.size();
	}
}