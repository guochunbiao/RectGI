#include "DXUT.h"

#include "DemoUI.h"
#include "MiniEngine.h"
#include "RenderData.h"
#include "MeshData.h"
#include <cstdint>
#include <algorithm>
#include <complex>
#include <corecrt_math_defines.h>

#pragma warning( disable : 4100 )

CDemoUI::CDemoUI()
	: mShowIndirectSpecular(true)
	, mShowIndirectDiffuse(true)
	, mShowDirectLighting(true)
	, mShowHDR(true)
	, mTxtHelper(nullptr)
	, mShowText(true)
{

}

CDemoUI& CDemoUI::GetInstance()
{
	static CDemoUI GInstance;
	return GInstance;
}

void CDemoUI::OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1:
		{
			// toggle direct lighting
			mShowDirectLighting = !mShowDirectLighting;
		}
		break;
		case VK_F2:
		{
			// toggle indirect diffuse lighting
			mShowIndirectDiffuse = !mShowIndirectDiffuse;
		}
		break;
		case VK_F3:
		{
			// toggle indirect specular lighting
			mShowIndirectSpecular = !mShowIndirectSpecular;
		}
		break;
		//case 'L':
		//{
		//	CMiniEngine& MiniEngine = CMiniEngine::GetInstance();

		//	static int GLightPositionIndex = 0;
		//	int lightMul = (++GLightPositionIndex) % 4;
		//	XMFLOAT3 newLightPos = XMFLOAT3(
		//		float(std::cos(M_PI * 0.25 + M_PI * lightMul * .5f)) * 300, 
		//		float(std::sin(M_PI * 0.25 + M_PI * lightMul * .5f)) * 300,
		//		-300);
		//	MiniEngine.SetLightPosition(newLightPos);
		//}
		//break;
		//case 'K':
		//{
		//	CMiniEngine::GetInstance().TakeScreenshot();
		//}
		//break;
		}
	}
}

void CALLBACK OnGUIEventCallback(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	CDemoUI::GetInstance().OnGUIEvent(nEvent, nControlID, pControl, pUserContext);
}

void CDemoUI::InitGUI()
{

}

void CDemoUI::CreateGUI(ID3D11Device* pd3dDevice)
{
	// Create DXUT GUI classes
	HRESULT hr;
	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	hr = (mDialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	assert(SUCCEEDED(hr));
	mTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &mDialogResourceManager, 15);
}

void CDemoUI::DestroyGUI()
{
	// Destroy DXUT related GUI classes.
	mDialogResourceManager.OnD3D11DestroyDevice();

	SAFE_DELETE(mTxtHelper);
}

void CDemoUI::OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{

}

void CDemoUI::RenderGUI(float fElapsedTime)
{
	if (!mShowText)
		return;

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	// Display text
	RenderText();
	DXUT_EndPerfEvent();
}

void CDemoUI::RenderText()
{
	// begin rendering text
	mTxtHelper->Begin();

	// text position and color
	mTxtHelper->SetInsertionPos(2, 0);
	mTxtHelper->SetForegroundColor(Colors::Yellow);

	// text content
	{
		WCHAR sz[255];
		swprintf_s(sz, 255,
			L"Direct Lighting(F1): %s\n"
			L"Indirect Diffuse(F2): %s\n"
			L"Indirect Specular(F3): %s\n",
			mShowDirectLighting ? L"On" : L"Off",
			mShowIndirectDiffuse ? L"On" : L"Off",
			mShowIndirectSpecular ? L"On" : L"Off"
			);
		mTxtHelper->DrawTextLine(sz);
	}

	// end rendering text
	mTxtHelper->End();
}
