#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <exception>
#include <iostream>

#include <imm.h>
#pragma comment (lib ,"imm32.lib")

#include "Config.h"
#include "TestScene.h"

static HINSTANCE g_HInstance;
static HWND g_Wnd;

ATOM                CustomRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

static HRESULT InitD3DDevice();
static void ShutdownD3DDevice();
static void InitRenderResource(TestScene& scene);
static void RenderFrame(TestScene& scene);

static D3D_FEATURE_LEVEL g_D3D11FeatureLevel = D3D_FEATURE_LEVEL_11_0;
static ID3D11Device* g_D3D11Device = nullptr;
static ID3D11DeviceContext* g_D3D11Ctx = nullptr;
static IDXGISwapChain* g_D3D11SwapChain = nullptr;
static ID3D11RenderTargetView* g_D3D11RenderTarget = nullptr;
static ID3D11VertexShader* g_VertexShader;
static ID3D11PixelShader* g_PixelShader;
static ID3D11ComputeShader* g_ComputeShader;
static ID3D11Texture2D* g_BackbufferTexture1;
static ID3D11Texture2D* g_BackbufferTexture2;
static ID3D11ShaderResourceView* g_BackbufferSRV1;
static ID3D11ShaderResourceView* g_BackbufferSRV2;
static ID3D11UnorderedAccessView* g_BackbufferUAV1;
static ID3D11UnorderedAccessView* g_BackbufferUAV2;
static ID3D11SamplerState* g_SamplerLinear;
static ID3D11RasterizerState* g_RasterState;

static int g_BackbufferIndex = 0;
static int s_FrameCount = 0;

static ID3D11Buffer* g_DataParams;
static ID3D11ShaderResourceView* g_SRVParams;
static ID3D11Buffer* g_DataSpheres;     
static ID3D11ShaderResourceView* g_SRVSpheres;
static ID3D11Buffer* g_DataMaterials;
static ID3D11ShaderResourceView* g_SRVMaterials;
static ID3D11Buffer* g_DataCounter;
static ID3D11UnorderedAccessView* g_UAVCounter;

static ID3D11Query* g_QueryBegin, * g_QueryEnd, * g_QueryDisjoint;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    CustomRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    if (FAILED(InitD3DDevice()))
    {
        ShutdownD3DDevice();
        return 0;
    }

    TestScene scene;
    InitRenderResource(scene);

    // Main message loop
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            RenderFrame(scene);
        }
    }

    ShutdownD3DDevice();

    return (int)msg.wParam;
}

ATOM CustomRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TestClass";
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_HInstance = hInstance;
    RECT rc = { 0, 0, kBackbufferWidth, kBackbufferHeight };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, style, FALSE);
    HWND hWnd = CreateWindowW(L"TestClass", L"Test", style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
        return FALSE;
    g_Wnd = hWnd;
    ImmAssociateContext(hWnd, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static HRESULT InitD3DDevice()
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
    UINT createDeviceFlags = 0;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &g_D3D11Device, &g_D3D11FeatureLevel, &g_D3D11Ctx);
    if (FAILED(hr))
        return hr;

    // Get DXGI factory
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_D3D11Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    RECT rc;
    GetClientRect(g_Wnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_Wnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    hr = dxgiFactory->CreateSwapChain(g_D3D11Device, &sd, &g_D3D11SwapChain);

    // Prevent Alt-Enter
    dxgiFactory->MakeWindowAssociation(g_Wnd, DXGI_MWA_NO_ALT_ENTER);
    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // RTV
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_D3D11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return hr;
    hr = g_D3D11Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_D3D11RenderTarget);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    g_D3D11Ctx->OMSetRenderTargets(1, &g_D3D11RenderTarget, nullptr);

    // Viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_D3D11Ctx->RSSetViewports(1, &vp);

    return S_OK;
}

static void ShutdownD3DDevice()
{
    if (g_D3D11Ctx) g_D3D11Ctx->ClearState();

    if (g_D3D11RenderTarget) g_D3D11RenderTarget->Release();
    if (g_D3D11SwapChain) g_D3D11SwapChain->Release();
    if (g_D3D11Ctx) g_D3D11Ctx->Release();
    if (g_D3D11Device) g_D3D11Device->Release();
}

