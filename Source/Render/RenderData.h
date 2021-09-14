#pragma once
#include <d3d11.h>
#include <string>
#include "RectProxy.h"

using namespace DirectX;
using namespace std;

class IMeshData;

// Define a render instance.
class CRenderInstance
{
public:
	CRenderInstance(const string& InName);

	// Create render instance for a rectangle.
	static CRenderInstance* CreateRectInstance(ID3D11Device* pd3dDevice,
		const string& InName, const XMFLOAT3& InPos, const XMFLOAT3& InRot, 
		const float InScale, float InRoughness,
		const XMFLOAT3& InDiffuseColor, INT16 inID);
	// Create render instance for a test object.
	static CRenderInstance* CreateTestRectInstance(ID3D11Device* pd3dDevice, const string& InName);

	// Set position to current render instance.
	void SetPosition(const XMFLOAT3& InPosition);
	// Set rotation to current render instance.
	void SetRotation(float InPitch, float InYaw, float InRoll);
	// Set scale to current render instance.
	void SetScale(float InScale);

	// Destroy current render instance.
	void Destroy();

	// Create vertex shader for current render instance.
	void CreateVertexShader(LPCWSTR pFileName, LPCSTR pEntrypoint,
		const D3D11_INPUT_ELEMENT_DESC* layout, UINT NumVertexElement, ID3D11Device* pd3dDevice);
	// Create pixel shader for current render instance.
	void CreatePixelShader(LPCWSTR pFileName, LPCSTR pEntrypoint, ID3D11Device* pd3dDevice);

	// Call each frame from renderer.
	void OnFrameRender(ID3D11DeviceContext* pd3dImmediateContext);

	// Get the world transform matrix.
	XMMATRIX GetWorldMatrix() const;
	// Get the world*view*projection matrix.
	XMMATRIX GetWVPMatrix() const;

	// Get rectangle proxy for this render instance.
	void GetRectProxy(class CRect& OutRect) const;

	// Link possible reflectors to this render instance.
	static void LinkReflectors(const string& RecvName, const vector<string>& ReflNames);
	// Unlink all reflectors.
	void UnlinkReflectors();

private:
	// Update constants buffer for vertex shader.
	void UpdateVSConstants(ID3D11DeviceContext* pd3dImmediateContext);
	// Update constants buffer for pixel shader.
	void UpdatePSConstants(ID3D11DeviceContext* pd3dImmediateContext);

public:
	// Related mesh data.
	IMeshData* mMeshData;
	// Instance name.
	string mName;
	// id
	INT16 mID;

	// whether or not this render instance needs to be rendered
	bool mRender;
	// is this a reflector
	bool mReflector;
	// is this a receiver
	bool mReceiver;

	// world position
	XMFLOAT3 mPosition;
	// rotation properties
	float mPitch;
	float mYaw;
	float mRoll;
	float mScale;
	// material roughness
	float mRoughness;
	// diffuse color
	XMFLOAT3 mDiffuseColor;

	// custom data passing to shaders
	XMFLOAT4 mCustomData0;

	// the cull mode of render instance
	bool mCull;

	// related reflector indices
	INT16 mReflectorIndices[MAX_RELATED_REFLECTOR_NUM];

	// vertex layout
	ID3D11InputLayout* mVertexLayout11;
	// vertex shader
	ID3D11VertexShader* mVertexShader;
	// constant buffer of vertex shader
	ID3D11Buffer* mCbVSPerObject;
	
	// pixel shader
	ID3D11PixelShader* mPixelShader;
	// constant buffers of pixel shader
	ID3D11Buffer* mCbPSPerObject;
	ID3D11Buffer* mCbPSPerFrame;
	ID3D11Buffer* mCbPSRects;
};
