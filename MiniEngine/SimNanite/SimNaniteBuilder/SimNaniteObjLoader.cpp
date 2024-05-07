#include "SimNaniteObjLoader.h"
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

bool LoadObj(const std::string& inputfile, SBuildCluster* out_mesh_data)
{
    if (!out_mesh_data)
    {
        return false;
    }

    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) 
    {
        if (!reader.Error().empty()) 
        {
            return false;
        }
    }

    //if (!reader.Warning().empty()) 
    //{
    //    return false;
    //}

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    //if (shapes.size() != 1)
    //{
    //    return false;
    //}

    int total_vtx_num = 0;

    for (size_t s = 0; s < shapes.size(); s++)
    {
        total_vtx_num += shapes[s].mesh.num_face_vertices.size() * 3;
    }

    out_mesh_data->m_positions.reserve(total_vtx_num);
    out_mesh_data->m_normals.reserve(total_vtx_num);
    out_mesh_data->m_uvs.reserve(total_vtx_num);

    for (size_t s = 0; s < shapes.size(); s++)
    {

    }

    for (size_t s = 0; s < shapes.size(); s++) 
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            if (fv != 3)
            {
                return false;
            }

            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                out_mesh_data->m_positions.push_back(DirectX::XMFLOAT3(vx, vy, vz));

                if (idx.normal_index >= 0) 
                {
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    out_mesh_data->m_normals.push_back(DirectX::XMFLOAT3(nx, ny, nz));
                }
                else
                {
                    out_mesh_data->m_normals.push_back(DirectX::XMFLOAT3(0, 0, 1));
                }

                if (idx.texcoord_index >= 0) 
                {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    out_mesh_data->m_uvs.push_back(DirectX::XMFLOAT2(tx, ty));
                }
            }
            index_offset += fv;
        }
    }
    return true;
}