void InitRenderResource(TestScene& scene)
{
    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    ID3DBlob* computeShaderBlob = nullptr;
    ID3DBlob* errors = nullptr;

#ifdef _DEBUG
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    HRESULT VertexShaderLoaded = D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob);
    HRESULT PixelShaderLoaded = D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob);  
    HRESULT ComputeShaderLoaded = D3DReadFileToBlob(L"ComputeShader.cso", &computeShaderBlob);

    if (VertexShaderLoaded != S_OK || PixelShaderLoaded != S_OK || ComputeShaderLoaded != S_OK)
    {
        try
        {
            D3DCompileFromFile(L"VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                               "main", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errors);
            D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                               "main", "ps_5_0", compileFlags, 0, &pixelShaderBlob, &errors);
            D3DCompileFromFile(L"ComputeShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                               "main", "cs_5_0", compileFlags, 0, &computeShaderBlob, &errors);
        }
        catch (const std::exception& e)
        {
            const char* errStr = (const char*)errors->GetBufferPointer();
            std::cout << errStr << std::endl;
        }
    }

    g_D3D11Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                      vertexShaderBlob->GetBufferSize(),
                                      NULL, &g_VertexShader);

    g_D3D11Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
                                     pixelShaderBlob->GetBufferSize(),
                                     NULL, &g_PixelShader);

    g_D3D11Device->CreateComputeShader(computeShaderBlob->GetBufferPointer(),
                                       computeShaderBlob->GetBufferSize(),
                                       NULL, &g_ComputeShader);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = kBackbufferWidth;
    texDesc.Height = kBackbufferHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    g_D3D11Device->CreateTexture2D(&texDesc, NULL, &g_BackbufferTexture1);
    g_D3D11Device->CreateTexture2D(&texDesc, NULL, &g_BackbufferTexture2);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_D3D11Device->CreateShaderResourceView(g_BackbufferTexture1, &srvDesc, &g_BackbufferSRV1);
    g_D3D11Device->CreateShaderResourceView(g_BackbufferTexture2, &srvDesc, &g_BackbufferSRV2);

    D3D11_SAMPLER_DESC smpDesc = {};
    smpDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    smpDesc.AddressU = smpDesc.AddressV = smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    g_D3D11Device->CreateSamplerState(&smpDesc, &g_SamplerLinear);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    g_D3D11Device->CreateRasterizerState(&rasterDesc, &g_RasterState);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    g_D3D11Device->CreateUnorderedAccessView(g_BackbufferTexture1, &uavDesc, &g_BackbufferUAV1);
    g_D3D11Device->CreateUnorderedAccessView(g_BackbufferTexture2, &uavDesc, &g_BackbufferUAV2);

    D3D11_BUFFER_DESC bdesc = {};
    bdesc.Usage = D3D11_USAGE_DEFAULT;
    bdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bdesc.CPUAccessFlags = 0;
    bdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;

    bdesc.ByteWidth = sizeof(ComputeParams);
    bdesc.StructureByteStride = sizeof(ComputeParams);
    g_D3D11Device->CreateBuffer(&bdesc, NULL, &g_DataParams);
    srvDesc.Buffer.NumElements = 1;
    g_D3D11Device->CreateShaderResourceView(g_DataParams, &srvDesc, &g_SRVParams);

    bdesc.ByteWidth = scene.GetSphereSize() * sizeof(Sphere);
    bdesc.StructureByteStride = sizeof(Sphere);
    g_D3D11Device->CreateBuffer(&bdesc, NULL, &g_DataSpheres);
    srvDesc.Buffer.NumElements = scene.GetSphereSize();
    g_D3D11Device->CreateShaderResourceView(g_DataSpheres, &srvDesc, &g_SRVSpheres);

    bdesc.ByteWidth = scene.GetMaterialSize() * sizeof(Material);
    bdesc.StructureByteStride = sizeof(Material);
    g_D3D11Device->CreateBuffer(&bdesc, NULL, &g_DataMaterials);
    srvDesc.Buffer.NumElements = scene.GetMaterialSize();
    g_D3D11Device->CreateShaderResourceView(g_DataMaterials, &srvDesc, &g_SRVMaterials);

    bdesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    bdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    bdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bdesc.ByteWidth = 4;
    g_D3D11Device->CreateBuffer(&bdesc, NULL, &g_DataCounter);
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = 1;
    g_D3D11Device->CreateUnorderedAccessView(g_DataCounter, &uavDesc, &g_UAVCounter);

    D3D11_QUERY_DESC qDesc = {};
    qDesc.Query = D3D11_QUERY_TIMESTAMP;
    g_D3D11Device->CreateQuery(&qDesc, &g_QueryBegin);
    g_D3D11Device->CreateQuery(&qDesc, &g_QueryEnd);
    qDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    g_D3D11Device->CreateQuery(&qDesc, &g_QueryDisjoint);
}

