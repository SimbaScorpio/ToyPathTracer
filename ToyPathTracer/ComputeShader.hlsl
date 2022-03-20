#include "Config.h"
#include "SharedDataStruct.h"

Texture2D srcImage : register(t0);
RWTexture2D<float4> dstImage : register(u0);
StructuredBuffer<ComputeParams> g_Params : register(t1);

inline uint RNG(inout uint seed)
{
    uint x = seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 15;
    seed = x;
    return x;
}

float RandomFloat01(inout uint seed)
{
    return (RNG(seed) & 0xFFFFFF) / 16777216.0f;
}

float3 RandomInUnitDisk(inout uint seed)
{
    float a = RandomFloat01(seed) * 2.0f * PI;
    float2 xy = float2(cos(a), sin(a));
    xy *= sqrt(RandomFloat01(seed));
    return float3(xy, 0);
}

float3 RandomInUnitSphere(inout uint seed)
{
    float z = RandomFloat01(seed) * 2.0f - 1.0f;
    float t = RandomFloat01(seed) * 2.0f * PI;
    float r = sqrt(max(0.0, 1.0f - z * z));
    float x = r * cos(t);
    float y = r * sin(t);
    float3 res = float3(x, y, z);
    res *= pow(RandomFloat01(seed), 1.0 / 3.0);
    return res;
}

float3 RandomUnitVector(inout uint seed)
{
    float z = RandomFloat01(seed) * 2.0f - 1.0f;
    float a = RandomFloat01(seed) * 2.0f * PI;
    float r = sqrt(1.0f - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return float3(x, y, z);
}

Ray CameraGetRay(Camera cam, float u, float v, inout uint seed)
{
    float3 rd = cam.lensRadius * RandomInUnitDisk(seed);
    float3 offset = cam.u * rd.x + cam.v * rd.y;
    float3 dir = normalize(cam.lowerLeftCorner + u * cam.horizontal + v * cam.vertical - cam.origin - offset);
    return MakeRay(cam.origin + offset, dir);
}

float3 Trace(Ray r, inout uint seed)
{
    float t = (r.dir.y + 1.0) / 2.0;
    float3 bgColor = lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t);
    return bgColor;
}

[numthreads(kCSGroupSizeX, kCSGroupSizeX, 1)]
void main(uint3 gid : SV_DispatchThreadID, uint3 tid : SV_GroupThreadID)
{
    ComputeParams params = g_Params[0];

    float3 color = 0;
    uint seed = (gid.x * 1973 + gid.y * 9277 + params.frames * 26699) | 1;
    for (int i = 0; i < SAMPLES_PER_PIXEL; ++i)
    {
        float u = float(gid.x + RandomFloat01(seed)) / kBackbufferWidth;
        float v = float(gid.y + RandomFloat01(seed)) / kBackbufferHeight;
        Ray ray = CameraGetRay(params.camera, u, v, seed);
        color += Trace(ray, seed);
    }
    color /= SAMPLES_PER_PIXEL;

    float3 prev = srcImage.Load(int3(gid.xy, 0)).rgb;
    float3 curr = lerp(color, prev, 0);

    dstImage[gid.xy] = float4(curr, 1.0);
}
