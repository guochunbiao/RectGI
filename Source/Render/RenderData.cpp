#include "DXUT.h"
#include "RenderData.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "MiniEngine.h"
#include "MeshData.h"
#include "DemoUI.h"
#include "RectProxy.h"

#ifdef _DEBUG
// Disable optimizations to further improve shader debugging
DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#endif

CRenderInstance::CRenderInstance(const string& InName)
	: mMeshData(nullptr)
	, mID(INVALID_PLANE_ID)
	, mRender(true)
	, mReflector(true)
	, mReceiver(true)
	, mPitch(0)
	, mYaw(0)
	, mRoll(0)
	, mScale(1.f)
	, mRoughness(1.f)
	, mCull(true)
	, mVertexLayout11(nullptr)
	, mVertexShader(nullptr)
	, mCbVSPerObject(nullptr)
	, mPixelShader(nullptr)
	, mCbPSPerObject(nullptr)
	, mCbPSRects(nullptr)
	, mCbPSPerFrame(nullptr)
{
	mName = InName;
}

CRenderInstance* CRenderInstance::CreateRectInstance(ID3D11Device* pd3dDevice, 
	const string& InName, const XMFLOAT3& InPos, const XMFLOAT3& InRot, 
	const float InScale, float InRoughness,
	const XMFLOAT3& InDiffuseColor, INT16 inID)
{
	CRectMesh* MeshData = IMeshData::CreateRectMesh(pd3dDevice);

	// Create render instance with given name and mesh data. 
	CRenderInstance* RenderInst = CMiniEngine::GetInstance().CreateRenderInstance(InName, MeshData,
		L"Shaders\\PlaneMeshVS.hlsl", L"Shaders\\PlaneMeshPS.hlsl", pd3dDevice);
	RenderInst->SetPosition(InPos);
	RenderInst->SetRotation(InRot.x, InRot.y, InRot.z);
	RenderInst->SetScale(InScale);
	RenderInst->mRoughness = InRoughness;
	RenderInst->mDiffuseColor = InDiffuseColor;
	RenderInst->mID = inID;

	return RenderInst;
}

CRenderInstance* CRenderInstance::CreateTestRectInstance(ID3D11Device* pd3dDevice, const string& InName)
{
	CCPUMesh* CpuMesh = (CCPUMesh*)IMeshData::CreateCpuMesh(pd3dDevice);

	// Create render instance with given name and mesh data. 
	CRenderInstance* RenderInst = CMiniEngine::GetInstance().CreateRenderInstance(InName, CpuMesh,
		L"Shaders\\OneColorVS.hlsl", L"Shaders\\OneColorPS.hlsl", pd3dDevice);
	RenderInst->mRender = false;

	return RenderInst;
}

void CRenderInstance::SetPosition(const XMFLOAT3& InPosition)
{
	mPosition = InPosition;
}

void CRenderInstance::SetRotation(float InPitch, float InYaw, float InRoll)
{
	mPitch = InPitch;
	mYaw = InYaw;
	mRoll = InRoll;
}

void CRenderInstance::SetScale(float InScale)
{
	mScale = InScale;
}

void CRenderInstance::Destroy()
{
	SAFE_RELEASE(mVertexLayout11);
	SAFE_RELEASE(mVertexShader);
	SAFE_RELEASE(mCbVSPerObject);

	SAFE_RELEASE(mPixelShader);    
	SAFE_RELEASE(mCbPSPerObject);
	SAFE_RELEASE(mCbPSRects);
	SAFE_RELEASE(mCbPSPerFrame);
}

XMMATRIX CRenderInstance::GetWorldMatrix() const
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

	const static FXMVECTOR ZeroOrigin = { .0f, .0f, .0f };
	XMVECTOR ScaleOrientQuat = XMQuaternionRotationRollPitchYawFromVector(ZeroOrigin);
	XMVECTOR RotationQuat = XMQuaternionRotationRollPitchYawFromVector({mPitch * XM_PI, mYaw * XM_PI, mRoll * XM_PI });
	XMMATRIX MeshMat = XMMatrixAffineTransformation(
		{mScale, mScale, mScale}, ZeroOrigin, RotationQuat, XMLoadFloat3(&mPosition));

	return MeshMat * MiniEngine.mCamera.GetWorldMatrix();
}

XMMATRIX CRenderInstance::GetWVPMatrix() const
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

	XMMATRIX mWorld = GetWorldMatrix();

	// Get the projection & view matrix from the camera class
	XMMATRIX mProj = MiniEngine.mCamera.GetProjMatrix();
	XMMATRIX mView = MiniEngine.mCamera.GetViewMatrix();

	return mWorld * mView * mProj;
}

