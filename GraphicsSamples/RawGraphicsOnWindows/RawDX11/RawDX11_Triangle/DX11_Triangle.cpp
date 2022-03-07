#include "DX11_Triangle.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#include "Common/DX.Utils/DxException.h"
#include "Common/Math.Utils/Math.h"
#include "Common/Win.Utils/ExePath.h"

void DX11_Triangle::DrawTriangle(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight)
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
    IDXGIFactory* factory;
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&factory)));

    /**
     * Adapter
     */
    IDXGIAdapter* adapter;
    ThrowIfFailed(factory->EnumAdapters(0, &adapter));

    /**
     * Adapter Output
     */
    IDXGIOutput* adapterOutput;
    ThrowIfFailed(adapter->EnumOutputs(0, &adapterOutput));

    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display
    // format for the adapter output (monitor).
    unsigned int numModes;
    DXGI_MODE_DESC* displayModeList;
    ThrowIfFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes, NULL));

    // Create a list to hold all the possible display modes for this monitor/video
    // card combination.
    displayModeList = new DXGI_MODE_DESC[numModes];
    ThrowIfFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_ENUM_MODES_INTERLACED,
        &numModes, displayModeList));

    // Now go through all the display modes and find the one that matches the screen
    // width and height. When a match is found store the numerator and denominator
    // of the refresh rate for that monitor.
    unsigned numerator = 0;
    unsigned denominator = 1;
    for (size_t i = 0; i < numModes; i++)
    {
        if (displayModeList[i].Width = windowWidth && displayModeList[i].Height == windowHeight)
        {
            numerator = displayModeList[i].RefreshRate.Numerator;
            denominator = displayModeList[i].RefreshRate.Denominator;
            break;
        }
    }

    // Release the display mode list.
    delete[] displayModeList;
    displayModeList = nullptr;

    /**
     * Device
     */
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;

    D3D_FEATURE_LEVEL featureLevelInputs[7] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1 };

    D3D_FEATURE_LEVEL featureLevelOutputs = D3D_FEATURE_LEVEL_11_1;

    ThrowIfFailed(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0,
        D3D11_CREATE_DEVICE_SINGLETHREADED |
        D3D11_CREATE_DEVICE_DEBUG,
        featureLevelInputs, 7u, D3D11_SDK_VERSION,
        &device, &featureLevelOutputs, &deviceContext));


    // Optionally enable the Debug Controller to validate your commands on runtime.
    ID3D11Debug* debugController = nullptr;
#if defined(_DEBUG)
    ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&debugController)));
