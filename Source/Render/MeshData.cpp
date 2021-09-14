#include "DXUT.h"
#include "MeshData.h"
#include "RenderData.h"
#include "MiniEngine.h"
#include "RectProxy.h"

#pragma warning( disable : 4100 )

IMeshData::IMeshData()
	: mVB(nullptr)
	, mIB(nullptr)
	, mTexture(nullptr)
{

}

void IMeshData::DynamicUpdateVB(ID3D11Device* pd3dDevice)
{

}

CDxMesh* IMeshData::CreateDxMesh(LPCWSTR szFileName,ID3D11Device* pd3dDevice)
{
	CDxMesh* TheMesh = new CDxMesh;
	assert(TheMesh->mSdkMesh == nullptr);

	CDXUTSDKMesh* pMesh = new CDXUTSDKMesh;
	// create DXUT mesh from file
	HRESULT hr = pMesh->Create(pd3dDevice, szFileName);
	assert(SUCCEEDED(hr));
	// get vertex number
	UINT vertexNum = 0;
	pMesh->GetNumVertices(0, vertexNum);

	// binding mesh
	TheMesh->mSdkMesh = pMesh;
	// create device buffers
	TheMesh->CreateBuffers(pd3dDevice);

	return TheMesh;
}

CRectMesh* IMeshData::CreateRectMesh(ID3D11Device* pd3dDevice)
{
	CRectMesh* TheMesh = new CRectMesh;
	// create device buffers
	TheMesh->CreateBuffers(pd3dDevice);

	return TheMesh;
}

CCPUMesh* IMeshData::CreateCpuMesh(ID3D11Device* pd3dDevice)
{
	CCPUMesh* TheMesh = new CCPUMesh;
	return TheMesh;
}

void IMeshData::DestroyMesh(IMeshData** ppMeshData)
{
	IMeshData* pMeshData = *ppMeshData;
	if (pMeshData == nullptr)
		return;

	// destroy mesh data
	pMeshData->DestroyData();
	EMeshData::Type MeshType = pMeshData->GetMeshType();
	// call delete function for the corresponding mesh type
	if (MeshType == EMeshData::DxMesh)
	{
		delete (CDxMesh*)pMeshData;
	}
	else if (MeshType == EMeshData::RectMesh)
	{
		delete (CRectMesh*)pMeshData;
	}
	else if (MeshType == EMeshData::CPUMesh)
	{
		delete (CCPUMesh*)pMeshData;
	}
	else
	{
		assert(false);
	}

	*ppMeshData = nullptr;
}

ID3D11Buffer* IMeshData::GetVertexBuffer(ID3D11Device* pd3dDevice)
{
	DynamicUpdateVB(pd3dDevice);
	return mVB;
}

ID3D11Buffer* IMeshData::GetIndexBuffer()
{
	return mIB;
}

// default rectangle vertices
const static vector<Vertex_P3N3> GPlaneVertices{
		{ XMFLOAT3(-1.0f, -1.0f, .0f), XMFLOAT3(.0f, .0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, .0f), XMFLOAT3(.0f, .0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, .0f), XMFLOAT3(.0f, .0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, .0f), XMFLOAT3(.0f, .0f, 1.0f) },
};
// default rectangle indices
const static vector<WORD> GPlaneIndices{
		0,1,2,
		2,3,0,
};

CRectMesh::CRectMesh()
{

}

const vector<Vertex_P3N3>& CRectMesh::GetRectVertices()
{
	return GPlaneVertices;
}

const vector<WORD>& CRectMesh::GetRectIndices()
{
	return GPlaneIndices;
}

void CRectMesh::CreateBuffers(ID3D11Device* pd3dDevice)
{
	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex_P3N3) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = &GPlaneVertices[0];
	assert(mVB == nullptr);
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, &mVB);
	assert(SUCCEEDED(hr));
	assert(mVB != nullptr);

	// Create index buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 6; 
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = &GPlaneIndices[0];
	assert(mIB == nullptr);
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &mIB);
	assert(SUCCEEDED(hr));
	assert(mIB != nullptr);
}

