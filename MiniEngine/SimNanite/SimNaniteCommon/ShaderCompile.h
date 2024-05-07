#pragma once
#include "SimNaniteCommon.h"

struct SCompiledShaderCode
{
	inline void* GetBufferPointer(void)
	{
		return data.data();
	}

	inline size_t GetBufferSize(void)
	{
		return data.size();
	}

	std::vector<uint8_t> data;
};


class CShaderCompiler
{
public:
	std::shared_ptr<SCompiledShaderCode> Compile(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, DxcDefine* dxcDefine, uint32_t defineCount);
	std::shared_ptr<SCompiledShaderCode> CompileDxc(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, DxcDefine* dxcDefine, uint32_t defineCount);
	std::shared_ptr<SCompiledShaderCode> CompileFxc(const std::wstring& shaderPath, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, DxcDefine* dxcDefine, uint32_t defineCount);
	void Init(bool use_fxc = true);
private:
	bool m_use_fxc;

	IDxcCompilerPtr m_dxc_compiler_ptr;
	IDxcLibraryPtr m_library_ptr;
	IDxcValidatorPtr m_dxc_validator_ptr;
};