#endif

    ///////////////////////////////
    ///      Frame Backing      ///
    ///////////////////////////////

    /**
     * Swapchain
     */
    bool isEnabledVSync     = false;
    bool isFullScreenWindow = false;

    DXGI_SWAP_CHAIN_DESC swapchainDesc;
    ZeroMemory(&swapchainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapchainDesc.BufferCount = 1;
    swapchainDesc.BufferDesc.Width = windowWidth;
    swapchainDesc.BufferDesc.Height = windowHeight;
    swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Set the refresh rate of the back buffer.
    if (isEnabledVSync)
    {
        swapchainDesc.BufferDesc.RefreshRate.Numerator = numerator;
        swapchainDesc.BufferDesc.RefreshRate.Denominator = denominator;
    }
    else
    {
        swapchainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
    }
    // Set the usage of the back buffer.
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // Turn multi-sampling off.
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    // Set the scan line ordering and scaling to unspecified.
    swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    // Discard the back buffer contents after presenting.
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    // Don't set the advanced flags.
    swapchainDesc.Flags = 0;

    swapchainDesc.OutputWindow = hWnd;
    swapchainDesc.Windowed = !isFullScreenWindow;

    IDXGISwapChain* swapchain;
    ThrowIfFailed(factory->CreateSwapChain(device, &swapchainDesc, &swapchain));;

    /**
     * Render Targets
     */
    
    // Get the back buffer texture.
    ID3D11Texture2D* texBackBuffer;
    ThrowIfFailed(swapchain->GetBuffer(0, IID_PPV_ARGS(&texBackBuffer)));

    // Create the render target view with the back buffer pointer.
    ID3D11RenderTargetView* rtv;
    ThrowIfFailed(device->CreateRenderTargetView(texBackBuffer, NULL, &rtv));

    // Create the texture for the depth buffer.
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
    depthBufferDesc.Width = windowWidth;
    depthBufferDesc.Height = windowHeight;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;
    ID3D11Texture2D* texDepthStencilBuffer;
    ThrowIfFailed(device->CreateTexture2D(&depthBufferDesc, NULL, &texDepthStencilBuffer));

    // Create the depth stencil view.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    ID3D11DepthStencilView* dsv;
    ThrowIfFailed(device->CreateDepthStencilView(texDepthStencilBuffer, &depthStencilViewDesc, &dsv));

    ///////////////////////////////
    ///       Resources         ///
    ///////////////////////////////

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
    
    // Set up the description of the static vertex buffer.
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the vertex data.
    D3D11_SUBRESOURCE_DATA vertexData;
    vertexData.pSysMem = vertexBufferData;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    // Now create the vertex buffer.
    ID3D11Buffer* vertexBuffer;
    ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer));

    /**
     * Index Buffer
     */

    // Declare Data
    uint32_t indexBufferData[3] = { 0, 1, 2 };

    // Set up the description of the static index buffer.
    D3D11_BUFFER_DESC indexBufferDesc;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned) * 3;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    // Give the subresource structure a pointer to the index data.
    D3D11_SUBRESOURCE_DATA indexData;
    indexData.pSysMem = indexBufferData;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    // Create the index buffer.
    ID3D11Buffer* indexBuffer;
    ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer));

    /**
     * Constant Buffer
     */
    
    struct
    {
        float4x4 projectionMatrix = float4x4::Identity();
        float4x4 modelMatrix = float4x4::Identity();
        float4x4 viewMatrix = float4x4::Identity();
    } cbVS;


    // Setup the description of the dynamic matrix constant buffer that is in the
    // vertex shader.
    D3D11_BUFFER_DESC constantBufferDesc;
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.ByteWidth = sizeof(cbVS);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantBufferDesc.MiscFlags = 0;
    constantBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader
    // constant buffer from within this class.
    ID3D11Buffer* constantBuffer;
    ThrowIfFailed(device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer));


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

    // Create the vertex shader from the buffer.
    ID3D11VertexShader* vertexShader;
    ThrowIfFailed(device->CreateVertexShader(
        vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(),
        nullptr, &vertexShader));
       
    /**
     * Pixel Shader
     */
    ID3DBlob* pixelShaderBlob = nullptr;
    std::wstring pathPS = exePath / "shaders" / "triangle.ps.hlsl";
    ThrowIfFailed(D3DCompileFromFile(pathPS.c_str(), nullptr, nullptr, "main",
        "ps_5_0", compileFlags, 0, &pixelShaderBlob,
        &errors), errors);
    
    // Create the pixel shader from the buffer.
    ID3D11PixelShader* pixelShader;
    ThrowIfFailed(device->CreatePixelShader(
        pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
        nullptr, &pixelShader));

    ///////////////////////////////
    ///       Rendering         ///
    ///////////////////////////////
    
    /**
     * Pipeline State
     */
    // Input Assembly
    D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "COLOR";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    // Get a count of the elements in the layout.
    unsigned numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

    // Create the vertex input layout.
    ID3D11InputLayout* inputLayout;
    ThrowIfFailed(device->CreateInputLayout(polygonLayout, numElements,
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &inputLayout));
    
    // Depth/Stencil
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create the depth stencil state.
    ID3D11DepthStencilState* depthStencilState;
    ThrowIfFailed(device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState));

    // Set the depth stencil state.
    deviceContext->OMSetDepthStencilState(depthStencilState, 1);

    // Rasterization
    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer state from the description we just filled out.
    ID3D11RasterizerState* rasterState;
    ThrowIfFailed(device->CreateRasterizerState(&rasterDesc, &rasterState));

    // Now set the rasterizer state.
    deviceContext->RSSetState(rasterState);

    /**
     * Viewport
     */
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = windowWidth;
    viewport.Height = windowHeight;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    /**
     * Draw Calls
     */

    // Set the position of the constant buffer in the vertex shader.
    unsigned int bufferNumber = 0;

    // Lock the constant buffer so it can be written to.
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ThrowIfFailed(deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD,
        0, &mappedResource));

    // Get a pointer to the data in the constant buffer.
    memcpy(mappedResource.pData, &cbVS, sizeof(cbVS));

    // Unlock the constant buffer.
    deviceContext->Unmap(constantBuffer, 0);

    // Finally set the constant buffer in the vertex shader with the updated
    // values.
    deviceContext->VSSetConstantBuffers(bufferNumber, 1, &constantBuffer);

    // Bind the render target view and depth stencil buffer to the output render
    // pipeline.
    deviceContext->OMSetRenderTargets(1, &rtv, dsv);

    deviceContext->RSSetViewports(1, &viewport);

    // Clear textures
    float color[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    deviceContext->ClearRenderTargetView(rtv, color);

    // Clear the depth buffer.
    deviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Set the vertex input layout.
    deviceContext->IASetInputLayout(inputLayout);

    // Set the vertex and pixel shaders that will be used to render this
    // triangle.
    deviceContext->VSSetShader(vertexShader, NULL, 0);
    deviceContext->PSSetShader(pixelShader, NULL, 0);

    // Set the vertex buffer to active in the input assembler so it can be
    // rendered.
    unsigned stride = sizeof(Vertex);
    unsigned offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be
    // rendered.
    deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex
    // buffer, in this case triangles.
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Render the triangle.
    deviceContext->DrawIndexed(3, 0, 0);

    if (isEnabledVSync)
    {
        swapchain->Present(1, 0);
    }
    else
    {
        swapchain->Present(0, 0);
    }

    ///////////////////////////////
    ///     Release pointers    ///
    ///////////////////////////////
    if (factory)                factory->Release();
    if (adapter)                adapter->Release();
    if (adapterOutput)          adapterOutput->Release();
    if (device)                 device->Release();
    if (deviceContext)          deviceContext->Release();
    if (debugController)        debugController->Release();
    if (swapchain)              swapchain->Release();
    if (texBackBuffer)          texBackBuffer->Release();
    if (rtv)                    rtv->Release();
    if (texDepthStencilBuffer)  texDepthStencilBuffer->Release();
    if (dsv)                    dsv->Release();
    if (vertexBuffer)           vertexBuffer->Release();
    if (indexBuffer)            indexBuffer->Release();
    if (constantBuffer)         constantBuffer->Release();
    if (errors)                 errors->Release();
    if (vertexShaderBlob)       vertexShaderBlob->Release();
    if (vertexShader)           vertexShader->Release();
    if (pixelShaderBlob)        pixelShaderBlob->Release();
    if (pixelShader)            pixelShader->Release();
    if (inputLayout)            inputLayout->Release();
    if (depthStencilState)      depthStencilState->Release();
    if (rasterState)            rasterState->Release();

    return;
}