void CRenderInstance::GetRectProxy(CRect& OutRect) const
{
	if (mMeshData->GetMeshType() != EMeshData::RectMesh)
	{
		assert(false);
		return;
	}

	XMMATRIX WorldMat = GetWorldMatrix();

	// copy rectangle properties
	OutRect.mRoughness = mRoughness;
	OutRect.mCenter = mPosition;
	OutRect.mMajorRadius = mScale * 1;
	OutRect.mMinorRadius = mScale * 1;
	OutRect.mDiffuseColor = mDiffuseColor;
	OutRect.mID = mID;

	// transform axes
	const XMFLOAT3 GDefaultNormal = XMFLOAT3(.0f, .0f, 1.0f);
	XMVECTOR xNormal = XMVector3TransformNormal(XMLoadFloat3(&GDefaultNormal), WorldMat);
	xNormal = XMVector3Normalize(xNormal);
	XMStoreFloat3(&(OutRect.mNormal), xNormal);

	const XMFLOAT3 GDefaultMajorAxis = XMFLOAT3(1.0f, .0f, .0f);
	XMVECTOR xMajor = XMVector3TransformNormal(XMLoadFloat3(&GDefaultMajorAxis), WorldMat);
	xMajor = XMVector3Normalize(xMajor);
	XMStoreFloat3(&(OutRect.mMajorAxis), xMajor);
}

void CRenderInstance::LinkReflectors(const string& RecvName, const vector<string>& ReflNames)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

	CRenderInstance* Recv = MiniEngine.GetRenderInstance(RecvName);
	Recv->UnlinkReflectors();
	int plIdx = 0;
	for (auto it = begin(ReflNames); it != end(ReflNames); ++it)
	{
		CRenderInstance* Refl = MiniEngine.GetRenderInstance(*it);
		assert(Refl != nullptr && Refl != Recv);
		Recv->mReflectorIndices[plIdx++] = Refl->mID;
	}
}

void CRenderInstance::UnlinkReflectors()
{
	for (int i = 0; i < MAX_RELATED_REFLECTOR_NUM; i ++)
	{
		mReflectorIndices[i] = INVALID_PLANE_ID;
	}
}