static void RenderFrame(TestScene& scene)
{
    ComputeParams dataParams;
    dataParams.frames = s_FrameCount;
    dataParams.lerpFactor = float(s_FrameCount) / float(s_FrameCount + 1);
    dataParams.camera = MakeCamera(float3(13, 2, 3), float3(0, 0, 0), float3(0, 1, 0), 20, float(kBackbufferWidth) / float(kBackbufferHeight), 0.1, 10);
    //dataParams.camera = MakeCamera(float3(3, 3, 2), float3(0, 0, -1), float3(0, 1, 0), 20, float(kBackbufferWidth) / float(kBackbufferHeight), 0.5, 5.2);
    dataParams.count = scene.GetSphereSize();
    g_D3D11Ctx->UpdateSubresource(g_DataParams, 0, NULL, &dataParams, 0, 0);

    void* dataSpheres = malloc(scene.GetSphereSize() * sizeof(Sphere));
    void* dataMaterials = malloc(scene.GetMaterialSize() * sizeof(Material));
    scene.GetData(dataSpheres, dataMaterials);
    g_D3D11Ctx->UpdateSubresource(g_DataSpheres, 0, NULL, dataSpheres, 0, 0);
    g_D3D11Ctx->UpdateSubresource(g_DataMaterials, 0, NULL, dataMaterials, 0, 0);
    free(dataSpheres);
    free(dataMaterials);

    g_BackbufferIndex = 1 - g_BackbufferIndex;
    g_D3D11Ctx->CSSetShader(g_ComputeShader, NULL, 0);
    ID3D11ShaderResourceView* srvs[] = {
        g_BackbufferIndex == 0 ? g_BackbufferSRV2 : g_BackbufferSRV1,
        g_SRVParams,
        g_SRVSpheres,
        g_SRVMaterials
    };
    g_D3D11Ctx->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
    ID3D11UnorderedAccessView* uavs[] = {
        g_BackbufferIndex == 0 ? g_BackbufferUAV1 : g_BackbufferUAV2,
        g_UAVCounter
    };
    g_D3D11Ctx->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, NULL);
    g_D3D11Ctx->Begin(g_QueryDisjoint);
    g_D3D11Ctx->End(g_QueryBegin);
    g_D3D11Ctx->Dispatch(kBackbufferWidth / kCSGroupSizeX, kBackbufferHeight / kCSGroupSizeY, 1);
    g_D3D11Ctx->End(g_QueryEnd);
    uavs[0] = NULL;
    g_D3D11Ctx->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, NULL);

    g_D3D11Ctx->VSSetShader(g_VertexShader, NULL, 0);
    g_D3D11Ctx->PSSetShader(g_PixelShader, NULL, 0);
    g_D3D11Ctx->PSSetShaderResources(0, 1, g_BackbufferIndex == 0 ? &g_BackbufferSRV1 : &g_BackbufferSRV2);
    g_D3D11Ctx->PSSetSamplers(0, 1, &g_SamplerLinear);
    g_D3D11Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_D3D11Ctx->RSSetState(g_RasterState);
    g_D3D11Ctx->Draw(3, 0);
    g_D3D11SwapChain->Present(0, 0);
    ++s_FrameCount;
    g_D3D11Ctx->End(g_QueryDisjoint);

    while (g_D3D11Ctx->GetData(g_QueryDisjoint, NULL, 0, 0) == S_FALSE) { Sleep(0); }
    D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
    g_D3D11Ctx->GetData(g_QueryDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
    if (!tsDisjoint.Disjoint)
    {
        UINT64 tsBegin, tsEnd;
        // Note: on some GPUs/drivers, even when the disjoint query above already said "yeah I have data",
        // might still not return "I have data" for timestamp queries before it.
        while (g_D3D11Ctx->GetData(g_QueryBegin, &tsBegin, sizeof(tsBegin), 0) == S_FALSE) { Sleep(0); }
        while (g_D3D11Ctx->GetData(g_QueryEnd, &tsEnd, sizeof(tsEnd), 0) == S_FALSE) { Sleep(0); }

        static float s_Time;
        s_Time += float(tsEnd - tsBegin) / float(tsDisjoint.Frequency);

        static uint64_t s_RayCounter;
        D3D11_MAPPED_SUBRESOURCE mapped;
        g_D3D11Ctx->Map(g_DataCounter, 0, D3D11_MAP_READ, 0, &mapped);
        s_RayCounter += *(const int*)mapped.pData;
        g_D3D11Ctx->Unmap(g_DataCounter, 0);
        int zeroCount = 0;
        g_D3D11Ctx->UpdateSubresource(g_DataCounter, 0, NULL, &zeroCount, 0, 0);

        static float s_Count;
        if (++s_Count > 150)
        {
            float avgTime = s_Time / s_Count;
            float avgRayCounter = s_RayCounter / s_Count;
            char s_Buffer[200];
            sprintf_s(s_Buffer, sizeof(s_Buffer), "%.2fms (%.1f FPS) %.1fMrays/s %.2fMrays/frame frames %i\n",
                      avgTime * 1000.0f,
                      1.f / avgTime,
                      avgRayCounter / avgTime * 1.0e-6f,
                      avgRayCounter * 1.0e-6f,
                      s_FrameCount);
            SetWindowTextA(g_Wnd, s_Buffer);
            s_Count = 0;
            s_Time = 0;
            s_RayCounter = 0;
        }
    }
}
