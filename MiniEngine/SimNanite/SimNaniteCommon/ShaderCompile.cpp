#include "ShaderCompile.h"
#include "Utility.h"
#include <fstream>
#include <sstream>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")



std::shared_ptr<SCompiledShaderCode> CShaderCompiler::Compile(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, D3D_SHADER_MACRO* fxcDefine, uint32_t defineCount)
{
    if (m_use_fxc)
    {
        return CompileFxc(shaderPath, pEntryPoint, pTargetProfile, fxcDefine, defineCount);
    }
    else
    {
        return CompileDxc(shaderPath, pEntryPoint, pTargetProfile, fxcDefine, defineCount);
    }
}

std::shared_ptr<SCompiledShaderCode> CShaderCompiler::CompileDxc(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, D3D_SHADER_MACRO* fxcDefine, uint32_t defineCount)
{
    std::ifstream shader_file(shaderPath);

    if (shader_file.good() == false)
    {
        ASSERT(false);
    }

    std::stringstream str_stream;
    str_stream << shader_file.rdbuf();
    std::string shader = str_stream.str();

    IDxcBlobEncodingPtr text_blob_ptr;
    IDxcOperationResultPtr result_ptr;
    HRESULT result_code;
    IDxcBlobPtr blob_ptr;
    IDxcOperationResultPtr valid_result_ptr;

    ASSERT(defineCount == 0);

    ASSERT_SUCCEEDED(m_library_ptr->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &text_blob_ptr));
    ASSERT_SUCCEEDED(m_dxc_compiler_ptr->Compile(text_blob_ptr, shaderPath.data(), pEntryPoint, pTargetProfile, nullptr, 0, nullptr, 0, nullptr, &result_ptr));
    ASSERT_SUCCEEDED(result_ptr->GetStatus(&result_code));

    if (FAILED(result_code))
    {
        IDxcBlobEncodingPtr pError;
        ASSERT_SUCCEEDED(result_ptr->GetErrorBuffer(&pError));
        std::string msg = ConvertBlobToString(pError.GetInterfacePtr());
        ERROR(msg.c_str());
        ASSERT_SUCCEEDED(-1);
    }

    ASSERT_SUCCEEDED(result_ptr->GetResult(&blob_ptr));
    m_dxc_validator_ptr->Validate(blob_ptr, DxcValidatorFlags_InPlaceEdit, &valid_result_ptr);

    HRESULT validateStatus;
    valid_result_ptr->GetStatus(&validateStatus);
    if (FAILED(validateStatus))
    {
        ASSERT_SUCCEEDED(-1);
    }

    std::shared_ptr<SCompiledShaderCode> ret_code = std::make_shared<SCompiledShaderCode>();
    ret_code->data.resize(blob_ptr->GetBufferSize());
    memcpy(ret_code->data.data(), blob_ptr->GetBufferPointer(), blob_ptr->GetBufferSize());
    return ret_code;
}

std::shared_ptr<SCompiledShaderCode> CShaderCompiler::CompileFxc(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, D3D_SHADER_MACRO* fxcDefine, uint32_t defineCount)
{
    ID3DBlobPtr code_gen;
    ID3DBlobPtr errors;

    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    std::vector<D3D_SHADER_MACRO>defines;
    defines.resize(defineCount);
    for (int def_idx = 0; def_idx < defineCount; def_idx++)
    {
        defines[def_idx] = fxcDefine[def_idx];
    }

    D3D_SHADER_MACRO end_macro;
    end_macro.Definition = NULL;
    end_macro.Name = NULL;
    defines.push_back(end_macro);
 
    char enret_u8[MAX_PATH];
    char target_u8[MAX_PATH];

    //LPSTR entry_u8;
    //LPSTR target_u8;
    if (!WideCharToMultiByte(CP_UTF8, 0, pEntryPoint, -1, enret_u8, MAX_PATH, nullptr, nullptr))
    {
        return nullptr;
    }

    if (!WideCharToMultiByte(CP_UTF8, 0, pTargetProfile, -1, target_u8, MAX_PATH, nullptr, nullptr))
    {
        return nullptr;
    }

    HRESULT hr = D3DCompileFromFile(shaderPath.c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, enret_u8, target_u8, compileFlags, 0, &code_gen, &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }

    std::shared_ptr<SCompiledShaderCode> ret_code = std::make_shared<SCompiledShaderCode>();
    ret_code->data.resize(code_gen->GetBufferSize());
    memcpy(ret_code->data.data(), code_gen->GetBufferPointer(), code_gen->GetBufferSize());
    return ret_code;
}

void CShaderCompiler::Init(bool use_fxc)
{
    m_use_fxc = use_fxc;
    ASSERT_SUCCEEDED(DxcCreateInstance(CLSID_DxcValidator, IID_PPV_ARGS(&m_dxc_validator_ptr)));
    ASSERT_SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxc_compiler_ptr)));
    ASSERT_SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library_ptr)));
}