struct CB_VS_PER_OBJECT
{
	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
};
UINT g_iCBVSPerObjectBind = 0;
void CRenderInstance::UpdateVSConstants(ID3D11DeviceContext* pd3dImmediateContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	D3D11_MAPPED_SUBRESOURCE MappedResource;

	// Get the projection & view matrix from the camera class
	XMMATRIX mProj = MiniEngine.mCamera.GetProjMatrix();
	XMMATRIX mView = MiniEngine.mCamera.GetViewMatrix();

	// constant buffer: VS Per object
	XMMATRIX mt;
	XMMATRIX mWorld = GetWorldMatrix();
	HRESULT hr = (pd3dImmediateContext->Map(mCbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	assert(SUCCEEDED(hr));
	auto pVSPerObject = reinterpret_cast<CB_VS_PER_OBJECT*>(MappedResource.pData);
	//XMMATRIX mt = XMMatrixTranspose(mWorldViewProjection);
	//XMStoreFloat4x4(&pVSPerObject->m_WorldViewProj, mt);
	mt = XMMatrixTranspose(mWorld);
	XMStoreFloat4x4(&pVSPerObject->mWorld, mt);
	mt = XMMatrixTranspose(mView);
	XMStoreFloat4x4(&pVSPerObject->mView, mt);
	mt = XMMatrixTranspose(mProj);
	XMStoreFloat4x4(&pVSPerObject->mProj, mt);
	pd3dImmediateContext->Unmap(mCbVSPerObject, 0);

	pd3dImmediateContext->VSSetConstantBuffers(g_iCBVSPerObjectBind, 1, &mCbVSPerObject);
}

struct CB_PS_PER_OBJECT
{
	XMFLOAT4 mObjectColor;
	XMFLOAT4 mCameraPos;
	XMFLOAT4 mRoughness4;
	XMFLOAT4 mCustomData0;
	XMFLOAT4 mDiffuseColor;
	uint32_t mLinkedReflectors[MAX_RELATED_REFLECTOR_NUM*4];
};
UINT g_iCBPSPerObjectBind = 0;

struct CB_PS_PER_FRAME
{
	XMFLOAT4 mLightDirAmbient;
	XMFLOAT4 mLightIntensity;
	uint32_t mToggleOptionsA[4];
	XMFLOAT4 mSamplingRadius;
};
UINT g_iCBPSPerFrameBind = 1;

struct CB_PS_PLANES
{
	// x: id, y: roughness, z: majorRadius, w, minorRadius
	XMFLOAT4 plElement0[MAX_RECT_NUM];
	// xyz: center, w: diffuse.x
	XMFLOAT4 plCenter_DiffX[MAX_RECT_NUM];
	// xyz: normal, w: diffuse.y
	XMFLOAT4 plNorm_DifY[MAX_RECT_NUM];
	// xyz: major axis, w: diffuse.z
	XMFLOAT4 plAxis_DifZ[MAX_RECT_NUM];
};
UINT g_iCBPSPlanesBind = 2;

void CRenderInstance::UpdatePSConstants(ID3D11DeviceContext* pd3dImmediateContext)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();
	CDemoUI& DemoUI = CDemoUI::GetInstance();
	CRectCollections& RectColls = CRectCollections::GetInstance();
	XMVECTOR CameraPt = MiniEngine.mCamera.GetEyePt();
	D3D11_MAPPED_SUBRESOURCE MappedResource;

	// constant buffer: PS Per frame
	HRESULT hr = (pd3dImmediateContext->Map(mCbPSPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	assert(SUCCEEDED(hr));
	auto pPerFrame = reinterpret_cast<CB_PS_PER_FRAME*>(MappedResource.pData);
	float fAmbient = 0.1f;
	// Get the light direction
	XMVECTOR vLightDir = CMiniEngine::GetInstance().mLightControl.GetLightDirection();
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pPerFrame->mLightDirAmbient), vLightDir);
	pPerFrame->mLightDirAmbient.w = fAmbient;
	pPerFrame->mLightIntensity.x = MiniEngine.mLightIntensity;
	pPerFrame->mLightIntensity.y = MiniEngine.mSpecularReflIntensity;
	pPerFrame->mLightIntensity.z = MiniEngine.mDiffuseReflIntensity;
	pPerFrame->mToggleOptionsA[0] = DemoUI.mShowIndirectSpecular;
	pPerFrame->mToggleOptionsA[1] = DemoUI.mShowDirectLighting;
	pPerFrame->mToggleOptionsA[2] = 0;
	pPerFrame->mToggleOptionsA[3] = DemoUI.mShowIndirectDiffuse;
	pPerFrame->mSamplingRadius.x = MiniEngine.mSpecularSamplingRadius;
	pPerFrame->mSamplingRadius.y = MiniEngine.mDiffuseSamplingRadius;
	pd3dImmediateContext->Unmap(mCbPSPerFrame, 0);
	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerFrameBind, 1, &mCbPSPerFrame);

	// constant buffer: PS Per object
	hr = (pd3dImmediateContext->Map(mCbPSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	assert(SUCCEEDED(hr));
	auto pPSPerObject = reinterpret_cast<CB_PS_PER_OBJECT*>(MappedResource.pData);
	XMStoreFloat4(&pPSPerObject->mObjectColor, Colors::White);
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pPSPerObject->mCameraPos), CameraPt);
	pPSPerObject->mRoughness4.x = mRoughness;
	pPSPerObject->mCustomData0 = mCustomData0; 
	pPSPerObject->mDiffuseColor = XMFLOAT4(mDiffuseColor.x, mDiffuseColor.y, mDiffuseColor.z, 1.f);
	for (uint8_t i = 0; i < MAX_RELATED_REFLECTOR_NUM; i++)
	{
		pPSPerObject->mLinkedReflectors[i * 4] = mReflectorIndices[i];
	}
	pd3dImmediateContext->Unmap(mCbPSPerObject, 0);
	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerObjectBind, 1, &mCbPSPerObject);

	// constant buffer: PS Planes
	hr = (pd3dImmediateContext->Map(mCbPSRects, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	assert(SUCCEEDED(hr));

	auto pPSPlanes = reinterpret_cast<CB_PS_PLANES*>(MappedResource.pData);
	for (uint8_t i = 0; i < MAX_RECT_NUM; i ++)
	{
		CRect& Rect = RectColls.mAllRects[i];		
		if (Rect.mID == INVALID_PLANE_ID)
		{
			pPSPlanes->plElement0[i].x = INVALID_PLANE_ID;
			continue;
		}

		// x: id, y: roughness, z: majorRadius, w, minorRadius
		pPSPlanes->plElement0[i].x = Rect.mID;
		pPSPlanes->plElement0[i].y = Rect.mRoughness;
		pPSPlanes->plElement0[i].z = Rect.mMajorRadius;
		pPSPlanes->plElement0[i].w = Rect.mMinorRadius;
		// xyz: center, w: diffuse.x
		pPSPlanes->plCenter_DiffX[i].x = Rect.mCenter.x;
		pPSPlanes->plCenter_DiffX[i].y = Rect.mCenter.y;
		pPSPlanes->plCenter_DiffX[i].z = Rect.mCenter.z;
		pPSPlanes->plCenter_DiffX[i].w = Rect.mDiffuseColor.x;
		// xyz: center, w: diffuse.x
		pPSPlanes->plNorm_DifY[i].x = Rect.mNormal.x;
		pPSPlanes->plNorm_DifY[i].y = Rect.mNormal.y;
		pPSPlanes->plNorm_DifY[i].z = Rect.mNormal.z;
		pPSPlanes->plNorm_DifY[i].w = Rect.mDiffuseColor.y;
		// xyz: major axis, w: diffuse.z
		pPSPlanes->plAxis_DifZ[i].x = Rect.mMajorAxis.x;
		pPSPlanes->plAxis_DifZ[i].y = Rect.mMajorAxis.y;
		pPSPlanes->plAxis_DifZ[i].z = Rect.mMajorAxis.z;
		pPSPlanes->plAxis_DifZ[i].w = Rect.mDiffuseColor.z;
	}

	pd3dImmediateContext->Unmap(mCbPSRects, 0);
	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPlanesBind, 1, &mCbPSRects);
}