void CRectMesh::DestroyData()
{
	// release buffers
	SAFE_RELEASE(mVB);
	SAFE_RELEASE(mIB);
}

const D3D11_INPUT_ELEMENT_DESC* CRectMesh::GetVertexDesc(UINT& OutNumElement)
{
	// vertex declaration
	const static D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	OutNumElement = ARRAYSIZE(layout);
	return layout;
}

UINT CRectMesh::GetVertexStride()
{
	return sizeof(Vertex_P3N3);
}

UINT CRectMesh::GetVertexNum()
{
	assert(GPlaneVertices.size() == 4);
	return (UINT)GPlaneVertices.size();
}

DXGI_FORMAT CRectMesh::GetIndexFormat()
{
	return DXGI_FORMAT_R16_UINT;
}

UINT CRectMesh::GetIndexNum()
{
	assert(GPlaneIndices.size() == 6);
	return (UINT)GPlaneIndices.size();
}

ID3D11ShaderResourceView* CRectMesh::GetTexture()
{
	return nullptr;
}

CCPUMesh::CCPUMesh()
{

}

void CCPUMesh::SetBufferData(const vector<Vertex_P3>& InVertices, const vector<WORD>& InIndices)
{
	mVertices = InVertices;
	mIndices = InIndices;
}

void CCPUMesh::CreateBuffers(ID3D11Device* pd3dDevice)
{
	assert(mVertices.size() >= 3);
	assert(mIndices.size() >= 3);

	// destroy buffers
	SAFE_RELEASE(mVB);
	SAFE_RELEASE(mIB);
	
	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex_P3N3) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = &mVertices[0];
	assert(mVB == nullptr);
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, &mVB);
	assert(SUCCEEDED(hr));
	assert(mVB != nullptr);

	// Create index buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = &mIndices[0];
	assert(mIB == nullptr);
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &mIB);
	assert(SUCCEEDED(hr));
	assert(mIB != nullptr);
}

void CCPUMesh::DynamicUpdateVB(ID3D11Device* pd3dDevice)
{
	CreateBuffers(pd3dDevice);
}

void CCPUMesh::DestroyData()
{
	SAFE_RELEASE(mVB);
	SAFE_RELEASE(mIB);
}

const D3D11_INPUT_ELEMENT_DESC* CCPUMesh::GetVertexDesc(UINT& OutNumElement)
{
	// vertex declaration
	const static D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	OutNumElement = ARRAYSIZE(layout);
	return layout;
}

UINT CCPUMesh::GetVertexStride()
{
	return sizeof(Vertex_P3);
}

UINT CCPUMesh::GetVertexNum()
{
	return (UINT)mVertices.size();
}

DXGI_FORMAT CCPUMesh::GetIndexFormat()
{
	return DXGI_FORMAT_R16_UINT;
}

UINT CCPUMesh::GetIndexNum()
{
	return (UINT)mIndices.size();
}

ID3D11ShaderResourceView* CCPUMesh::GetTexture()
{
	return nullptr;
}

