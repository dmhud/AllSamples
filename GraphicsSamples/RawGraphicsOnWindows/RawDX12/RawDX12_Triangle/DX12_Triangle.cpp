#include "DX12_Triangle.h"

#include <iostream>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "Common/DX.Utils/DxException.h"
#include "Common/Math.Utils/Math.h"
#include "Common/Win.Utils/ExePath.h"

void DX12_Triangle::DrawTriangle(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight)
{
    using namespace dx;
    using namespace math;

    auto exePath = win::exe_path();

    ///////////////////////////////
    ///      Initialize API     ///
    ///////////////////////////////

    /**
     * Factory
     */
     // Declare DirectX 12 Handles
    IDXGIFactory4* factory;
    ID3D12Debug1* debugController;

    // Create Factory
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Create a Debug Controller to track errors
    ID3D12Debug* dc;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&dc)));
    ThrowIfFailed(dc->QueryInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
    debugController->SetEnableGPUBasedValidation(true);

    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

    dc->Release();
    dc = nullptr;
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
    
    /**
     * Adapter
     */
    IDXGIAdapter1* adapter;

    // Create Adapter
    for (UINT adapterIndex = 0;
        DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter);
        ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Don't select the Basic Render Driver adapter.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        // Check if the adapter supports Direct3D 12, and use that for the rest
        // of the application
        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0,
            _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }

        // Else we won't use this iteration's adapter, so release it
        adapter->Release();
    }
    if (!adapter)
        throw std::runtime_error("No adapter");
    
    /**
     * Device
     */
     // Declare Handles
    ID3D12Device* device;

    // Create Device
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    // Debug mode
    ID3D12DebugDevice* debugDevice;

#if defined(_DEBUG)
    // Get debug device
    ThrowIfFailed(device->QueryInterface(&debugDevice));
