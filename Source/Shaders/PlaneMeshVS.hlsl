#include "ShaderBuffers.fxc"

struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float3 Normal : NORMAL;
	float4 WorldPos : TEXCOORD0;
	float4 Pos : SV_POSITION;
};

VS_OUTPUT main( VS_INPUT Input )
{
	VS_OUTPUT Output;
	
	float4 WorldPos = mul(Input.Pos, World);
	Output.WorldPos = WorldPos;
	Output.Pos = mul(WorldPos, View);
	Output.Pos = mul(Output.Pos, Proj);

	Output.Normal = normalize(mul( Input.Normal, (float3x3)World));
	
	return Output;
}