void CCPUMesh::UpdateTestRectMesh(const string& RectMeshName, const string& PlaneMeshName)
{
	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

	CRenderInstance* MeshInst = MiniEngine.GetRenderInstance(PlaneMeshName);
	CRenderInstance* RectInst = MiniEngine.GetRenderInstance(RectMeshName);

	assert(MeshInst->mMeshData->GetMeshType() == EMeshData::RectMesh);
	assert(RectInst->mMeshData->GetMeshType() == EMeshData::CPUMesh);

	XMMATRIX mWorld = MeshInst->GetWorldMatrix();
	XMMATRIX mWVP = MeshInst->GetWVPMatrix();

	const vector<WORD>& SrcIndices = CRectMesh::GetRectIndices();
	const vector<Vertex_P3N3>& SrcVertices = CRectMesh::GetRectVertices();
	vector<Vertex_P3> DstVertices;

#if 1
	CRect PlaneRect;
	MeshInst->GetRectProxy(PlaneRect);
	assert(SrcVertices.size() == 4);
	PlaneRect.GetRenderVerticesWVP(MeshInst->mScale, DstVertices);
	RectInst->mCustomData0.x = PlaneRect.mNormal.x;
	RectInst->mCustomData0.y = PlaneRect.mNormal.y;
	RectInst->mCustomData0.z = PlaneRect.mNormal.z;
	RectInst->mDiffuseColor = MeshInst->mDiffuseColor;
#else
	DstVertices.reserve(SrcVertices.size());
	for (auto iter = begin(SrcVertices); iter != end(SrcVertices); ++iter)
	{
		XMVECTOR VecWorld = XMVector3TransformCoord(XMLoadFloat3(&iter->mPos), mWorld);
		XMVECTOR VecProj = XMVector3TransformCoord(XMLoadFloat3(&iter->mPos), mWVP);
		Vertex_P3 newVertex;
		XMStoreFloat3(&(newVertex.mPos), VecProj);
		DstVertices.push_back(newVertex);
	}

	// World Normal @OneColorPS.hlsl
	// @see GPlaneVertices
	const XMFLOAT3 GDefaultNormal = XMFLOAT3(.0f, .0f, 1.0f);
	XMVECTOR xNormal = XMVector3TransformNormal(XMLoadFloat3(&GDefaultNormal), mWorld);
	XMStoreFloat4(&(RectInst->mCustomData0), xNormal);
#endif

	CCPUMesh* CPUMesh = (CCPUMesh*)RectInst->mMeshData;
	CPUMesh->SetBufferData(DstVertices, SrcIndices);

	RectInst->mRender = true;
}

CDxMesh::CDxMesh()
	: mSdkMesh(nullptr)
{

}

CDxMesh::~CDxMesh()
{
	DestroyData();
}

void CDxMesh::CreateBuffers(ID3D11Device* pd3dDevice)
{
	// vertex buffer
	assert(mVB == nullptr);
	mVB = mSdkMesh->GetVB11(0, 0);
	assert(mVB != nullptr);

	// index buffer
	assert(mIB == nullptr);
	mIB = mSdkMesh->GetIB11(0);
	assert(mIB != nullptr);
}

void CDxMesh::DestroyData()
{
	if (mSdkMesh != nullptr)
	{
		mSdkMesh->Destroy();
		delete mSdkMesh;
		mSdkMesh = nullptr;
	}

	mVB = nullptr;
	mIB = nullptr;
}

const D3D11_INPUT_ELEMENT_DESC* CDxMesh::GetVertexDesc(UINT& OutNumElement)
{
	// vertex declaration
	const static D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	OutNumElement = ARRAYSIZE(layout);
	return layout;
}

UINT CDxMesh::GetVertexStride()
{
	return mSdkMesh->GetVertexStride(0, 0);
}

UINT CDxMesh::GetVertexNum()
{
	UINT nSubset = mSdkMesh->GetNumSubsets(0);
	assert(nSubset == 1);
	auto pSubset = mSdkMesh->GetSubset(0, 0);

	assert(pSubset->VertexCount > 0);
	return (UINT)pSubset->VertexCount;
}

DXGI_FORMAT CDxMesh::GetIndexFormat()
{
	return mSdkMesh->GetIBFormat11(0);
}

UINT CDxMesh::GetIndexNum()
{
	UINT nSubset = mSdkMesh->GetNumSubsets(0);
	assert(nSubset == 1);
	auto pSubset = mSdkMesh->GetSubset(0, 0);

	assert(pSubset->IndexCount > 0);
	return (UINT)pSubset->IndexCount;
}

ID3D11ShaderResourceView* CDxMesh::GetTexture()
{
	UINT nSubset = mSdkMesh->GetNumSubsets(0);
	assert(nSubset == 1);
	auto pSubset = mSdkMesh->GetSubset(0, 0);
	auto pDiffuseRV = mSdkMesh->GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
	
	return pDiffuseRV;
}
