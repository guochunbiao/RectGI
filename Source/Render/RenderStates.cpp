#include "DXUT.h"
#include "RenderStates.h"

CRenderStates::CRenderStates()
	: mFrontCull(nullptr)
	, mNoCull(nullptr)
{
	for (int i0 = 0; i0 < 2; ++i0)
	{
		for (int i1 = 0; i1 < 2; ++i1)
		{
			mSamplerStates[i0][i1] = nullptr;
		}
	}
}

CRenderStates& CRenderStates::GetInstance()
{
	static CRenderStates GInstance;
	return GInstance;
}

void CRenderStates::InitRenderStates(ID3D11Device* pd3dDevice)
{
	InitRasterizerStates(pd3dDevice);
	InitSamplerStates(pd3dDevice);
}

void CRenderStates::OnDestroy()
{
	DestroyRasterizerStates();
	DestroySamplerStates();
}

const XMVECTORF32 CRenderStates::GetClearColor()
{
	//return Colors::DimGray;
	//return Colors::MidnightBlue;
	return Colors::Black;
}

ID3D11SamplerState* CRenderStates::GetSamplerState(bool bLinear, bool bWrap)
{
	int i0 = bLinear ? 0 : 1;
	int i1 = bWrap ? 0 : 1;
	assert(mSamplerStates[i0][i1] != nullptr);
	return mSamplerStates[i0][i1];
}

ID3D11RasterizerState* CRenderStates::GetRasterizerState(bool bCull)
{
	ID3D11RasterizerState* ret = nullptr;
	if (bCull)
	{
		ret = mFrontCull;
	}
	else
	{
		ret = mNoCull;
	}
	
	assert(ret != nullptr);
	return ret;
}

void CRenderStates::InitRasterizerStates(ID3D11Device* pd3dDevice)
{
	D3D11_RASTERIZER_DESC RasterizerDescBackCull = {
		D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
		D3D11_CULL_FRONT,   // D3D11_CULL_MODE CullMode;
		TRUE,               // BOOL FrontCounterClockwise;
		0,                  // INT DepthBias;
		0,                  // FLOAT DepthBiasClamp;
		0,                  // FLOAT SlopeScaledDepthBias;
		FALSE,              // BOOL DepthClipEnable;
		FALSE,              // BOOL ScissorEnable;
		TRUE,               // BOOL MultisampleEnable;
		FALSE,              // BOOL AntialiasedLineEnable;
	};
	HRESULT hr = (pd3dDevice->CreateRasterizerState(&RasterizerDescBackCull, &mFrontCull));
	assert(SUCCEEDED(hr));

	D3D11_RASTERIZER_DESC RasterizerDescNoCull = {
		D3D11_FILL_SOLID,   // D3D11_FILL_MODE FillMode;
		D3D11_CULL_NONE,    // D3D11_CULL_MODE CullMode;
		TRUE,               // BOOL FrontCounterClockwise;
		0,                  // INT DepthBias;
		0,                  // FLOAT DepthBiasClamp;
		0,                  // FLOAT SlopeScaledDepthBias;
		FALSE,              // BOOL DepthClipEnable;
		FALSE,              // BOOL ScissorEnable;
		TRUE,               // BOOL MultisampleEnable;
		FALSE,              // BOOL AntialiasedLineEnable;
	};
	hr = (pd3dDevice->CreateRasterizerState(&RasterizerDescNoCull, &mNoCull));
	assert(SUCCEEDED(hr));
}

void CRenderStates::DestroyRasterizerStates()
{
	SAFE_RELEASE(mFrontCull);
	SAFE_RELEASE(mNoCull);
}

void CRenderStates::InitSamplerStates(ID3D11Device* pd3dDevice)
{
	//int i0 = bLinear ? 0 : 1;
	//int i1 = bWrap ? 0 : 1;
	HRESULT hr;
	// Samplers
	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = (pd3dDevice->CreateSamplerState(&SamplerDesc, &mSamplerStates[0][1]));
	assert(SUCCEEDED(hr));

	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = (pd3dDevice->CreateSamplerState(&SamplerDesc, &mSamplerStates[1][1]));
	assert(SUCCEEDED(hr));

	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 0;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = (pd3dDevice->CreateSamplerState(&SamplerDesc, &mSamplerStates[0][0]));
	assert(SUCCEEDED(hr));

	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hr = (pd3dDevice->CreateSamplerState(&SamplerDesc, &mSamplerStates[1][0]));
	assert(SUCCEEDED(hr));
}

void CRenderStates::DestroySamplerStates()
{
	for (int i0 = 0; i0 < 2; ++i0)
	{
		for (int i1 = 0; i1 < 2; ++i1)
		{
			SAFE_RELEASE(mSamplerStates[i0][i1]);
			mSamplerStates[i0][i1] = nullptr;
		}
	}
}

