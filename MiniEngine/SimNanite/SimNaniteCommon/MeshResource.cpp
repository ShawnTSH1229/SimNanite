#include "MeshResource.h"
#include "../SimNaniteBuilder/SimNaniteBuilder.h"
#include "../SimNaniteBuilder/SimNaniteObjLoader.h"
#include "Utility.h"
#include <fstream>

static void SaveFunc(char* data, long long size, void* streaming)
{
    std::ofstream* o_streaming = reinterpret_cast<std::ofstream*>(streaming);
    o_streaming->write(data, size);
}

static void LoadFunc(char* data, long long size, void* streaming)
{
    std::ifstream* i_streaming = reinterpret_cast<std::ifstream*>(streaming);
    i_streaming->read(data, size);
}

void CSimNaniteMeshResource::LoadFrom(const std::string& source_file_an, const std::wstring& source_file, bool bforce_rebuild)
{
	const std::wstring obj_file_Name = source_file;
	const std::wstring nanite_file_name = Utility::RemoveExtension(source_file) + L".nanite";

    struct _stat64 sourceFileStat;
    struct _stat64 miniFileStat;

    bool obj_file_missing = _wstat64(obj_file_Name.c_str(), &sourceFileStat) == -1;
    bool nanite_file_missing = _wstat64(nanite_file_name.c_str(), &miniFileStat) == -1;

    if (obj_file_missing && nanite_file_missing)
    {
        Utility::Printf("Error: Could not find %ws\n", obj_file_Name.c_str());
        return;
    }

    bool needBuild = bforce_rebuild;
    if (nanite_file_missing)
    {
        needBuild = true;
    }

    if (needBuild)
    {
        SBuildCluster total_mesh_cluster;
        bool load_result = LoadObj(source_file_an, &total_mesh_cluster);
        CSimNaniteBuilder nanite_builder;
        nanite_builder.Build(total_mesh_cluster, *this);

        std::ofstream out_file(nanite_file_name, std::ios::out | std::ios::binary);
        if (!out_file)
        {
            return;
        }

        out_file.write((char*)&m_header, sizeof(SSimNaniteMeshHeader));
        out_file.write((char*)&m_bouding_box, sizeof(DirectX::BoundingBox));

        out_file.write((char*)m_positions.data(), m_positions.size() * sizeof(DirectX::XMFLOAT3));
        out_file.write((char*)m_normals.data(), m_normals.size() * sizeof(DirectX::XMFLOAT3));
        out_file.write((char*)m_uvs.data(), m_uvs.size() * sizeof(DirectX::XMFLOAT2));
        out_file.write((char*)m_indices.data(), m_indices.size() * sizeof(unsigned int));

        for (int grp_idx = 0; grp_idx < m_header.m_cluster_group_size; grp_idx++)
        {
            m_cluster_groups[grp_idx].Serialize(SaveFunc, &out_file);
        }

        for (int clu_idx = 0; clu_idx < m_header.m_cluster_size; clu_idx++)
        {
            out_file.write((char*)m_clusters.data(), m_clusters.size() * sizeof(CSimNaniteClusterResource));
        }

        for (int lod_idx = 0; lod_idx < m_header.m_lod_size; lod_idx++)
        {
            out_file.write((char*)m_nanite_lods.data(), m_nanite_lods.size() * sizeof(CSimNaniteLodResource));
        }

    }
    else
    {
        std::ifstream in_file = std::ifstream(nanite_file_name, std::ios::in | std::ios::binary);
        in_file.read((char*)&m_header, sizeof(SSimNaniteMeshHeader));
        m_positions.resize(m_header.m_vertex_count);
        m_normals.resize(m_header.m_vertex_count);
        m_uvs.resize(m_header.m_vertex_count);
        m_indices.resize(m_header.m_index_count);
        m_cluster_groups.resize(m_header.m_cluster_group_size);
        m_clusters.resize(m_header.m_cluster_size);
        m_nanite_lods.resize(m_header.m_lod_size);

        in_file.read((char*)&m_bouding_box, sizeof(DirectX::BoundingBox));
        in_file.read((char*)m_positions.data(), m_positions.size() * sizeof(DirectX::XMFLOAT3));
        in_file.read((char*)m_normals.data(), m_normals.size() * sizeof(DirectX::XMFLOAT3));
        in_file.read((char*)m_uvs.data(), m_uvs.size() * sizeof(DirectX::XMFLOAT2));
        in_file.read((char*)m_indices.data(), m_indices.size() * sizeof(unsigned int));
        
        for (int grp_idx = 0; grp_idx < m_header.m_cluster_group_size; grp_idx++)
        {
            m_cluster_groups[grp_idx].Serialize(LoadFunc, &in_file);
        }
        
        for (int clu_idx = 0; clu_idx < m_header.m_cluster_size; clu_idx++)
        {
            in_file.read((char*)m_clusters.data(), m_clusters.size() * sizeof(CSimNaniteClusterResource));
        }
        
        for (int lod_idx = 0; lod_idx < m_header.m_lod_size; lod_idx++)
        {
            in_file.read((char*)m_nanite_lods.data(), m_nanite_lods.size() * sizeof(CSimNaniteLodResource));
        }

    }

}

void CSimNaniteClusterGrpupResource::Serialize(group_serialize_fun func, void* streaming)
{
    func((char*)&m_bouding_box, sizeof(DirectX::BoundingBox), streaming);
    func((char*)&cluster_next_lod_dist, sizeof(float), streaming);
    func((char*)&m_child_group_num, sizeof(int), streaming);
    func((char*)&m_cluster_num, sizeof(int), streaming);

    m_child_group_indices.resize(m_child_group_num);
    func((char*)m_child_group_indices.data(), sizeof(int) * m_child_group_num, streaming);

    m_clusters_indices.resize(m_cluster_num);
    func((char*)m_clusters_indices.data(), sizeof(int) * m_cluster_num, streaming);
}


