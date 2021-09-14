#include "DXUT.h"
#include "MiniEngine.h"

#include "MiniEngine.inl"
#include "RenderData.h"
#include "MeshData.h"
#include <map>
#include "DemoUI.h"
#include "RenderStates.h"
#include "PostProcess.h"

CMiniEngine::CMiniEngine()
	: mLightIntensity(1)
	, mLightInstance(nullptr)
	, mSpecularReflIntensity(0.1f)
	, mDiffuseReflIntensity(1)
	, mSpecularSamplingRadius(300)
	, mDiffuseSamplingRadius(100)
{

}

CMiniEngine& CMiniEngine::GetInstance()
{
	static CMiniEngine GInstance;
	return GInstance;
}

void CMiniEngine::SetupDXUT(CreateRenderInstancesCallback InCreateRenderInstances, SetupEnvironmentCallback InSetupEnvironment,
	FrameUpdateCallback InFrameUpdate)
{
	OnCreateRenderInstances = InCreateRenderInstances;
	OnSetupEnvironment = InSetupEnvironment;
	OnFrameUpdate = InFrameUpdate;

	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);

	DXUTSetCallbackFrameMove(OnFrameMove);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	InitApp();
}

void CMiniEngine::InitApp()
{
	CDemoUI::GetInstance().InitGUI();
	
	// Parse the command line, show msgboxes on error, no extra command line params
	DXUTInit(true, true, nullptr); 
	// Show the cursor and clip it when in full screen
	DXUTSetCursorSettings(true, true);
	DXUTCreateWindow(L"RectGI");
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_1, true, 1024, 768);
	// Enter into the DXUT render loop
	DXUTMainLoop();
}

void CMiniEngine::DestroyEngine()
{
	// Destroy UI
	CDemoUI::GetInstance().DestroyGUI();
	DXUTGetGlobalResourceCache().OnDestroyDevice();

	// Destroy render instances.
	DestroyRenderInstances();

	// Destroy render states.
	CRenderStates::GetInstance().OnDestroy();

#if _DEBUG
	ReportLiveDeviceObjects();
#endif
}

void CMiniEngine::ReportLiveDeviceObjects()
{
	ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
	ID3D11Debug* debugDev = nullptr;
	pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDev));
	//debugDev->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	debugDev->ReportLiveDeviceObjects(D3D11_RLDO_IGNORE_INTERNAL);
	SAFE_RELEASE(debugDev);
}

void CMiniEngine::TakeScreenshot()
{
	IDXGISwapChain* pSwapChain = DXUTGetDXGISwapChain();
	ID3D11Device* pDevice = DXUTGetD3D11Device();

	ID3D11DeviceContext* pContext;
	pDevice->GetImmediateContext(&pContext);

	ID3D11Texture2D* pBackBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));

	D3D11_TEXTURE2D_DESC txtDesc;
	pBackBuffer->GetDesc(&txtDesc);
	txtDesc.Usage = D3D11_USAGE_STAGING;
	txtDesc.BindFlags = 0;
	txtDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	// txtDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM

	ID3D11Texture2D* pBackBufferStaging;
	pDevice->CreateTexture2D(&txtDesc, nullptr, &pBackBufferStaging);
	pContext->CopyResource(pBackBufferStaging, pBackBuffer);

	HRESULT hr = DXUTSaveTextureToFile(pContext, pBackBufferStaging, false, L"E:\\Tmp\\screenshot.png");
	assert(SUCCEEDED(hr));

	pBackBufferStaging->Release();
	pBackBuffer->Release();
	pContext->Release();
}

CRenderInstance* CMiniEngine::CreateRenderInstance(const string& InName, IMeshData* InMeshData, 
	LPCWSTR InVS, LPCWSTR InPS, ID3D11Device* pd3dDevice)
{
	// Create render instance.
	CRenderInstance* RenderInst = new CRenderInstance(InName);
	RenderInst->mMeshData = InMeshData;

	UINT NumVertexElement;
	const D3D11_INPUT_ELEMENT_DESC* layout = InMeshData->GetVertexDesc(NumVertexElement);
	// Create vertex shader and pixel shader.
	RenderInst->CreateVertexShader(InVS, "main", layout, NumVertexElement, pd3dDevice);
	RenderInst->CreatePixelShader(InPS, "main", pd3dDevice);

	// Add render instance to container.
	assert(mRenderInstances.count(InName) == 0);
	mRenderInstances[InName] = RenderInst;

	return RenderInst;
}

CRenderInstance* CMiniEngine::GetRenderInstance(const string& InName)
{
	return mRenderInstances[InName];
}

CRenderInstance* CMiniEngine::GetRenderInstance(UINT Index)
{
	assert(Index < mRenderInstances.size());
	auto it = mRenderInstances.begin();
	std::advance(it, Index);
	return it->second;
}

void CMiniEngine::SetLightPosition(const XMFLOAT3& InLightPos)
{
	if (mLightInstance == nullptr)
		return;

	mLightInstance->mPosition = InLightPos;
	SetLightDir(InLightPos);
}

void CMiniEngine::SetLightDir(const XMFLOAT3& InLightDir)
{
	XMVECTOR vLightDir = mLightControl.GetLightDirection();

	vLightDir = XMVector3Normalize(XMLoadFloat3(&InLightDir));
	mLightControl.SetLightDirection(vLightDir);
}

