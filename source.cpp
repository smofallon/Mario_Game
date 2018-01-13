//--------------------------------------------------------------------------------------
// File: Tutorial022.cpp
//
// This application displays a triangle using Direct3D 11
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include "resource.h"

struct VS_CONSTANT_BUFFER
	{
	float some_variable_a;
	float some_variable_b;
	float some_variable_c;
	float some_variable_d;
	};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11VertexShader*     g_pVertexShader = NULL;
ID3D11PixelShader*      g_pPixelShader = NULL;
ID3D11InputLayout*      g_pVertexLayout = NULL;
ID3D11Buffer*           g_pVertexBuffer = NULL;
ID3D11Buffer*			g_pConstantBuffer11 = NULL;
ID3D11BlendState*					g_BlendState;
ID3D11SamplerState*                 g_Sampler = NULL;
float background_position = 0;
int deathCount = 0;
float ground = -0.713;
float blockHeight = 0.144;
bool win = false;

ID3D11ShaderResourceView*           g_Texture = NULL;
ID3D11ShaderResourceView*           g_Tex_BG= NULL;


VS_CONSTANT_BUFFER VsConstData;
//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 2: Rendering a Triangle",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"shader.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
		
        XMFLOAT3( -0.5f, 0.5f, 0.5f ),
		XMFLOAT2(0.0f, 0.0f),
        XMFLOAT3( 0.5f, 0.5f, 0.5f ),
		XMFLOAT2(0.1f, 0.0f),
        XMFLOAT3( 0.5f, -0.5f, 0.5f ),
		XMFLOAT2(0.1f, 1.0f),

		XMFLOAT3(-0.5f, 0.5f, 0.5f),
		XMFLOAT2(0.0f, 0.0f),
		XMFLOAT3(0.5f, -0.5f, 0.5f),
		XMFLOAT2(0.1f, 1.0f),
		XMFLOAT3(-0.5f, -0.5f, 0.5f),
		XMFLOAT2(0.0f, 1.0f),
		
    };
    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 6;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );


	// Supply the vertex shader constant data.
	
	VsConstData.some_variable_a = 0;
	VsConstData.some_variable_b = 0;
	VsConstData.some_variable_c = 1;
	VsConstData.some_variable_d = 1;
	
	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Create the buffer.
	hr = g_pd3dDevice->CreateBuffer(&cbDesc, NULL,	&g_pConstantBuffer11);

	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"mario.png", NULL, NULL, &g_Texture, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"mario-1-1.png", NULL, NULL, &g_Tex_BG, NULL);
	if (FAILED(hr))
		return hr;

	// Create the sample state

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_Sampler);
	if (FAILED(hr))
		return hr;
	
	//blendstate:
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);


	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{

	
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}

///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{

}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{

}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
{

}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////
void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
{

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
	{
	}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
	{
	}
}

//*************************************************************************
class sprite
{
public:
	XMFLOAT2 position;
	XMFLOAT2 impulse;
	sprite()
	{
		position = XMFLOAT2(0, 0);
		impulse = XMFLOAT2(0, 0);
	}
};
//***************************************************
sprite player, bg;
BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	player.position.y = ground;
	player.position.x = -0.6;
	return TRUE;
}
void OnTimer(HWND hwnd, UINT id)
{
	
}

void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
		case 65: //a
			bg.impulse.x = 0;
		break;

		case 68: //d
			bg.impulse.x = 0;
		break;

		case 37: //left arrow
			player.impulse.x = 0;
		break;

		case 39: //right arrow
			player.impulse.x = 0;
		break;

		case 27: // esc
			PostQuitMessage(0);
		break;

		case 32: //space
			player.impulse.y /= 2;
		break;

	default:break;

	}

}
void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	if (win == false) {
		switch (vk)
		{
		default:break;
		case 65: //a
			if (background_position < -0.15)
				bg.impulse.x = 0.0005;
			else
				bg.impulse.x = 0;
			break;

		case 68: //d
			bg.impulse.x = -0.0005;
			break;

		case 37: //left arrow
			player.impulse.x = -0.0005;
			break;

		case 39: //right arrow
			player.impulse.x = 0.0005;
			break;

		case 32: //space
			if (player.impulse.y != -0.0011 && (player.position.y == ground || player.position.y == (ground + blockHeight) || player.position.y == (ground + blockHeight * 2) ||
				player.position.y == (ground + blockHeight * 3) || player.position.y == (ground + blockHeight * 4) ||
				player.position.y == (ground + blockHeight * 5) || player.position.y == (ground + blockHeight * 6) ||
				player.position.y == (ground + blockHeight * 7) || player.position.y == (ground + blockHeight * 8))) {
				player.impulse.y = 0.0011;
			}
			break;
		}
	}
}
//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	SCROLLINFO si;



	switch (message)
	{
		/*
		#define HANDLE_MSG(hwnd, message, fn)    \
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (fn))
		*/

		//HANDLE_MSG(hwnd, WM_CHAR, OnChar);
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLBD);
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLBU);
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMM);
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKeyDown);
		HANDLE_MSG(hwnd, WM_KEYUP, OnKeyUp);

	case WM_ERASEBKGND:
		return (LRESULT)1;
	case WM_DESTROY:

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
//*************************************************
bool platform1() {
	if (player.position.y >= (ground + blockHeight - 0.05) && player.position.y < (ground + blockHeight) && player.impulse.y < 0) {

		// first pair of steps
		if (bg.position.x < -18.35 && bg.position.x > -19.05) {
			return true;
		}
		if (bg.position.x < -19.2 && bg.position.x > -19.9) {
			return true;
		}

		// second pair of steps
		if (bg.position.x < -20.3 && bg.position.x > -21.14) {
			return true;
		}
		if (bg.position.x < -21.35 && bg.position.x > -22) {
			return true;
		}

		// last steps
		if (bg.position.x < -25 && bg.position.x > -26.4) {
			return true;
		}

	}
	return false;
}

