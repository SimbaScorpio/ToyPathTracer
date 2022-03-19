#include "Config.h"

struct Params
{
    int frames;
};

Texture2D srcImage : register(t0);
RWTexture2D<float4> dstImage : register(u0);
StructuredBuffer<Params> g_Params : register(t1);

[numthreads(kCSGroupSizeX, kCSGroupSizeX, 1)]
void main(uint3 gid : SV_DispatchThreadID, uint3 tid : SV_GroupThreadID)
{
    Params params = g_Params[0];
    float r = sin(params.frames / 2000.0f) * 0.5 + 0.5;
    float3 curr = { r, 0.2f, 0.3f };
    float3 prev = srcImage.Load(int3(gid.xy, 0)).rgb;
    curr = lerp(curr, prev, 0.5);
    dstImage[gid.xy] = float4(curr, 1);
}
