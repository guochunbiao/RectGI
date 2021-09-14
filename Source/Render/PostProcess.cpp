#include "DXUT.h"
#include "RenderStates.h"
#include "MiniEngine.h"
#include "PostProcess.h"

#pragma warning( disable : 4100 )

static const int ToneMappingTexSize = (int)pow(3.0f, NUM_TONEMAP_TEXTURES - 1);
#define NUM_BLOOM_TEXTURES 2

CPostProcess::CPostProcess()
	: mTexRender(nullptr)
	, mTexRenderRTV(nullptr)
	, mTexRenderRV(nullptr)
	, mDownScale2x2LumPS(nullptr)
	, mDownScale3x3PS(nullptr)
	, mFinalPassPS(nullptr)
	, mScreenQuadVB(nullptr)
	, mQuadLayout(nullptr)
	, mQuadVS(nullptr)
{
	// Initialize pointers to null
	for (int i = 0; i < NUM_TONEMAP_TEXTURES; ++i)
	{
		mTexToneMap[i] = nullptr;
		mTexToneMapRV[i] = nullptr;
		mTexToneMapRTV[i] = nullptr;
	}
}

CPostProcess& CPostProcess::GetInstance()
{
	static CPostProcess GInstance;
	return GInstance;
}

void CPostProcess::RenderHDR(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	CRenderStates& RenderStates = CRenderStates::GetInstance();

	// Store off original render target, this is the back buffer of the swap chain
	ID3D11RenderTargetView* pOrigRTV = nullptr;
	ID3D11DepthStencilView* pOrigDSV = nullptr;
	pd3dImmediateContext->OMGetRenderTargets(1, &pOrigRTV, &pOrigDSV);

	// Set the render target to our own texture
	ID3D11RenderTargetView* aRTViews[1] = { mTexRenderRTV };
	pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, pOrigDSV);

	pd3dImmediateContext->ClearRenderTargetView(mTexRenderRTV, RenderStates.GetClearColor());

	pd3dImmediateContext->ClearRenderTargetView(pOrigRTV, RenderStates.GetClearColor());
	pd3dImmediateContext->ClearDepthStencilView(pOrigDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	// render scene
	MiniEngine.RenderScene(pd3dDevice, pd3dImmediateContext);

	// measure luminance
	//MeasureLuminancePS11(pd3dImmediateContext);

	// Restore original render targets
	aRTViews[0] = pOrigRTV;
	pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, pOrigDSV);

	// Tone-mapping
	ToneMapping(pd3dDevice, pd3dImmediateContext);

	ID3D11ShaderResourceView* ppSRVNULL[3] = { nullptr, nullptr, nullptr };
	pd3dImmediateContext->PSSetShaderResources(0, 3, ppSRVNULL);

	// release resources
	SAFE_RELEASE(pOrigRTV);
	SAFE_RELEASE(pOrigDSV);
}

void CPostProcess::ToneMapping(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	CRenderStates& RenderStates = CRenderStates::GetInstance();

	auto pBackBufferDesc = DXUTGetDXGIBackBufferSurfaceDesc();

	// render tone mapping
	ID3D11ShaderResourceView* aRViews[3] = { mTexRenderRV, mTexToneMapRV[0], nullptr };
	pd3dImmediateContext->PSSetShaderResources(0, 3, aRViews);

	ID3D11SamplerState* aSamplers[] = {
		RenderStates.GetSamplerState(false, false), RenderStates.GetSamplerState(true, false) };
	pd3dImmediateContext->PSSetSamplers(0, 2, aSamplers);

	DrawFullScreenQuad11(pd3dImmediateContext, mFinalPassPS, pBackBufferDesc->Width, pBackBufferDesc->Height);
}

