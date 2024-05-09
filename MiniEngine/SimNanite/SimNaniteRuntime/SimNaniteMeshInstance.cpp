#include "SimNaniteMeshInstance.h"

using namespace Graphics;

static void CreateBuffer(ByteAddressBuffer& out_buf, void* data, int num_element, int element_size)
{
    int buffer_size = num_element * element_size;
    UploadBuffer upload;
    upload.Create(L"None", buffer_size);
    memcpy(upload.Map(), data, buffer_size);
    upload.Unmap();
    out_buf.Create(L"Mesh Buffer", num_element, element_size, upload);
}

void CreateExampleNaniteMeshInstances(
    std::vector<SNaniteMeshInstance>& out_mesh_instances, 
    DescriptorHeap& tex_heap, DescriptorHeap& sampler_heap)
{
   
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Create");

    const uint32_t kNaniteNumTextures = 1;
    uint32_t DestCount = kNaniteNumTextures;
    uint32_t SourceCount = kNaniteNumTextures;

    TextureRef default_tex;
    uint32_t tex_table_idx;
    uint32_t sampler_table_idx;

    //texture
    {
        const std::wstring original_file = L"Textures/T_CheckBoard_D.png";
        CompileTextureOnDemand(original_file, 0);
        std::wstring ddsFile = Utility::RemoveExtension(original_file) + L".dds";
        default_tex = TextureManager::LoadDDSFromFile(ddsFile);

        DescriptorHandle texture_handles = tex_heap.Alloc(1);
        tex_table_idx = tex_heap.GetOffsetOfHandle(texture_handles);

        D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[kNaniteNumTextures];
        SourceTextures[0] = default_tex.GetSRV();

        g_Device->CopyDescriptors(1, &texture_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    //sampler
    {
        DescriptorHandle SamplerHandles = sampler_heap.Alloc(kNaniteNumTextures);
        sampler_table_idx = sampler_heap.GetOffsetOfHandle(SamplerHandles);

        D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[kNaniteNumTextures] = { SamplerLinearWrap };

        g_Device->CopyDescriptors(1, &SamplerHandles, &DestCount, DestCount, SourceSamplers, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    out_mesh_instances.resize(2);
    {
        SNaniteMeshInstance& mesh_instance = out_mesh_instances[0];
        CSimNaniteMeshResource& nanite_mesh_resource = mesh_instance.m_nanite_mesh_resource;
        nanite_mesh_resource.LoadFrom(std::string("bunny/bunny.obj"), std::wstring(L"bunny/bunny.obj"), false);
        
        //buffer
        CreateBuffer(mesh_instance.m_vertex_pos_buffer, nanite_mesh_resource.m_positions.data(),
            nanite_mesh_resource.m_positions.size(), sizeof(DirectX::XMFLOAT3));

        CreateBuffer(mesh_instance.m_vertex_norm_buffer, nanite_mesh_resource.m_normals.data(),
            nanite_mesh_resource.m_normals.size(), sizeof(DirectX::XMFLOAT3));

        CreateBuffer(mesh_instance.m_vertex_uv_buffer, nanite_mesh_resource.m_uvs.data(),
            nanite_mesh_resource.m_uvs.size(), sizeof(DirectX::XMFLOAT2));

        CreateBuffer(mesh_instance.m_index_buffer, nanite_mesh_resource.m_indices.data(),
            nanite_mesh_resource.m_indices.size(), sizeof(unsigned int));

        mesh_instance.m_texture = default_tex;
        mesh_instance.m_tex_table_index = tex_table_idx;
        mesh_instance.m_sampler_table_idx = sampler_table_idx;

        // constant buffer
        {
            mesh_instance.m_mesh_constants_cpu.Create(L"Mesh Constant Upload Buffer", 1 * 1 * sizeof(MeshConstants));
            mesh_instance.m_mesh_constants_gpu.Create(L"Mesh Constant GPU Buffer", 1 * 1, sizeof(SNaniteMeshConstants));

            MeshConstants* cb = (MeshConstants*)mesh_instance.m_mesh_constants_cpu.Map();
            MeshConstants& cbv = cb[0];
            cbv.World = Matrix4(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1));
            cbv.WorldIT = InverseTranspose(cbv.World.Get3x3());
            mesh_instance.m_mesh_constants_cpu.Unmap();

            gfxContext.TransitionResource(mesh_instance.m_mesh_constants_gpu, D3D12_RESOURCE_STATE_COPY_DEST, true);
            gfxContext.GetCommandList()->CopyBufferRegion(mesh_instance.m_mesh_constants_gpu.GetResource(), 0, mesh_instance.m_mesh_constants_cpu.GetResource(), 0, mesh_instance.m_mesh_constants_cpu.GetBufferSize());
            gfxContext.TransitionResource(mesh_instance.m_mesh_constants_gpu, D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        {
            for (int x = 0; x < 8; x++)
            {
                for (int z = 0; z < 8; z++)
                {
                    SInstanceData instance_data;
                    instance_data.World = Matrix4(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1));
                    instance_data.World = Matrix4::MakeScale(10.0) * Matrix4(AffineTransform(Vector3(x * 5, 3.0, z * 5)));
                    instance_data.WorldIT = InverseTranspose(instance_data.World.Get3x3());
                    mesh_instance.m_instance_datas.push_back(instance_data);
                }
            }
            
            int num_element = mesh_instance.m_instance_datas.size();
            int element_size = sizeof(SInstanceData);
            mesh_instance.m_instance_buffer.Create(L"None", num_element, element_size, mesh_instance.m_instance_datas.data());

            {
                DescriptorHandle instance_buffer_handles = tex_heap.Alloc(1);
                mesh_instance.m_instance_buffer_idx = tex_heap.GetOffsetOfHandle(instance_buffer_handles);

                D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[kNaniteNumTextures];
                SourceTextures[0] = mesh_instance.m_instance_buffer.GetSRV();

                g_Device->CopyDescriptors(1, &instance_buffer_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }
    }

    {
        SNaniteMeshInstance& mesh_instance = out_mesh_instances[1];
        CSimNaniteMeshResource& nanite_mesh_resource = mesh_instance.m_nanite_mesh_resource;
        nanite_mesh_resource.LoadFrom(std::string("teapot/teapot.obj"), std::wstring(L"teapot/teapot.obj"), false);

        //buffer
        CreateBuffer(mesh_instance.m_vertex_pos_buffer, nanite_mesh_resource.m_positions.data(),
            nanite_mesh_resource.m_positions.size(), sizeof(DirectX::XMFLOAT3));

        CreateBuffer(mesh_instance.m_vertex_norm_buffer, nanite_mesh_resource.m_normals.data(),
            nanite_mesh_resource.m_normals.size(), sizeof(DirectX::XMFLOAT3));

        CreateBuffer(mesh_instance.m_vertex_uv_buffer, nanite_mesh_resource.m_uvs.data(),
            nanite_mesh_resource.m_uvs.size(), sizeof(DirectX::XMFLOAT2));

        CreateBuffer(mesh_instance.m_index_buffer, nanite_mesh_resource.m_indices.data(),
            nanite_mesh_resource.m_indices.size(), sizeof(unsigned int));

        mesh_instance.m_texture = default_tex;
        mesh_instance.m_tex_table_index = tex_table_idx;
        mesh_instance.m_sampler_table_idx = sampler_table_idx;

        // constant buffer
        {
            mesh_instance.m_mesh_constants_cpu.Create(L"Mesh Constant Upload Buffer", 1 * 1 * sizeof(MeshConstants));
            mesh_instance.m_mesh_constants_gpu.Create(L"Mesh Constant GPU Buffer", 1 * 1, sizeof(SNaniteMeshConstants));

            MeshConstants* cb = (MeshConstants*)mesh_instance.m_mesh_constants_cpu.Map();
            MeshConstants& cbv = cb[0];
            cbv.World = Matrix4(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1));
            cbv.WorldIT = InverseTranspose(cbv.World.Get3x3());
            mesh_instance.m_mesh_constants_cpu.Unmap();

            gfxContext.TransitionResource(mesh_instance.m_mesh_constants_gpu, D3D12_RESOURCE_STATE_COPY_DEST, true);
            gfxContext.GetCommandList()->CopyBufferRegion(mesh_instance.m_mesh_constants_gpu.GetResource(), 0, mesh_instance.m_mesh_constants_cpu.GetResource(), 0, mesh_instance.m_mesh_constants_cpu.GetBufferSize());
            gfxContext.TransitionResource(mesh_instance.m_mesh_constants_gpu, D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        {
            for (int x = 0; x < 10; x++)
            {
                for (int z = 0; z < 10; z++)
                {
                    SInstanceData instance_data;
                    instance_data.World = Matrix4(Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(0, 0, 0, 1));
                    instance_data.World = Matrix4::MakeScale(0.1) * Matrix4(AffineTransform(Vector3(x * 300, 0, z * 300)));
                    instance_data.WorldIT = InverseTranspose(instance_data.World.Get3x3());
                    mesh_instance.m_instance_datas.push_back(instance_data);
                }
            }

            int num_element = mesh_instance.m_instance_datas.size();
            int element_size = sizeof(SInstanceData);
            mesh_instance.m_instance_buffer.Create(L"None", num_element, element_size, mesh_instance.m_instance_datas.data());

            {
                DescriptorHandle instance_buffer_handles = tex_heap.Alloc(1);
                mesh_instance.m_instance_buffer_idx = tex_heap.GetOffsetOfHandle(instance_buffer_handles);

                D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[kNaniteNumTextures];
                SourceTextures[0] = mesh_instance.m_instance_buffer.GetSRV();

                g_Device->CopyDescriptors(1, &instance_buffer_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }
        
        
    }
    gfxContext.Finish();
	
}
