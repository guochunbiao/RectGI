#include "ShaderBuffers.fxc"

// input structure
struct VS_INPUT
{
	float4 vPosition	: POSITION;
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
};

// output structure
struct VS_OUTPUT
{
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosition	: SV_POSITION;
};

// vertex shader
VS_OUTPUT main( VS_INPUT Input )
{
	VS_OUTPUT Output;
	
	Output.vPosition = mul( Input.vPosition, World);
	Output.vPosition = mul(Output.vPosition, View);
	Output.vPosition = mul(Output.vPosition, Proj);
	Output.vNormal = mul( Input.vNormal, (float3x3)World);
	Output.vTexcoord = Input.vTexcoord;
	
	return Output;
}

