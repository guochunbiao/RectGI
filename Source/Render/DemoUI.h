#pragma once

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"

using namespace std;
using namespace DirectX;

class CRenderInstance;

// UI class.
class CDemoUI
{
private:
	CDemoUI();

public:
	static CDemoUI& GetInstance();

	// Handling keyboard events.
	void OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);

	// Initialize.
	void InitGUI();
	// Create UI.
	void CreateGUI(ID3D11Device* pd3dDevice);
	// Destroy UI.
	void DestroyGUI();

	// Handling UI events. e.g. button clicked.
	void OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

	// Rendering UI.
	void RenderGUI(float fElapsedTime);

private:
	// Rendering Text.
	void RenderText();

public:
	// Manager for shared resources of dialogs
	CDXUTDialogResourceManager mDialogResourceManager; 
	// Helper class for displaying text.
	CDXUTTextHelper* mTxtHelper;
	// Whehter or not showing text
	bool mShowText;
	// Whether or not showing direct lighting.
	bool mShowDirectLighting;
	// Whether or not showing indirect diffuse lighting.
	bool mShowIndirectDiffuse;
	// Whether or not showing indirect specular lighting.
	bool mShowIndirectSpecular;
	// Whether or not using HDR.
	bool mShowHDR;
};