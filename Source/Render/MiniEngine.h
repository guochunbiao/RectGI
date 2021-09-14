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

class IMeshData;
class CRenderInstance;

using namespace std;
using namespace DirectX;

// Callback function when creating a render instance.
typedef void (*CreateRenderInstancesCallback)(ID3D11Device*);
// Callback function when initializing device.
typedef void (*SetupEnvironmentCallback)(void);
// Callback function when updating a frame.
typedef void (*FrameUpdateCallback)(double, float);

// A minimal engine class.
class CMiniEngine
{
private:
	CMiniEngine();

public:
	static CMiniEngine& GetInstance();

	// Setup DXUT callbacks.
	void SetupDXUT(CreateRenderInstancesCallback InCreateRenderInstances, SetupEnvironmentCallback InSetupEnvironment,
		FrameUpdateCallback InFrameUpdate);
	// Destroy device.
	void DestroyEngine();

	// Create render instance with given name and mesh data.
	CRenderInstance* CreateRenderInstance(const string& InName, IMeshData* InMeshData,
		LPCWSTR InVS, LPCWSTR InPS, ID3D11Device* pd3dDevice);

	// Find rendering instance by name.
	CRenderInstance* GetRenderInstance(const string& InName);
	// Find rendering instance by index.
	CRenderInstance* GetRenderInstance(UINT Index);

	// Set position of light.
	void SetLightPosition(const XMFLOAT3& InLightPos);
	// Set direction of distant light.
	void SetLightDir(const XMFLOAT3& InLightDir);

	// The entry function for rendering the scene.
	void RenderScene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);

	// Rendering entry function called every frame.
	void OnFrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime);

	// Report non-zero references.
	void ReportLiveDeviceObjects();

	// Take a screenshot.
	void TakeScreenshot();

private:
	// Initialize.
	void InitApp();
	// Destroy all render instances.
	void DestroyRenderInstances();

	// Rendering the scene with low dynamic range.
	void RenderLDR(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);

public:
	// Container for all rendering instances.
	map<string, CRenderInstance*> mRenderInstances;

	// Camera class.
	CModelViewerCamera mCamera;

	// Render instance of light.
	CRenderInstance* mLightInstance;
	// Controller for light.
	CDXUTDirectionWidget mLightControl;

	// Light intensity.
	float mLightIntensity;
	// Deprecated.
	float mSpecularReflIntensity;
	float mDiffuseReflIntensity;
	// Sampling radius for specular indirect lighting.
	float mSpecularSamplingRadius;
	// Sampling radius for diffuse indirect lighting.
	float mDiffuseSamplingRadius;

	// Callback function when creating a render instance.
	CreateRenderInstancesCallback OnCreateRenderInstances;
	// Callback function when initializing device.
	SetupEnvironmentCallback OnSetupEnvironment;
	// Callback function when updating a frame.
	FrameUpdateCallback OnFrameUpdate;
};
