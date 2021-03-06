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
    ComPtr<IDXGIFactory4> factory;

    // Create Factory
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    {
        // Create a Debug Controller to track errors
        ComPtr<ID3D12Debug> dc;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(dc.GetAddressOf())));

        ComPtr<ID3D12Debug1> debugController;
        ThrowIfFailed(dc.As(&debugController));
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(true);

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory.GetAddressOf())));
    
    /**
     * Adapter
     */
    ComPtr<IDXGIAdapter1> adapter;

    // Create Adapter
    for (UINT adapterIndex = 0;
        DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
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
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
            _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }
    if (!adapter)
        throw std::runtime_error("No adapter");
    
    /**
     * Device
     */
     // Declare Handles
    ComPtr<ID3D12Device> device;

    // Create Device
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf())));
    
#if defined(_DEBUG)
    // Debug mode
    ComPtr<ID3D12DebugDevice> debugDevice;
    ThrowIfFailed(device.As(&debugDevice));
#endif

    /**
     * Command Queue
     */
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ComPtr<ID3D12CommandQueue> commandQueue;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));

    /**
     * Command Allocator
     */
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.GetAddressOf())));

    /**
     * Synchronization
     */
    UINT frameIndex;
    HANDLE fenceEvent;
    UINT64 fenceValue = 1;

    // Create fence
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
    
    /**
     * Swapchain
     */

    // Declare Handles
    static const UINT backbufferCount = 2;
    UINT currentBuffer;
    ComPtr<ID3D12DescriptorHeap> renderTargetViewHeap;
    ComPtr<ID3D12Resource> renderTargets[backbufferCount];
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

    ComPtr<IDXGISwapChain3> swapchain = nullptr;
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
        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapchainDesc, nullptr, nullptr, swapChain1.GetAddressOf()));

        // Cast to IDXGISwapChain3 interface
        ThrowIfFailed(swapChain1.As(&swapchain));
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
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(renderTargetViewHeap.GetAddressOf())));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create frame resources
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());
    
    // Create a RTV for each frame.
    for (UINT n = 0; n < backbufferCount; n++)
    {
        ThrowIfFailed(swapchain->GetBuffer(n, IID_PPV_ARGS(renderTargets[n].GetAddressOf())));
        device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += (1 * rtvDescriptorSize);
    }

    /**
     * Root Signature
     */
     // Declare Handles
    ComPtr<ID3D12RootSignature> rootSignature;

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

    {
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        try
        {
            // Create the root signature
            ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf()), error);
            ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf())));
            rootSignature->SetName(L"Hello Triangle Root Signature");
        }
        catch (const std::exception& e)
        {
            const char* errStr = (const char*)error->GetBufferPointer();
            std::cout << errStr;
        }
    }

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
    // Declare Handles
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    {
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
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertexBuffer.GetAddressOf())));

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
     // Declare Data
    uint32_t indexBufferData[3] = { 0, 1, 2 };

    // Declare Handles
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    {
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
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(indexBuffer.GetAddressOf())));

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
        float4x4 projectionMatrix = float4x4::Identity();
        float4x4 modelMatrix      = float4x4::Identity();
        float4x4 viewMatrix       = float4x4::Identity();
    } cbVS;

    // Declare Handles
    ComPtr<ID3D12Resource> constantBuffer;
    ComPtr<ID3D12DescriptorHeap> constantBufferHeap;
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
            IID_PPV_ARGS(constantBufferHeap.GetAddressOf())));

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
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(constantBuffer.GetAddressOf())));
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

    ComPtr<ID3DBlob> errors;

    /**
     * Vertex Shader
     */
    ComPtr<ID3DBlob> vertexShaderBlob;
    std::wstring pathVS = exePath / "shaders" / "triangle.vs.hlsl";
    ThrowIfFailed(D3DCompileFromFile(pathVS.c_str(), nullptr, nullptr, "main",
        "vs_5_0", compileFlags, 0, vertexShaderBlob.GetAddressOf(),
        errors.ReleaseAndGetAddressOf()), errors);
        
    /**
     * Pixel Shader
     */
    ComPtr<ID3DBlob> pixelShaderBlob;
    std::wstring pathPS = exePath / "shaders" / "triangle.ps.hlsl";
    ThrowIfFailed(D3DCompileFromFile(pathPS.c_str(), nullptr, nullptr, "main",
        "ps_5_0", compileFlags, 0, pixelShaderBlob.GetAddressOf(),
        errors.ReleaseAndGetAddressOf()), errors);

    errors.Reset();
    
    /**
     * Pipeline State
     */
     // Declare handles
    ComPtr<ID3D12PipelineState> pipelineState;

    // Define the Graphics Pipeline
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    // Input Assembly
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

    // Resources
    psoDesc.pRootSignature = rootSignature.Get();

    // Vertex Shader
    D3D12_SHADER_BYTECODE vsBytecode;
    vsBytecode.pShaderBytecode = vertexShaderBlob->GetBufferPointer();
    vsBytecode.BytecodeLength = vertexShaderBlob->GetBufferSize();
    psoDesc.VS = vsBytecode;

    // Pixel Shader
    D3D12_SHADER_BYTECODE psBytecode;
    psBytecode.pShaderBytecode = pixelShaderBlob->GetBufferPointer();
    psBytecode.BytecodeLength = pixelShaderBlob->GetBufferSize();
    psoDesc.PS = psBytecode;

    // Rasterization
    D3D12_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;
    rasterDesc.ForcedSampleCount = 0;
    rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Color/Blend
    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
    psoDesc.BlendState = blendDesc;

    // Depth/Stencil State
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;

    // Output
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    // Create the raster pipeline state
    try
    {
        ThrowIfFailed(device->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf())));
    }
    catch (std::exception& e)
    {
        std::cout << "Failed to create Graphics Pipeline!";
    }

    ///////////////////////////////
    ///   Encoding Commands     ///
    ///////////////////////////////
    // Declare handles
    ComPtr<ID3D12PipelineState> initialPipelineState;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    // Create the command list.
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(), initialPipelineState.Get(),
        IID_PPV_ARGS(commandList.GetAddressOf())));

    // Command lists are created in the recording state, but there is nothing to record yet.
    ThrowIfFailed(commandList->Close());



    // Reset the command list and add new commands.
    ThrowIfFailed(commandAllocator->Reset());


    // Begin using the Raster Graphics Pipeline
    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

    // Setup Resources
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    ID3D12DescriptorHeap* pDescriptorHeaps[] = { constantBufferHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);
    D3D12_GPU_DESCRIPTOR_HANDLE
        cbvHandle(constantBufferHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

    // Indicate that the back buffer will be used as a render target.
    D3D12_RESOURCE_BARRIER renderTargetBarrier;
    renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    renderTargetBarrier.Transition.pResource = renderTargets[frameIndex].Get();
    renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    renderTargetBarrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &renderTargetBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleOM(renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandleOM.ptr = rtvHandleOM.ptr + (frameIndex * rtvDescriptorSize);
    commandList->OMSetRenderTargets(1, &rtvHandleOM, FALSE, nullptr);

    // Record raster commands.
    const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &surfaceSize);
    commandList->ClearRenderTargetView(rtvHandleOM, clearColor, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

    // Indicate that the back buffer will now be used to present.
    D3D12_RESOURCE_BARRIER presentBarrier;
    presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    presentBarrier.Transition.pResource = renderTargets[frameIndex].Get();
    presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &presentBarrier);

    ThrowIfFailed(commandList->Close());


    ///////////////////////////////
    ///       Rendering         ///
    ///////////////////////////////
    
    /**
     * Update Uniforms
     */
    D3D12_RANGE readRange;
    readRange.Begin = 0;
    readRange.End = 0;

    ThrowIfFailed(constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
    memcpy(mappedConstantBuffer, &cbVS, sizeof(cbVS));
    constantBuffer->Unmap(0, &readRange);

    //setupCommands();

    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    /**
     * Present
     */
    swapchain->Present(1, 0);


    // Create an event handle to use for frame synchronization.
    fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

    // then wait till finished to continue execution
    const UINT64 fenceV = fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceV));
    fenceValue++;

    if (fence->GetCompletedValue() < fenceV)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceV, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapchain->GetCurrentBackBufferIndex();

    ///////////////////////////////
    ///    Destroy Handles      ///
    ///////////////////////////////
    CloseHandle(fenceEvent);
}
