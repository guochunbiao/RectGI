#include "DXUT.h"
#include "RectProxy.h"
#include "MeshData.h"
#include <DirectXMath.h>
#include "MiniEngine.h"
#include "RenderData.h"
#include <algorithm>

CRect::CRect()
	: mID(-2)
	, mMajorRadius(0)
	, mMinorRadius(0)
	, mRoughness(0.1f)
	, mDiffuseColor(1.f, 1.f, 1.f)
{
	
}

void CRect::GetRenderVerticesWorld(float InScale, vector<Vertex_P3>& OutVertices)
{
	XMVECTOR vNorm = XMVector3Normalize(XMLoadFloat3(&mNormal));
	XMVECTOR vMajor = XMVector3Normalize(XMLoadFloat3(&mMajorAxis));
	XMVECTOR vMinor = XMVector3Cross(vNorm, vMajor);
	vMinor = XMVector3Normalize(vMinor);
	
	XMVECTOR vCenter = XMLoadFloat3(&mCenter);

	// Create vertices
	XMVECTOR pts[4];
	pts[0] = vCenter + (-InScale) * vMajor + (-InScale) * vMinor;
	pts[1] = vCenter + ( InScale) * vMajor + (-InScale) * vMinor;
	pts[2] = vCenter + ( InScale) * vMajor + ( InScale) * vMinor;
	pts[3] = vCenter + (-InScale) * vMajor + ( InScale) * vMinor;

	OutVertices.reserve(4);
	for (int i = 0; i < 4; ++i)
	{
		Vertex_P3 vt;
		XMStoreFloat3(&vt.mPos, pts[i]);
		OutVertices.push_back(vt);
	}
}

void CRect::GetRenderVerticesWVP(float InScale, vector<Vertex_P3>& OutVertices)
{
	vector<Vertex_P3> WorldPts;
	GetRenderVerticesWorld(InScale, WorldPts);

	// Get the projection & view matrix from the camera class
	XMMATRIX mProj = CMiniEngine::GetInstance().mCamera.GetProjMatrix();
	XMMATRIX mView = CMiniEngine::GetInstance().mCamera.GetViewMatrix();
	XMMATRIX mViewProj = mView * mProj;

	OutVertices.reserve(WorldPts.size());
	for (auto iter = begin(WorldPts); iter != end(WorldPts); ++iter)
	{
		XMVECTOR ptWVP = XMVector3TransformCoord(XMLoadFloat3(&iter->mPos), mViewProj);
		Vertex_P3 vt;
		XMStoreFloat3(&vt.mPos, ptWVP);
		OutVertices.push_back(vt);
	}
}

CRectCollections::CRectCollections()
{

}

CRectCollections& CRectCollections::GetInstance()
{
	static CRectCollections GInstannce;
	return GInstannce;
}

void CRectCollections::UpdateAllProxies()
{
	// get render instances
	const map<string, CRenderInstance*>& AllRenderInst = CMiniEngine::GetInstance().mRenderInstances;

	INT16 rectIndex = 0;
	map<string, CRenderInstance*>::const_iterator iter;
	for (iter = AllRenderInst.begin(); iter != AllRenderInst.end(); ++iter)
	{
		const CRenderInstance* RenderInst = iter->second;
		if (RenderInst->mMeshData->GetMeshType() != EMeshData::RectMesh)
			continue;

		// get rect proxy of this render instance
		RenderInst->GetRectProxy(mAllRects[rectIndex ++]);
	}
	assert(rectIndex <= MAX_RECT_NUM);

	// sort rectangles by id
	std::sort(mAllRects, mAllRects + MAX_RECT_NUM,
		[](CRect const& a, CRect const& b) -> bool{ return a.mID < b.mID; });
	assert(rectIndex <= MAX_RECT_NUM);
}

void FUtils::CopyFloats(XMFLOAT4& Dst, const XMFLOAT3& Src)
{
	Dst.x = Src.x;
	Dst.y = Src.y;
	Dst.z = Src.z;
}