void CRenderInstance::OnFrameRender(ID3D11DeviceContext* pd3dImmediateContext)
{
	// Set the shaders
	pd3dImmediateContext->VSSetShader(mVertexShader, nullptr, 0);
	pd3dImmediateContext->PSSetShader(mPixelShader, nullptr, 0);

	UpdateVSConstants(pd3dImmediateContext);
	UpdatePSConstants(pd3dImmediateContext);
}

void CRenderInstance::CreateVertexShader(LPCWSTR pFileName, LPCSTR pEntrypoint,
	const D3D11_INPUT_ELEMENT_DESC* layout, UINT NumVertexElement, ID3D11Device* pd3dDevice)
{
	ID3DBlob* pVertexShaderBuffer = nullptr;
	HRESULT hr = DXUTCompileFromFile(pFileName, nullptr, pEntrypoint, "vs_5_0", dwShaderFlags, 0, &pVertexShaderBuffer);
	assert(SUCCEEDED(hr));

	// vertex shader
	assert(mVertexShader == nullptr);
	hr = (pd3dDevice->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(),
		pVertexShaderBuffer->GetBufferSize(), nullptr, &mVertexShader));
	assert(SUCCEEDED(hr));
	DXUT_SetDebugName(mVertexShader, "VSMain");
	assert(mVertexShader != nullptr);

	// vertex layout
	assert(mVertexLayout11 == nullptr);
	hr = (pd3dDevice->CreateInputLayout(layout, NumVertexElement, pVertexShaderBuffer->GetBufferPointer(),
		pVertexShaderBuffer->GetBufferSize(), &mVertexLayout11));
	assert(SUCCEEDED(hr));
	assert(mVertexLayout11 != nullptr);

	SAFE_RELEASE(pVertexShaderBuffer);

	// constant buffer: per instance
	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;

	Desc.ByteWidth = sizeof(CB_VS_PER_OBJECT);
	assert(mCbVSPerObject == nullptr);
	hr = (pd3dDevice->CreateBuffer(&Desc, nullptr, &mCbVSPerObject));
	assert(SUCCEEDED(hr));
	assert(mCbVSPerObject != nullptr);
}

void CRenderInstance::CreatePixelShader(LPCWSTR pFileName, LPCSTR pEntrypoint, ID3D11Device* pd3dDevice)
{
	ID3DBlob* pPixelShaderBuffer = nullptr;
	HRESULT hr = (DXUTCompileFromFile(pFileName, nullptr, pEntrypoint, "ps_5_0", dwShaderFlags, 0, &pPixelShaderBuffer));
	assert(SUCCEEDED(hr));

	assert(mPixelShader == nullptr);
	hr = (pd3dDevice->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(),
		pPixelShaderBuffer->GetBufferSize(), nullptr, &mPixelShader));
	assert(SUCCEEDED(hr));
	assert(mPixelShader != nullptr);

	SAFE_RELEASE(pPixelShaderBuffer);

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	// constant buffer: per instance
	Desc.ByteWidth = sizeof(CB_PS_PER_OBJECT);
	assert(mCbPSPerObject == nullptr);
	hr = (pd3dDevice->CreateBuffer(&Desc, nullptr, &mCbPSPerObject));
	assert(SUCCEEDED(hr));
	assert(mCbPSPerObject != nullptr);

	// constant buffer: planes
	Desc.ByteWidth = sizeof(CB_PS_PLANES);
	assert(mCbPSRects == nullptr);
	hr = (pd3dDevice->CreateBuffer(&Desc, nullptr, &mCbPSRects));
	assert(SUCCEEDED(hr));
	assert(mCbPSRects != nullptr);

	// constant buffer: per frame
	Desc.ByteWidth = sizeof(CB_PS_PER_FRAME);
	assert(mCbPSPerFrame == nullptr);
	hr = (pd3dDevice->CreateBuffer(&Desc, nullptr, &mCbPSPerFrame));
	assert(SUCCEEDED(hr));
	assert(mCbPSPerFrame != nullptr);
}