bool platform2() {
	if (player.position.y >= (ground + (blockHeight * 2) - 0.05) && player.position.y < ground + (blockHeight * 2) && player.impulse.y < 0) {
		
		// first pipe
		if (bg.position.x < -3.3 && bg.position.x > -3.75) {
			return true;
		}

		// last 2 pipes
		if (bg.position.x < -22.45 && bg.position.x > -22.87) {
			return true;
		}
		if (bg.position.x < -24.67 && bg.position.x > -25.1) {
			return true;
		}

		// first pair of steps
		if (bg.position.x < -18.5 && bg.position.x > -19.05) {
			return true;
		}
		if (bg.position.x < -19.2 && bg.position.x > -19.75) {
			return true;
		}

		// second pair of steps
		if (bg.position.x < -20.45 && bg.position.x > -21.14) {
			return true;
		}
		if (bg.position.x < -21.35 && bg.position.x > -21.85) {
			return true;
		}

		// last steps
		if (bg.position.x < -25.17 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform3() {
	if (player.position.y >= (ground + (blockHeight * 3) - 0.05) && player.position.y < ground + (blockHeight * 3) && player.impulse.y < 0) {

		// pipe
		if (bg.position.x < -4.7 && bg.position.x > -5.15) {
			return true;
		}

		// first pair of steps
		if (bg.position.x < -18.65 && bg.position.x > -19.05) {
			return true;
		}
		if (bg.position.x < -19.2 && bg.position.x > -19.6) {
			return true;
		}

		// second pair of steps
		if (bg.position.x < -20.6 && bg.position.x > -21.14) {
			return true;
		}
		if (bg.position.x < -21.35 && bg.position.x > -21.7) {
			return true;
		}

		// last steps
		if (bg.position.x < -25.3 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform4() {

	// stand on blocks position 4
	if (player.position.y >= (ground + (blockHeight * 4) - 0.05) && player.position.y < ground + (blockHeight * 4) && player.impulse.y < 0) {

		//first '?' block
		if (bg.position.x < -1.6 && bg.position.x > -1.9) {
			return true;
		}

		//second set of blocks
		if (bg.position.x < -2.2 && bg.position.x > -3) {
			return true;
		}

		// pipes
		if ((bg.position.x < -5.85 && bg.position.x > -6.3) || (bg.position.x < -7.4 && bg.position.x > -7.85)) {
			return true;
		}

		// more blocks
		if (bg.position.x < -10.2 && bg.position.x > -10.8) {
			return true;
		}
		if (bg.position.x < -12.65 && bg.position.x > -12.95) {
			return true;
		}
		if (bg.position.x < -13.5 && bg.position.x > -13.95) {
			return true;
		}

		// '?' block cluster
		if (bg.position.x < -14.35 && bg.position.x > -14.65) {
			return true;
		}
		if (bg.position.x < -14.8 && bg.position.x > -15.1) {
			return true;
		}
		if (bg.position.x < -15.2 && bg.position.x > -15.5) {
			return true;
		}

		// single block
		if (bg.position.x < -16.05 && bg.position.x > -16.35) {
			return true;
		}

		// double blocks
		if (bg.position.x < -17.6 && bg.position.x > -18.05) {
			return true;
		}

		// last blocks
		if (bg.position.x < -23.15 && bg.position.x > -23.87) {
			return true;
		}

		// first pair of steps
		if (bg.position.x < -18.8 && bg.position.x > -19.05) {
			return true;
		}
		if (bg.position.x < -19.2 && bg.position.x > -19.45) {
			return true;
		}

		// second pair of steps
		if (bg.position.x < -20.75 && bg.position.x > -21.14) {
			return true;
		}
		if (bg.position.x < -21.35 && bg.position.x > -21.6) {
			return true;
		}

		// last steps
		if (bg.position.x < -25.45 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform5() {
	if (player.position.y >= (ground + (blockHeight * 5) - 0.05) && player.position.y < ground + (blockHeight * 5) && player.impulse.y < 0) {

		// first '?' block
		if (bg.position.x < -8.4 && bg.position.x > -8.7) {
			return true;
		}

		// last steps
		if (bg.position.x < -25.55 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform6() {
	if (player.position.y >= (ground + (blockHeight * 6) - 0.05) && player.position.y < ground + (blockHeight * 6) && player.impulse.y < 0) {

		// last steps
		if (bg.position.x < -25.7 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform7() {
	if (player.position.y >= (ground + (blockHeight * 7) - 0.05) && player.position.y < ground + (blockHeight * 7) && player.impulse.y < 0) {

		// last steps
		if (bg.position.x < -25.85 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool platform8() {
	if (player.position.y >= (ground + (blockHeight * 8) - 0.05) && player.position.y < ground + (blockHeight * 8) && player.impulse.y < 0) {

		// first '?' block
		if (bg.position.x < -2.45 && bg.position.x > -2.75) {
			return true;
		}

		// first walkway set
		if (bg.position.x < -10.8 && bg.position.x > -12) {
			return true;
		}
		if (bg.position.x < -12.25 && bg.position.x > -12.95) {
			return true;
		}

		// '?' block cluster
		if (bg.position.x < -14.8 && bg.position.x > -15.1) {
			return true;
		}

		// last blocks
		if (bg.position.x < -16.5 && bg.position.x > -17.07) {
			return true;
		}
		if (bg.position.x < -17.47 && bg.position.x > -18.2) {
			return true;
		}

		// last steps
		if (bg.position.x < -26 && bg.position.x > -26.4) {
			return true;
		}
	}
	return false;
}

bool flag() {

	if (bg.position.x < -27.55 && bg.position.x > -27.65) {
		
		win = true;
		bg.impulse.x = 0;
		player.impulse.y = -0.0002;

		return true;
	}

	return false;
}

void animation(){
	background_position += bg.impulse.x;
	player.position.x += player.impulse.x;
	player.position.y += player.impulse.y;
	bg.position.x = bg.position.x + bg.impulse.x;

	player.impulse.y -= 0.000001;

	// stand on blocks and pipes
	if (platform1()) {
		player.position.y = ground + blockHeight;
		player.impulse.y = 0;
	}
	else if (platform2()) {
		player.position.y = ground + (blockHeight * 2);
		player.impulse.y = 0;
	}
	else if (platform3()) {
		player.position.y = ground + (blockHeight * 3);
		player.impulse.y = 0;
	}
	else if (platform4()) {
		player.position.y = ground + (blockHeight * 4);
		player.impulse.y = 0;
	}
	else if (platform5()) {
		player.position.y = ground + (blockHeight * 5);
		player.impulse.y = 0;
	}
	else if (platform6()) {
		player.position.y = ground + (blockHeight * 6);
		player.impulse.y = 0;
	}
	else if (platform7()) {
		player.position.y = ground + (blockHeight * 7);
		player.impulse.y = 0;
	}
	else if (platform8()) {
		player.position.y = ground + (blockHeight * 8);
		player.impulse.y = 0;
	}

	// fall down holes
	if ((player.position.y <= ground && player.position.y > -0.75) && 
		(bg.position.x > -9.2 || bg.position.x < -9.4) &&
		(bg.position.x > -11.6 || bg.position.x < -12) &&
		(bg.position.x > -21.14 || bg.position.x < -21.35)){
			player.position.y = ground;
			player.impulse.y = 0;
	}
	
	// restart level if player dies
	if (player.position.y < -3) { 
		player.position.y = ground;
		player.position.x = -0.6;
		bg.position.x = 0;
		background_position = 0;
		deathCount++;
	}

	// quit after losing 3 lives or player reaches castle
	if (player.position.y < -1.5 && deathCount >= 2 || bg.position.x < -28.4)
		PostQuitMessage(0);

	flag();
	if (win == true && player.position.y == ground)
		bg.impulse.x = -0.0003;
}
//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------


void Render()
{
// Clear the back buffer 
float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	float scale_factor=1.0;
	animation();

	//for all rectangles:
	g_pImmediateContext->PSSetSamplers(0, 1, &g_Sampler);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	//draw background
	scale_factor = 2.0;
	VsConstData.some_variable_a = bg.position.x;
	VsConstData.some_variable_b = bg.position.y;
	VsConstData.some_variable_c = scale_factor;
	VsConstData.some_variable_d = background_position;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_Tex_BG);
	g_pImmediateContext->Draw(6, 0);

	//draw player
	scale_factor = 0.15;
	VsConstData.some_variable_a = player.position.x;
	VsConstData.some_variable_b = player.position.y;
	VsConstData.some_variable_c = scale_factor;
	VsConstData.some_variable_d = 1000;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
    
   	g_pImmediateContext->PSSetShaderResources(0, 1, &g_Texture);
	g_pImmediateContext->Draw( 6, 0 );	

    // Present the information rendered to the back buffer to the front buffer (the screen)
    g_pSwapChain->Present( 0, 0 );
}
