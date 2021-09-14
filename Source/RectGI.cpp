#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "resource.h"
#include "Render/MeshData.h"
#include "Render/MiniEngine.h"
#include "Render/RenderData.h"
#include "Render/DemoUI.h"
#include <complex>
#include <corecrt_math_defines.h>

#pragma warning( disable : 4100 )

using namespace DirectX;

// Initialize mini engine.
CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

// Create render instances.
void CreateRenderInstances(ID3D11Device* pd3dDevice)
{
	// Create light instance.
	IMeshData* MeshData0 = IMeshData::CreateDxMesh(L"ball.sdkmesh", pd3dDevice);
	CRenderInstance* light0 = MiniEngine.CreateRenderInstance("tiny0", MeshData0,
		L"Shaders\\DxMeshVS.hlsl", L"Shaders\\DxMeshPS.hlsl", pd3dDevice);
	light0->SetPosition(XMFLOAT3(-300.f, -300.f, -280.f));
	light0->SetRotation(.0f, .0f, .0f);
	light0->SetScale(50);
	MiniEngine.mLightInstance = light0;
	MiniEngine.SetLightPosition(light0->mPosition);

	// Create scene.
	const XMFLOAT3 boxColor = XMFLOAT3(.7f, 0.7f, .2f);
	INT16 InstID = 0;
	CRenderInstance::CreateRectInstance(pd3dDevice, "floor",
		XMFLOAT3(0.f, 0.f, 280.f), XMFLOAT3(1.f, .0f, .0f), 300, 0.1f, XMFLOAT3(0.7f, 0.2f, 0.2f), InstID++);
	CRenderInstance::CreateRectInstance(pd3dDevice, "wallBack",
		XMFLOAT3(0.f, 300.f, -20.f), XMFLOAT3(.5f, 1.f, .0f), 300, 0.1f, XMFLOAT3(0.2f, 0.7f, 0.2f), InstID++);
	CRenderInstance::CreateRectInstance(pd3dDevice, "wallR",
		XMFLOAT3(300.f, 0.f, -20.f), XMFLOAT3(1.f, 2.5f, .0f), 300, 0.1f, XMFLOAT3(0.2f, 0.2f, 0.7f), InstID++);
	CRenderInstance::CreateRectInstance(pd3dDevice, "wallL",
		XMFLOAT3(-300.f, 0.f, -20.f), XMFLOAT3(-1.f, -2.5f, .0f), 300, 0.1f, XMFLOAT3(0.2f, 0.2f, 0.7f), InstID++);
}

void SetupEnvironment()
{
	DXUTSetMediaSearchPath(L"./mesh");

	// Light
	MiniEngine.mLightIntensity = 18;
	MiniEngine.mSpecularReflIntensity = 1;
	MiniEngine.mDiffuseReflIntensity = .008f;

	// Camera
	static const XMVECTORF32 s_vecEye = { 0.0f, -30.0f, -20.0f, 0.0f };
	FLOAT mSceneRadius = 378.15607f;
	MiniEngine.mCamera.SetViewParams(s_vecEye, g_XMZero);
	MiniEngine.mCamera.SetRadius(mSceneRadius * 3.0f, mSceneRadius * 0.5f, mSceneRadius * 10.0f);
}

void UpdateFrame(double fTime, float fElapsedTime)
{
	// moving light
	static const float GLightSpeed = 2;
	XMFLOAT3 newLightPos = XMFLOAT3(std::cos((float)fTime* GLightSpeed) * 500, std::sin((float)fTime * GLightSpeed) * 500, -300);
	MiniEngine.SetLightPosition(newLightPos);

	CRectCollections::GetInstance().UpdateAllProxies();

	// link reflectors
	CRenderInstance::LinkReflectors("floor", {"wallBack", "wallL", "wallR"});
	CRenderInstance::LinkReflectors("wallBack", {"floor", "wallL", "wallR" });
	CRenderInstance::LinkReflectors("wallL", { "wallBack", "floor" });
	CRenderInstance::LinkReflectors("wallR", { "wallBack", "floor" });
}

int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    MiniEngine.SetupDXUT(CreateRenderInstances, SetupEnvironment, UpdateFrame);
    return DXUTGetExitCode();
}