#endif

    /**
     * Command Queue
     */
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ID3D12CommandQueue* commandQueue;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    /**
     * Command Allocator
     */
    ID3D12CommandAllocator* commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    /**
     * Synchronization
     */
    UINT frameIndex;
    HANDLE fenceEvent;
    UINT64 fenceValue;

    // Create fence
    ID3D12Fence* fence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence)));

    //// Declare handles
    //ID3D12GraphicsCommandList* commandList;
    //
    //// Create Barrier
    //D3D12_RESOURCE_BARRIER barrier = {};
    //result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //barrier.Transition.pResource = texResource;
    //barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    //barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    //barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    //
    //commandList->ResourceBarrier(1, &barrier);

    /**
     * Swapchain
     */

    // Declare Handles
    static const UINT backbufferCount = 2;
    UINT currentBuffer;
    ID3D12DescriptorHeap* renderTargetViewHeap;
    ID3D12Resource* renderTargets[backbufferCount];
    UINT rtvDescriptorSize;

    // Surface
    D3D12_RECT surfaceSize;
    surfaceSize.left = 0;
    surfaceSize.top = 0;
    surfaceSize.right = static_cast<LONG>(windowWidth);
    surfaceSize.bottom = static_cast<LONG>(windowHeight);

    // Viewport
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(windowWidth);
    viewport.Height = static_cast<float>(windowHeight);
    viewport.MinDepth = .1f;
    viewport.MaxDepth = 1000.f;

    IDXGISwapChain3* swapchain = nullptr;
    if (swapchain)
    {
        // Create Render Target Attachments from swapchain
        swapchain->ResizeBuffers(backbufferCount, windowWidth, windowHeight,
            DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    }
    else
    {
        // Create swapchain
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = windowWidth;
        swapchainDesc.Height = windowHeight;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = backbufferCount;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        IDXGISwapChain1* swapChain1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, hWnd, &swapchainDesc, nullptr, nullptr, &swapChain1));

        IDXGISwapChain1* newSwapchain;
        HRESULT swapchainSupport = swapChain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&newSwapchain);
        if (SUCCEEDED(swapchainSupport))
        {
            swapchain = (IDXGISwapChain3*)newSwapchain;
        }
    }

    frameIndex = swapchain->GetCurrentBackBufferIndex();


    ///////////////////////////////
    ///       Resources         ///
    ///////////////////////////////
    /**
     * Descriptor Heaps
     */
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = backbufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&renderTargetViewHeap)));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create frame resources
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each frame.
    for (UINT n = 0; n < backbufferCount; n++)
    {
        ThrowIfFailed(swapchain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
        device->CreateRenderTargetView(renderTargets[n], nullptr, rtvHandle);
        rtvHandle.ptr += (1 * rtvDescriptorSize);
    }

    /**
     * Root Signature
     */
     // Declare Handles
    ID3D12RootSignature* rootSignature;

    // Determine if we can get Root Signature Version 1.1:
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Individual GPU Resources
    D3D12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].BaseShaderRegister = 0;
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    ranges[0].NumDescriptors = 1;
    ranges[0].RegisterSpace = 0;
    ranges[0].OffsetInDescriptorsFromTableStart = 0;
    ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    // Groups of GPU Resources
    D3D12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

    // Overall Layout
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.Desc_1_1.NumParameters = 1;
    rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
    rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

    ID3DBlob* signature;
    ID3DBlob* error = nullptr;
    try
    {
        // Create the root signature
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error), error);
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        rootSignature->SetName(L"Hello Triangle Root Signature");
    }
    catch (const std::exception& e)
    {
        const char* errStr = (const char*)error->GetBufferPointer();
        std::cout << errStr;
        error->Release();
        error = nullptr;
    }

    if (signature)
    {
        signature->Release();
        signature = nullptr;
    }

    /**
     * Heaps
     */


    /**
     * Vertex Buffer
     */
    // Declare Data
    struct Vertex
    {
        float position[3];
        float color[3];
    };
    Vertex vertexBufferData[3] = { {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                   {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                   {{ 0.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} };
    {
        // Declare Handles
        ID3D12Resource* vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        const UINT vertexBufferSize = sizeof(vertexBufferData);

        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC vertexBufferResourceDesc;
        vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferResourceDesc.Alignment = 0;
        vertexBufferResourceDesc.Width = vertexBufferSize;
        vertexBufferResourceDesc.Height = 1;
        vertexBufferResourceDesc.DepthOrArraySize = 1;
        vertexBufferResourceDesc.MipLevels = 1;
        vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferResourceDesc.SampleDesc.Count = 1;
        vertexBufferResourceDesc.SampleDesc.Quality = 0;
        vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;

        // We do not intend to read from this resource on the CPU.
        D3D12_RANGE readRange;
        readRange.Begin = 0;
        readRange.End = 0;

        ThrowIfFailed(vertexBuffer->Map(0, &readRange,
            reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, vertexBufferData, sizeof(vertexBufferData));
        vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    /**
     * Index Buffer
     */
    {
        // Declare Data
        uint32_t indexBufferData[3] = { 0, 1, 2 };

        // Declare Handles
        ID3D12Resource* indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        const UINT indexBufferSize = sizeof(indexBufferData);

        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC vertexBufferResourceDesc;
        vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferResourceDesc.Alignment = 0;
        vertexBufferResourceDesc.Width = indexBufferSize;
        vertexBufferResourceDesc.Height = 1;
        vertexBufferResourceDesc.DepthOrArraySize = 1;
        vertexBufferResourceDesc.MipLevels = 1;
        vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferResourceDesc.SampleDesc.Count = 1;
        vertexBufferResourceDesc.SampleDesc.Quality = 0;
        vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer)));

        // Copy data to DirectX 12 driver memory:
        UINT8* pVertexDataBegin;

        // We do not intend to read from this resource on the CPU.
        D3D12_RANGE readRange;
        readRange.Begin = 0;
        readRange.End = 0;

        ThrowIfFailed(indexBuffer->Map(0, &readRange,
            reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, indexBufferData, sizeof(indexBufferData));
        indexBuffer->Unmap(0, nullptr);

        // Initialize the index buffer view.
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBufferView.SizeInBytes = indexBufferSize;
    }

    /**
     * Constant Buffer
     */
    // Declare Data
    struct
    {
        float4x4 projectionMatrix;
        float4x4 modelMatrix;
        float4x4 viewMatrix;
    } cbVS;

    // Declare Handles
    ID3D12Resource* constantBuffer;
    ID3D12DescriptorHeap* constantBufferHeap;
    UINT8* mappedConstantBuffer;
    {
        // Create the Constant Buffer

        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc,
            IID_PPV_ARGS(&constantBufferHeap)));

        D3D12_RESOURCE_DESC cbResourceDesc;
        cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbResourceDesc.Alignment = 0;
        cbResourceDesc.Width = (sizeof(cbVS) + 255) & ~255;
        cbResourceDesc.Height = 1;
        cbResourceDesc.DepthOrArraySize = 1;
        cbResourceDesc.MipLevels = 1;
        cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        cbResourceDesc.SampleDesc.Count = 1;
        cbResourceDesc.SampleDesc.Quality = 0;
        cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));
        constantBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

        // Create our Constant Buffer View
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes =
            (sizeof(cbVS) + 255) & ~255; // CB size is required to be 256-byte aligned.

        D3D12_CPU_DESCRIPTOR_HANDLE
            cbvHandle(constantBufferHeap->GetCPUDescriptorHandleForHeapStart());
        cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
            0;

        device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // We do not intend to read from this resource on the CPU.
        D3D12_RANGE readRange;
        readRange.Begin = 0;
        readRange.End = 0;

        ThrowIfFailed(constantBuffer->Map(
            0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
        memcpy(mappedConstantBuffer, &cbVS, sizeof(cbVS));
        constantBuffer->Unmap(0, &readRange);
    }

    /**
     * Shaders options
     */
#if defined(_DEBUG)
     // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ID3DBlob* errors = nullptr;

    /**
     * Vertex Shader
     */
    ID3DBlob* vertexShaderBlob = nullptr;
    std::wstring pathVS = exePath / "shaders" / "triangle.vs.hlsl";
    ThrowIfFailed(D3DCompileFromFile(pathVS.c_str(), nullptr, nullptr, "main",
        "vs_5_0", compileFlags, 0, &vertexShaderBlob,
        &errors), errors);

    // Declare handles
    D3D12_SHADER_BYTECODE vsBytecode;
    vsBytecode.pShaderBytecode = vertexShaderBlob->GetBufferPointer();
    vsBytecode.BytecodeLength = vertexShaderBlob->GetBufferSize();
    
    /**
     * Pixel Shader
     */
    ID3DBlob* pixelShaderBlob = nullptr;
    std::wstring pathPS = exePath / "shaders" / "triangle.ps.hlsl";
    ThrowIfFailed(D3DCompileFromFile(pathPS.c_str(), nullptr, nullptr, "main",
        "ps_5_0", compileFlags, 0, &pixelShaderBlob,
        &errors), errors);

    // Declare handles
    D3D12_SHADER_BYTECODE psBytecode;
    psBytecode.pShaderBytecode = pixelShaderBlob->GetBufferPointer();
    psBytecode.BytecodeLength = pixelShaderBlob->GetBufferSize();

    /**
     * Pipeline State
     */

    ///////////////////////////////
    ///   Encoding Commands     ///
    ///////////////////////////////

    ///////////////////////////////
    ///       Rendering         ///
    ///////////////////////////////
    

    /**
     * Viewport
     */

    /**
     * Draw Calls
     */

     ///////////////////////////////
     ///    Destroy Handles      ///
     ///////////////////////////////

    int a = 0;
}
