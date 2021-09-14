#pragma once
#include <d3d11.h>
#include <string>
#include <vector>

using namespace DirectX;
using namespace std;

#define MAX_RECT_NUM 8
#define MAX_RELATED_REFLECTOR_NUM 6
#define INVALID_PLANE_ID -2

// Define a reflector rectangle.
class CRect
{
public:
	CRect();

	// Get world position of vertices.
	void GetRenderVerticesWorld(float InScale, vector<struct Vertex_P3>& OutVertices);
	// Get perspective projection position of vertices.
	void GetRenderVerticesWVP(float InScale, vector<struct Vertex_P3>& OutVertices);

public:
	// id
	INT16 mID;
	// center position
	XMFLOAT3 mCenter;
	// normal
	XMFLOAT3 mNormal;
	// major axis
	XMFLOAT3 mMajorAxis;
	// diffuse color
	XMFLOAT3 mDiffuseColor;
	// half extent of major axis
	float mMajorRadius;
	// half extent of minor axis
	float mMinorRadius;
	// material roughness
	float mRoughness;
};

// Collection of rectangles.
class CRectCollections
{
private:
	CRectCollections();

public:
	static CRectCollections& GetInstance();

	// Update relationship for all rectangle proxies.
	void UpdateAllProxies();

public:
	// All rectangles
	CRect mAllRects[MAX_RECT_NUM];
};

// Utility class.
class FUtils
{
public:
	// Copy from XMFLOAT3 to XMFLOAT4
	static void CopyFloats(XMFLOAT4& Dst, const XMFLOAT3& Src);

};