HRESULT CPostProcess::MeasureLuminancePS11(ID3D11DeviceContext* pd3dImmediateContext)
{
	CRenderStates& RenderStates = CRenderStates::GetInstance();

	ID3D11ShaderResourceView* pTexSrc = nullptr;
	ID3D11ShaderResourceView* pTexDest = nullptr;
	ID3D11RenderTargetView* pSurfDest = nullptr;

	//-------------------------------------------------------------------------
	// Initial sampling pass to convert the image to the log of the grayscale
	//-------------------------------------------------------------------------
	pTexSrc = mTexRenderRV;
	pTexDest = mTexToneMapRV[NUM_TONEMAP_TEXTURES - 1];
	pSurfDest = mTexToneMapRTV[NUM_TONEMAP_TEXTURES - 1];

	D3D11_TEXTURE2D_DESC descSrc;
	mTexRender->GetDesc(&descSrc);
	D3D11_TEXTURE2D_DESC descDest;
	mTexToneMap[NUM_TONEMAP_TEXTURES - 1]->GetDesc(&descDest);

	ID3D11RenderTargetView* aRTViews[1] = { pSurfDest };
	pd3dImmediateContext->OMSetRenderTargets(1, aRTViews, nullptr);
	ID3D11ShaderResourceView* aRViews[1] = { pTexSrc };
	pd3dImmediateContext->PSSetShaderResources(0, 1, aRViews);

	ID3D11SamplerState* aSamplers[] = { RenderStates.GetSamplerState(false, false) };
	pd3dImmediateContext->PSSetSamplers(0, 1, aSamplers);

	DrawFullScreenQuad11(pd3dImmediateContext, mDownScale2x2LumPS, descDest.Width, descDest.Height);

	ID3D11ShaderResourceView* ppSRVNULL[1] = { nullptr };
	pd3dImmediateContext->PSSetShaderResources(0, 1, ppSRVNULL);

	//-------------------------------------------------------------------------
	// Iterate through the remaining tone map textures
	//------------------------------------------------------------------------- 
	for (int i = NUM_TONEMAP_TEXTURES - 1; i > 0; i--)
	{
		// Cycle the textures
		pTexSrc = mTexToneMapRV[i];
		pTexDest = mTexToneMapRV[i - 1];
		pSurfDest = mTexToneMapRTV[i - 1];

		D3D11_TEXTURE2D_DESC desc;
		mTexToneMap[i]->GetDesc(&desc);

		ID3D11RenderTargetView* aRTViews2[1] = { pSurfDest };
		pd3dImmediateContext->OMSetRenderTargets(1, aRTViews2, nullptr);

		ID3D11ShaderResourceView* aRViews2[1] = { pTexSrc };
		pd3dImmediateContext->PSSetShaderResources(0, 1, aRViews2);

		DrawFullScreenQuad11(pd3dImmediateContext, mDownScale3x3PS, desc.Width / 3, desc.Height / 3);

		ID3D11ShaderResourceView* ppSRVNULL2[1] = { nullptr };
		pd3dImmediateContext->PSSetShaderResources(0, 1, ppSRVNULL2);
	}

	return S_OK;
}

void CPostProcess::CreateResources(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
#ifdef _DEBUG
	// Disable optimizations to further improve shader debugging
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#endif

	HRESULT hr;

	// Create the render target texture
	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.ArraySize = 1;
	Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	Desc.Width = pBackBufferSurfaceDesc->Width;
	Desc.Height = pBackBufferSurfaceDesc->Height;
	Desc.MipLevels = 1;
	Desc.SampleDesc.Count = 1;
	hr = (pd3dDevice->CreateTexture2D(&Desc, nullptr, &mTexRender));
	assert(SUCCEEDED(hr));

	// Create the render target view
	D3D11_RENDER_TARGET_VIEW_DESC DescRT;
	DescRT.Format = Desc.Format;
	DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DescRT.Texture2D.MipSlice = 0;
	hr = (pd3dDevice->CreateRenderTargetView(mTexRender, &DescRT, &mTexRenderRTV));
	assert(SUCCEEDED(hr));

	// Create the resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
	DescRV.Format = Desc.Format;
	DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	DescRV.Texture2D.MipLevels = 1;
	DescRV.Texture2D.MostDetailedMip = 0;
	hr = (pd3dDevice->CreateShaderResourceView(mTexRender, &DescRV, &mTexRenderRV));
	assert(SUCCEEDED(hr));

	// Textures for tone mapping for the PS path
	int nSampleLen = 1;
	for (int i = 0; i < NUM_TONEMAP_TEXTURES; i++)
	{
		D3D11_TEXTURE2D_DESC tmdesc = {};
		tmdesc.ArraySize = 1;
		tmdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		tmdesc.Usage = D3D11_USAGE_DEFAULT;
		tmdesc.Format = DXGI_FORMAT_R32_FLOAT;
		tmdesc.Width = nSampleLen;
		tmdesc.Height = nSampleLen;
		tmdesc.MipLevels = 1;
		tmdesc.SampleDesc.Count = 1;
		hr = (pd3dDevice->CreateTexture2D(&tmdesc, nullptr, &mTexToneMap[i]));
		assert(SUCCEEDED(hr));

		// Create the render target view
		D3D11_RENDER_TARGET_VIEW_DESC DescRT2;
		DescRT2.Format = tmdesc.Format;
		DescRT2.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		DescRT2.Texture2D.MipSlice = 0;
		hr = (pd3dDevice->CreateRenderTargetView(mTexToneMap[i], &DescRT2, &mTexToneMapRTV[i]));
		assert(SUCCEEDED(hr));

		// Create the shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC DescRV2;
		DescRV2.Format = tmdesc.Format;
		DescRV2.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		DescRV2.Texture2D.MipLevels = 1;
		DescRV2.Texture2D.MostDetailedMip = 0;
		hr = (pd3dDevice->CreateShaderResourceView(mTexToneMap[i], &DescRV2, &mTexToneMapRV[i]));
		assert(SUCCEEDED(hr));

		nSampleLen *= 3;
	}


	ID3DBlob* pBlob = nullptr;
	// Create the shaders
	hr = (DXUTCompileFromFile(L"Shaders\\PostProcess.hlsl", nullptr, "FinalPass", "ps_5_0", dwShaderFlags, 0, &pBlob));
	assert(SUCCEEDED(hr));
	hr = (pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &mFinalPassPS));
	assert(SUCCEEDED(hr));
	SAFE_RELEASE(pBlob);

	hr = (DXUTCompileFromFile(L"Shaders\\PostProcess.hlsl", nullptr, "DownScale2x2_Lum", "ps_5_0", dwShaderFlags, 0, &pBlob));;
	assert(SUCCEEDED(hr));
	hr = (pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &mDownScale2x2LumPS));;
	assert(SUCCEEDED(hr));
	SAFE_RELEASE(pBlob);

	hr = (DXUTCompileFromFile(L"Shaders\\PostProcess.hlsl", nullptr, "DownScale3x3", "ps_5_0", dwShaderFlags, 0, &pBlob));;
	assert(SUCCEEDED(hr));
	hr = (pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &mDownScale3x3PS));;
	assert(SUCCEEDED(hr));
	SAFE_RELEASE(pBlob);

	hr = (DXUTCompileFromFile(L"Shaders\\PostProcess.hlsl", nullptr, "QuadVS", "vs_5_0", dwShaderFlags, 0, &pBlob));
	assert(SUCCEEDED(hr));
	hr = (pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &mQuadVS));
	assert(SUCCEEDED(hr));
	const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = (pd3dDevice->CreateInputLayout(quadlayout, 2, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &mQuadLayout));
	assert(SUCCEEDED(hr));
	SAFE_RELEASE(pBlob);

	// Create a screen quad for render to texture operations
	SCREEN_VERTEX svQuad[4];
	svQuad[0].mPos = XMFLOAT4(-1.0f, 1.0f, 0.5f, 1.0f);
	svQuad[0].mTex = XMFLOAT2(0.0f, 0.0f);
	svQuad[1].mPos = XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f);
	svQuad[1].mTex = XMFLOAT2(1.0f, 0.0f);
	svQuad[2].mPos = XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f);
	svQuad[2].mTex = XMFLOAT2(0.0f, 1.0f);
	svQuad[3].mPos = XMFLOAT4(1.0f, -1.0f, 0.5f, 1.0f);
	svQuad[3].mTex = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC vbdesc =
	{
		4 * sizeof(SCREEN_VERTEX),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0
	};
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = svQuad;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	hr = (pd3dDevice->CreateBuffer(&vbdesc, &InitData, &mScreenQuadVB));
	assert(SUCCEEDED(hr));
}

