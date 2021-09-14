#pragma once
#include <d3d11.h>
#include <string>

using namespace DirectX;
using namespace std;

// Stores pre-created render states.
class CRenderStates
{
private:
	CRenderStates();

public:
	static CRenderStates& GetInstance();

	// Initialization
	void InitRenderStates(ID3D11Device* pd3dDevice);
	// Destroy all resources.
	void OnDestroy();

	// Get clear color for background.
	const XMVECTORF32 GetClearColor();

	// Get rasterizer state.
	ID3D11RasterizerState* GetRasterizerState(bool bCull);

	// Get sampler state.
	ID3D11SamplerState* GetSamplerState(bool bLinear, bool bWrap);

private:
	// Initialize rasterizer states.
	void InitRasterizerStates(ID3D11Device* pd3dDevice);
	// Destroy rasterizer states.
	void DestroyRasterizerStates();

	// Initialize sampler states.
	void InitSamplerStates(ID3D11Device* pd3dDevice);
	// Destroy sampler states.
	void DestroySamplerStates();

private:
	// pre-created cull modes
	ID3D11RasterizerState* mFrontCull;
	ID3D11RasterizerState* mNoCull;

	// pre-created sampler states
	ID3D11SamplerState* mSamplerStates[2][2];
};