void CMiniEngine::RenderScene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	CRenderStates& RSMgr = CRenderStates::GetInstance();

	map<string, CRenderInstance*>::iterator iter;
	for (iter = mRenderInstances.begin(); iter != mRenderInstances.end(); ++iter) 
	{
		CRenderInstance* RenderInst = iter->second;
		if (!RenderInst->mRender)
			continue;

		// Set the rasterizer state
		pd3dImmediateContext->RSSetState(RSMgr.GetRasterizerState(RenderInst->mCull));

		//Get the mesh
		//IA setup
		pd3dImmediateContext->IASetInputLayout(RenderInst->mVertexLayout11);
		UINT Strides[1];
		UINT Offsets[1];
		ID3D11Buffer* pVB[1];
		pVB[0] = RenderInst->mMeshData->GetVertexBuffer(pd3dDevice);
		assert(pVB[0] != nullptr);
		Strides[0] = (UINT)RenderInst->mMeshData->GetVertexStride();
		assert(Strides[0] > 0 && Strides[0] < 1024);
		Offsets[0] = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
		pd3dImmediateContext->IASetIndexBuffer(RenderInst->mMeshData->GetIndexBuffer(), RenderInst->mMeshData->GetIndexFormat(), 0);

		// Update render instance before rendering.
		RenderInst->OnFrameRender(pd3dImmediateContext);

		// set topology type.
		auto PrimType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		pd3dImmediateContext->IASetPrimitiveTopology(PrimType);

		// Ignores most of the material information in the mesh to use only a simple shader
		auto pDiffuseRV = RenderInst->mMeshData->GetTexture();
		if (pDiffuseRV != nullptr)
		{
			ID3D11SamplerState* SamplerState = RSMgr.GetSamplerState(true, true);
			pd3dImmediateContext->PSSetSamplers(0, 1, &SamplerState);
			pd3dImmediateContext->PSSetShaderResources(0, 1, &pDiffuseRV);
		}

		// Drawing.
		UINT VertexStart = 0;
		pd3dImmediateContext->DrawIndexed(RenderInst->mMeshData->GetIndexNum(), 0, VertexStart);
	}
}

void CMiniEngine::DestroyRenderInstances()
{
	// release render instances.
	map<string, CRenderInstance*>::iterator iter;
	for (iter = mRenderInstances.begin(); iter != mRenderInstances.end(); ++iter) {
		CRenderInstance* RenderInst = iter->second;
		IMeshData::DestroyMesh(&RenderInst->mMeshData);
		RenderInst->Destroy();
		SAFE_DELETE(RenderInst);
	}

	mRenderInstances.clear();
}

//--------------------------------------------------------------------------------------
// DXUT callbacks
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	// Create UI.
	CDemoUI::GetInstance().CreateGUI(pd3dDevice);

	// Callbacks
	CMiniEngine::GetInstance().OnSetupEnvironment();
	CMiniEngine::GetInstance().OnCreateRenderInstances(pd3dDevice);

	// Initialize render states.
	CRenderStates::GetInstance().InitRenderStates(pd3dDevice);

	return S_OK;
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	// Update the camera's position based on user input 
	MiniEngine.mCamera.FrameMove(fElapsedTime);
	MiniEngine.OnFrameUpdate(fTime, fElapsedTime);
}

void CMiniEngine::RenderLDR(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	// Clear the render target and depth stencil
	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, CRenderStates::GetInstance().GetClearColor());
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	// render scene
	RenderScene(pd3dDevice, pd3dImmediateContext);
}

void CMiniEngine::OnFrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	if (CDemoUI::GetInstance().mShowHDR)
	{
		// render with high dynamic range
		CPostProcess::GetInstance().RenderHDR(pd3dDevice, pd3dImmediateContext);
	}
	else
	{
		// render with low dynamic range
		RenderLDR(pd3dDevice, pd3dImmediateContext);
	}

	// render ui to the backbuffer
	CDemoUI::GetInstance().RenderGUI(fElapsedTime);
}

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	CMiniEngine::GetInstance().OnFrameRender(pd3dDevice, pd3dImmediateContext, fElapsedTime);
}

void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	MiniEngine.DestroyEngine();
}

void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	CDemoUI::GetInstance().OnKeyboard(nChar, bKeyDown, bAltDown, pUserContext);
}

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	CDemoUI& DemoUI = CDemoUI::GetInstance();
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = DemoUI.mDialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	MiniEngine.mLightControl.HandleMessages(hWnd, uMsg, wParam, lParam);

	// Pass all remaining windows messages to camera so it can respond to user input
	MiniEngine.mCamera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	CDemoUI& DemoUI = CDemoUI::GetInstance();
	HRESULT hr;

	CPostProcess::GetInstance().CreateResources(pd3dDevice, pSwapChain, pBackBufferSurfaceDesc);

	hr = (DemoUI.mDialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	assert(SUCCEEDED(hr));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	MiniEngine.mCamera.SetProjParams(XM_PI / 4, fAspectRatio, 2.0f, 4000.0f);
	MiniEngine.mCamera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	MiniEngine.mCamera.SetButtonMasks(MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);

	return S_OK;
}

void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	CPostProcess::GetInstance().ReleaseResources();
	CDemoUI::GetInstance().mDialogResourceManager.OnD3D11ReleasingSwapChain();
}
