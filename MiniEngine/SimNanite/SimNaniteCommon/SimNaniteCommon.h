#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxcapi.h>

// windows headers
#include <comdef.h>
#include <Windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <memory>

#define MAKE_SMART_COM_PTR(_a) _COM_SMARTPTR_TYPEDEF(_a, __uuidof(_a))

MAKE_SMART_COM_PTR(IDXGIFactory4);
MAKE_SMART_COM_PTR(ID3D12Device5);
MAKE_SMART_COM_PTR(IDXGIAdapter1);
MAKE_SMART_COM_PTR(ID3D12Debug);
MAKE_SMART_COM_PTR(ID3D12CommandQueue);
MAKE_SMART_COM_PTR(ID3D12CommandAllocator);
MAKE_SMART_COM_PTR(ID3D12GraphicsCommandList4);
MAKE_SMART_COM_PTR(ID3D12Resource);
MAKE_SMART_COM_PTR(ID3D12Fence);
MAKE_SMART_COM_PTR(IDxcBlobEncoding);
MAKE_SMART_COM_PTR(IDxcCompiler);
MAKE_SMART_COM_PTR(IDxcLibrary);
MAKE_SMART_COM_PTR(IDxcOperationResult);
MAKE_SMART_COM_PTR(IDxcBlob);
MAKE_SMART_COM_PTR(ID3DBlob);
MAKE_SMART_COM_PTR(ID3D12StateObject);
MAKE_SMART_COM_PTR(ID3D12RootSignature);
MAKE_SMART_COM_PTR(IDxcValidator);
MAKE_SMART_COM_PTR(ID3D12StateObjectProperties);
MAKE_SMART_COM_PTR(ID3D12DescriptorHeap);
MAKE_SMART_COM_PTR(ID3D12PipelineState);

template<class BlotType>
inline std::string ConvertBlobToString(BlotType* pBlob)
{
    std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
    memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
    infoLog[pBlob->GetBufferSize()] = 0;
    return std::string(infoLog.data());
}