#include <algorithm>
#include "SimNaniteBuilder.h"

struct SClusterGroupToSort
{
	uint32_t m_origianl_group_index = 0;
	DirectX::BoundingBox m_bounding_box;
};

struct SCustomLess
{
	int m_split_dimension = 0;
	bool operator()(SClusterGroupBVHNode a, SClusterGroupBVHNode b) const
	{
		if (m_split_dimension == 0)
		{
			return a.m_bouding_box.Extents.x < b.m_bouding_box.Extents.x;
		}
		else if (m_split_dimension == 1)
		{
			return a.m_bouding_box.Extents.y < b.m_bouding_box.Extents.y;
		}
		else
		{
			return a.m_bouding_box.Extents.z < b.m_bouding_box.Extents.z;
		}
	}
};

void CSimNaniteBuilder::BuildBVH(CSimNaniteMeshResource& out_nanite_reousource)
{
	// todo: optimize this function
	const DirectX::BoundingBox& bbox = out_nanite_reousource.m_bouding_box;
	


	std::vector<SClusterGroupBVHNode>& bvh_nodes = out_nanite_reousource.m_bvh_nodes;

	for (uint32_t lod_idx = 0; lod_idx < out_nanite_reousource.m_nanite_lods.size(); lod_idx++)
	{
		CSimNaniteLodResource& lod_resource = out_nanite_reousource.m_nanite_lods[lod_idx];

		std::vector<SClusterGroupBVHNode> leaf_nodes;
		for (uint32_t clu_grp_idx = 0; clu_grp_idx < lod_resource.m_cluster_group_num; clu_grp_idx++)
		{
			uint32_t global_clu_grp_idx = lod_resource.m_cluster_group_start + clu_grp_idx;
			SClusterGroupBVHNode leaf_node;
			leaf_node.m_is_leaf_node = true;
			leaf_node.m_cluster_group_index = global_clu_grp_idx;
			leaf_node.m_bouding_box = out_nanite_reousource.m_cluster_groups[global_clu_grp_idx].m_bouding_box;
			leaf_nodes.push_back(leaf_node);
		}

		uint32_t max_dimension_index = (bbox.Extents.x > bbox.Extents.y) ? 0 : 1;
		float max_dimension = (bbox.Extents.x > bbox.Extents.y) ? bbox.Extents.x : bbox.Extents.y;
		max_dimension_index = (max_dimension > bbox.Extents.z) ? max_dimension_index : 2;
		max_dimension = (max_dimension > bbox.Extents.z) ? max_dimension : bbox.Extents.z;
		SCustomLess custom_less;
		custom_less.m_split_dimension = max_dimension_index;

		std::sort(leaf_nodes.begin(), leaf_nodes.end(), custom_less);

		uint32_t offset = bvh_nodes.size();
		uint32_t level_node_num = leaf_nodes.size();
		for (uint32_t leaf_idx = 0; leaf_idx < leaf_nodes.size(); leaf_idx++)
		{
			bvh_nodes.push_back(leaf_nodes[leaf_idx]);
		}
		
		while (true)
		{
			std::vector<SClusterGroupBVHNode> tree_nodes;
			for (uint32_t tree_node_idx = 0; tree_node_idx < level_node_num; tree_node_idx += 4)
			{
				uint32_t child_node_start_idx = tree_node_idx + offset;
				SClusterGroupBVHNode tree_node;
				tree_node.m_is_leaf_node = false;
				tree_node.m_child_node_index[0] = child_node_start_idx + 0;
				tree_node.m_child_node_index[1] = child_node_start_idx + 1;
				tree_node.m_child_node_index[2] = child_node_start_idx + 2;
				tree_node.m_child_node_index[3] = child_node_start_idx + 3;

				DirectX::BoundingBox merged_box;
				DirectX::BoundingBox::CreateMerged(merged_box, bvh_nodes[child_node_start_idx + 0].m_bouding_box, bvh_nodes[child_node_start_idx + 1].m_bouding_box);
				DirectX::BoundingBox::CreateMerged(merged_box, merged_box, bvh_nodes[child_node_start_idx + 2].m_bouding_box);
				DirectX::BoundingBox::CreateMerged(merged_box, merged_box, bvh_nodes[child_node_start_idx + 3].m_bouding_box);

				tree_node.m_bouding_box = merged_box;
				bvh_nodes.push_back(tree_node);
			};
			offset += level_node_num;
			level_node_num /= 4;

			if (level_node_num == 1)
			{
				SClusterGroupBVHNode tree_node;
				tree_node.m_is_leaf_node = false;
				tree_node.m_child_node_index[0] = offset + 0;
				tree_node.m_child_node_index[1] = offset + 1;
				tree_node.m_child_node_index[2] = offset + 2;
				tree_node.m_child_node_index[3] = offset + 3;
				tree_node.m_bouding_box = out_nanite_reousource.m_bouding_box;
				bvh_nodes.push_back(tree_node);
				lod_resource.m_root_node_index = bvh_nodes.size() - 1;
				break;
			}
		};
	}
}