void CPostProcess::ReleaseResources()
{
	SAFE_RELEASE(mTexRender);
	SAFE_RELEASE(mTexRenderRTV);
	SAFE_RELEASE(mTexRenderRV);

	SAFE_RELEASE(mDownScale2x2LumPS);
	SAFE_RELEASE(mDownScale3x3PS);
	SAFE_RELEASE(mFinalPassPS);

	for (int i = 0; i < NUM_TONEMAP_TEXTURES; i++)
	{
		SAFE_RELEASE(mTexToneMap[i]); // Tone mapping calculation textures
		SAFE_RELEASE(mTexToneMapRV[i]);
		SAFE_RELEASE(mTexToneMapRTV[i]);
	}

	SAFE_RELEASE(mScreenQuadVB);
	SAFE_RELEASE(mQuadVS);
	SAFE_RELEASE(mQuadLayout);
}

void CPostProcess::DrawFullScreenQuad11(ID3D11DeviceContext* pd3dImmediateContext,
	ID3D11PixelShader* pPS, UINT Width, UINT Height)
{
	// Save the old viewport
	D3D11_VIEWPORT vpOld[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
	UINT nViewPorts = 1;
	pd3dImmediateContext->RSGetViewports(&nViewPorts, vpOld);

	// Setup the viewport to match the backbuffer
	D3D11_VIEWPORT vp;
	vp.Width = (float)Width;
	vp.Height = (float)Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pd3dImmediateContext->RSSetViewports(1, &vp);

	UINT strides = sizeof(SCREEN_VERTEX);
	UINT offsets = 0;
	ID3D11Buffer* pBuffers[1] = { mScreenQuadVB };

	pd3dImmediateContext->IASetInputLayout(mQuadLayout);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pBuffers, &strides, &offsets);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	pd3dImmediateContext->VSSetShader(mQuadVS, nullptr, 0);
	pd3dImmediateContext->PSSetShader(pPS, nullptr, 0);
	pd3dImmediateContext->Draw(4, 0);

	// Restore the Old viewport
	pd3dImmediateContext->RSSetViewports(nViewPorts, vpOld);
}