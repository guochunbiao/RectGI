#include "ShaderBuffers.fxc"

// Textures and Samplers
Texture2D	g_txDiffuse : register( t0 );
SamplerState g_samLinear : register( s0 );

// Input / Output structures
struct PS_INPUT
{
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
};

// Pixel Shader
float4 main( PS_INPUT Input ) : SV_TARGET
{
	float4 vDiffuse = g_txDiffuse.Sample( g_samLinear, Input.vTexcoord );
	
	float fLighting = saturate( dot(mLightDir, Input.vNormal ) );
	fLighting = max( fLighting, mAmbient );
	
	return vDiffuse * fLighting;
}

