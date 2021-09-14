#include "ShaderBuffers.fxc"
#include "RectGI.hlsl"

struct PS_INPUT
{
	float3 Normal : NORMAL;
	float4 WorldPos : TEXCOORD0;
};

float D_GGX(float a2, float NoH)
{
	float NoH2 = NoH * NoH;
	float d = NoH2 * (a2 - 1) + 1;
	return a2 / (PI * d * d);
}

float Vis_SmithJointApprox(float a2, float NoV, float NoL)
{
	float a = sqrt(a2);
	float Vis_SmithV = NoL * (NoV * (1 - a) + a);
	float Vis_SmithL = NoV * (NoL * (1 - a) + a);
	return 0.5 * rcp(Vis_SmithV + Vis_SmithL);
}

float3 F_Schlick(float3 SpecularColor, float VoH)
{
	float a1 = 1 - VoH;
	float a2 = a1 * a1;
	float a4 = a2 * a2;
	float Fc = a4 * a1;	
	return saturate(50.0 * SpecularColor.g) * Fc + (1 - Fc) * SpecularColor;
}

float4 main( PS_INPUT Input ) : SV_TARGET
{
	float3 WorldNormal = normalize(Input.Normal);
	float3 WorldPos = Input.WorldPos.xyz;

	bool bShowSpecularGI = mToggleOptionsA[0];
	bool bShowBRDF = mToggleOptionsA[1];
	bool bOnlyNoL = mToggleOptionsA[2];
	bool bShowDiffuseGI = mToggleOptionsA[3];
	
	float Roughness = Roughness4.x;
	float a2 = Roughness * Roughness;

	float3 CameraVector = normalize(CameraPos.xyz - WorldPos);
	float3 DirectionalLightDirection = normalize(mLightDir);
	float NoV = max(0, dot(WorldNormal, CameraVector));
	float NoL = max(0, dot(WorldNormal, DirectionalLightDirection));
	float3 H = normalize(CameraVector + DirectionalLightDirection);
	float NoH = max(0, dot(WorldNormal, H));
	float VoH = max(0, dot(CameraVector, H));

	float Vis = Vis_SmithJointApprox(a2, NoV, NoL);
	float D = D_GGX(a2, NoH);
	float3 MySpecularColor = float3(1, 1, 1);
	float3 F = F_Schlick(MySpecularColor, VoH);
	
	// direct lighting
	float3 OutColor = 0;
	if (bShowBRDF)
	{
		OutColor += DiffuseColor.xyz * D * F * Vis * NoL;
	}

	// indirect lighting
	OutColor *= LightIntensity;
	OutColor += GILighting(WorldPos, WorldNormal, Roughness, DirectionalLightDirection, CameraPos.xyz);

	return float4(OutColor, 1);
}

