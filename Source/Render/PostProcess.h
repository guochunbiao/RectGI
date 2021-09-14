#pragma once

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include <vector>
#include <string>
#include <map>
#include <d3d11.h>

using namespace std;
using namespace DirectX;

// Number of stages in the 3x3 down-scaling for post-processing in PS
#define NUM_TONEMAP_TEXTURES  5

// Stuff used for drawing the "full screen quad"
struct SCREEN_VERTEX
{
	XMFLOAT4 mPos;
	XMFLOAT2 mTex;
};

// Class for rendering post processes.
class CPostProcess
{
private:
	CPostProcess();

public:
	static CPostProcess& GetInstance();

	// Render HDR.
	void RenderHDR(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);

	// Create resources for post process rendering.
	void CreateResources(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
		const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
	// Release render resources.
	void ReleaseResources();

private:
	// Tone mapping.
	void ToneMapping(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	// Draw full screen quad for post process.
	void DrawFullScreenQuad11(ID3D11DeviceContext* pd3dImmediateContext,
		ID3D11PixelShader* pPS, UINT Width, UINT Height);

	// Measure luminance.
	HRESULT MeasureLuminancePS11(ID3D11DeviceContext* pd3dImmediateContext);

private:
	// texture
	ID3D11Texture2D* mTexRender;
	// render target view
	ID3D11RenderTargetView* mTexRenderRTV;
	// shader resource view
	ID3D11ShaderResourceView* mTexRenderRV;

	// Tone mapping calculation textures used in PS path
	ID3D11Texture2D* mTexToneMap[NUM_TONEMAP_TEXTURES]; 
	ID3D11ShaderResourceView* mTexToneMapRV[NUM_TONEMAP_TEXTURES];
	ID3D11RenderTargetView* mTexToneMapRTV[NUM_TONEMAP_TEXTURES];

	// Shaders in PS path
	ID3D11PixelShader* mDownScale2x2LumPS;
	ID3D11PixelShader* mDownScale3x3PS;
	ID3D11PixelShader* mFinalPassPS;

	// vertex buffer
	ID3D11Buffer* mScreenQuadVB;
	// vertex layout
	ID3D11InputLayout* mQuadLayout;
	// vertex shader
	ID3D11VertexShader* mQuadVS;
};