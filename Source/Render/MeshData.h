#pragma once
#include "SDKmesh.h"
#include <d3d11.h>
#include <string>

using namespace DirectX;
using namespace std;

class CDxMesh;
class CRectMesh;
class CCPUMesh;

// Define mesh types.
namespace EMeshData
{
	enum Type
	{
		// DXUT built-in mesh type
		DxMesh = 0,
		// Rectangle mesh
		RectMesh,
		// Mesh with vertices being handled on CPU
		CPUMesh,

		// Undefined mesh type
		Unknown
	};
};

// Vertex type with Position(3floats)
struct Vertex_P3
{
	XMFLOAT3 mPos;
};

// Vertex type with Position(3floats) and Normal(3floats)
struct Vertex_P3N3
{
	XMFLOAT3 mPos;
	XMFLOAT3 mNormal;
};

// Defines the data source of a mesh.
class IMeshData
{
public:
	IMeshData();

	// Get current mesh type.
	virtual EMeshData::Type GetMeshType() = 0;

	// Create rendering buffers for current mesh.
	virtual void CreateBuffers(ID3D11Device* pd3dDevice) = 0;
	// Destroy rendering buffers.
	virtual void DestroyData() = 0;

	// Get vertex buffer of current mesh.
	ID3D11Buffer* GetVertexBuffer(ID3D11Device* pd3dDevice);
	// Get vertex description of current mesh.
	virtual const D3D11_INPUT_ELEMENT_DESC* GetVertexDesc(UINT& OutNumElement) = 0;
	// Get vertex stride of current mesh.
	virtual UINT GetVertexStride() = 0;
	// Get vertex number of current mesh.
	virtual UINT GetVertexNum() = 0;

	// Get index buffer of current mesh.
	ID3D11Buffer* GetIndexBuffer();
	// Get index format of current mesh.
	virtual DXGI_FORMAT GetIndexFormat()
	{
		return DXGI_FORMAT_R16_UINT;
	}
	// Get index number of current mesh.
	virtual UINT GetIndexNum() = 0;

	// Get textures of current mesh if there is any.
	virtual ID3D11ShaderResourceView* GetTexture() = 0;

protected:
	// Updating vertex buffer for current mesh.
	virtual void DynamicUpdateVB(ID3D11Device* pd3dDevice);

public:
	// Create a DXUT built-in mesh.
	static CDxMesh* CreateDxMesh(LPCWSTR szFileName, ID3D11Device* pd3dDevice);
	// Create a rectangle mesh.
	static CRectMesh* CreateRectMesh(ID3D11Device* pd3dDevice);
	// Create a cpu mesh.
	static CCPUMesh* CreateCpuMesh(ID3D11Device* pd3dDevice);

	// Release mesh memory.
	static void DestroyMesh(IMeshData** ppMeshData);

protected:
	// vertex buffer
	ID3D11Buffer* mVB;
	// index buffer
	ID3D11Buffer* mIB;

	// texture
	ID3D11ShaderResourceView* mTexture;
};

// Rectangle mesh.
class CRectMesh : public IMeshData
{
	friend class IMeshData;

public:
	CRectMesh();

	// Get vertices of rectangle mesh.
	const static vector<Vertex_P3N3>& GetRectVertices();
	// Get indices of rectangle mesh.
	const static vector<WORD>& GetRectIndices();

	// Get mesh type.
	virtual EMeshData::Type GetMeshType() override { return EMeshData::RectMesh; }

	// Create rendering buffers for current mesh.
	virtual void CreateBuffers(ID3D11Device* pd3dDevice) override;
	// Destroy rendering buffers.
	virtual void DestroyData() override;

	// Get vertex description of current mesh.
	virtual const D3D11_INPUT_ELEMENT_DESC* GetVertexDesc(UINT& OutNumElement) override;
	// Get vertex stride of current mesh.
	virtual UINT GetVertexStride() override;
	// Get vertex number of current mesh.
	virtual UINT GetVertexNum() override;

	// Get index format of current mesh.
	virtual DXGI_FORMAT GetIndexFormat() override;
	// Get index number of current mesh.
	virtual UINT GetIndexNum() override;

	// Get textures of current mesh if there is any.
	virtual ID3D11ShaderResourceView* GetTexture() override;
};

// Mesh with vertices being handled on CPU
class CCPUMesh : public IMeshData
{
	friend class IMeshData;

public:
	CCPUMesh();

	// Get mesh type.
	virtual EMeshData::Type GetMeshType() override { return EMeshData::CPUMesh; }

	// Update buffer data.
	void SetBufferData(const vector<Vertex_P3>& InVertices, const vector<WORD>& InIndices);

	// Create rendering buffers for current mesh.
	virtual void CreateBuffers(ID3D11Device* pd3dDevice) override;
	// Destroy rendering buffers.
	virtual void DestroyData() override;

	// Get vertex description of current mesh.
	virtual const D3D11_INPUT_ELEMENT_DESC* GetVertexDesc(UINT& OutNumElement) override;
	// Get vertex stride of current mesh.
	virtual UINT GetVertexStride() override;
	// Get vertex number of current mesh.
	virtual UINT GetVertexNum() override;

	// Get index format of current mesh.
	virtual DXGI_FORMAT GetIndexFormat() override;
	// Get index number of current mesh.
	virtual UINT GetIndexNum() override;

	// Get textures of current mesh if there is any.
	virtual ID3D11ShaderResourceView* GetTexture() override;

	// Update buffer with test data.
	static void UpdateTestRectMesh(const string& RectMeshName, const string& PlaneMeshName);

protected:
	// Updating vertex buffer for current mesh.
	virtual void DynamicUpdateVB(ID3D11Device* pd3dDevice) override;

private:
	// transformed vertex buffer data
	vector<Vertex_P3> mVertices;
	// index buffer data
	vector<WORD> mIndices;
};

// DXUT built-in mesh.
class CDxMesh : public IMeshData
{
	friend class IMeshData;

public:
	CDxMesh();
	~CDxMesh();

	// Get mesh type.
	virtual EMeshData::Type GetMeshType() override { return EMeshData::DxMesh; }

	// Create rendering buffers for current mesh.
	virtual void CreateBuffers(ID3D11Device* pd3dDevice) override;
	// Destroy rendering buffers.
	virtual void DestroyData() override;

	// Get vertex description of current mesh.
	virtual const D3D11_INPUT_ELEMENT_DESC* GetVertexDesc(UINT& OutNumElement) override;
	// Get vertex stride of current mesh.
	virtual UINT GetVertexStride() override;
	// Get vertex number of current mesh.
	virtual UINT GetVertexNum() override;

	// Get index format of current mesh.
	virtual DXGI_FORMAT GetIndexFormat() override;
	// Get index number of current mesh.
	virtual UINT GetIndexNum() override;

	// Get textures of current mesh if there is any.
	virtual ID3D11ShaderResourceView* GetTexture() override;

private:
	// DXUT mesh
	CDXUTSDKMesh* mSdkMesh;
};