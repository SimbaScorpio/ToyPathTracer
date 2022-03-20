#include "Config.h"
#include "SharedDataStruct.h"

Texture2D srcImage : register(t0);
RWTexture2D<float4> dstImage : register(u0);
StructuredBuffer<ComputeParams> g_Params : register(t1);
StructuredBuffer<Sphere> g_Spheres : register(t2);

///////////////////////////
struct Ray
{
    float3 origin;
    float3 dir;
};
Ray MakeRay(float3 origin, float3 dir)
{
    Ray r;
    r.origin = origin;
    r.dir = dir;
    return r;
}
float3 RayPointAt(Ray r, float t)
{
    return r.origin + r.dir * t;
}

struct HitRecord
{
    float3 position;
    float3 normal;
    bool isFrontFace;
};

///////////////////////////
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

bool HitWorld(Ray ray, float tMin, float tMax, int count, inout HitRecord record)
{
    bool hit = false;
    for (int i = 0; i < count; ++i)
    {
        Sphere sphere = g_Spheres[i];
        float3 oc = ray.origin - sphere.center;
        float b = dot(oc, ray.dir);
        float c = dot(oc, oc) - sphere.radius * sphere.radius;
        float discriminant = b * b - c;

        if (discriminant > 0)
        {
            float sqrtd = sqrt(discriminant);
            float step = -b - sqrtd;
            if (step <= tMin)
            {
                step = -b + sqrtd;
            }
            if (step > tMin && step < tMax)
            {
                hit = true;
                record.position = RayPointAt(ray, step);
                float3 normal = (record.position - sphere.center) / sphere.radius;
                record.isFrontFace = dot(ray.dir, normal) < 0;
                if (record.isFrontFace)
                    record.normal = normal;
                else
                    record.normal = -normal;
                tMax = step;
            }
        }
    }
    return hit;
}

float3 Trace(Ray ray, int count, inout uint seed)
{
    float3 color = 1;
    for (int depth = kMaxDepth; depth > 0; --depth)
    {
        HitRecord record;
        if (HitWorld(ray, kMinT, kMaxT, count, record))
        {
            color *= 0.5;//attenuation;
            ray.origin = record.position;
            ray.dir = normalize(record.normal + RandomUnitVector(seed));
        }
        else
        {
            float t = (ray.dir.y + 1.0) / 2.0;
            float3 bgColor = lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t);
            color *= bgColor;
            break;
        }
    }
    return color;
}

///////////////////////////
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
        color += Trace(ray, params.count, seed);
    }
    color /= float(SAMPLES_PER_PIXEL);

    float3 prev = srcImage.Load(int3(gid.xy, 0)).rgb;
    float3 curr = lerp(color, prev, params.lerpFactor);

    dstImage[gid.xy] = float4(curr, 1.0);
}
