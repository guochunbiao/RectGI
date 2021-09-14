#include "ShaderBuffers.fxc"

float4 main(float4 Pos : SV_POSITION) : SV_Target
{
    float3 DirectionalLightDirection = normalize(mLightDir);
    float3 WorldNormal = normalize(CustomData0.xyz);
    float NoL = max(0, dot(WorldNormal, DirectionalLightDirection));
    float3 OutColor = NoL;
    return float4(OutColor, 1);
}