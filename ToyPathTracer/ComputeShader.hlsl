#include "Config.h"
#include "SharedDataStruct.h"

// SRV
Texture2D srcImage : register(t0);
StructuredBuffer<ComputeParams> g_Params : register(t1);
StructuredBuffer<Sphere> g_Spheres : register(t2);
StructuredBuffer<Material> g_Materials : register(t3);
// UAV
RWTexture2D<float4> dstImage : register(u0);

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
float3 Refract(float3 dir, float3 normal, float refraction)
{
    float theta = min(dot(-dir, normal), 1.0);
    float3 perp = refraction * (dir + theta * normal);
    float3 parallel = -sqrt(abs(1.0 - dot(perp, perp))) * normal;
    return perp + parallel;
}

struct HitRecord
{
    float3 position;
    float3 normal;
    bool isFrontFace;
    int material;
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
                record.material = sphere.material;
                tMax = step;
            }
        }
    }
    return hit;
}

bool Scatter(in HitRecord record, inout float3 color, inout Ray ray, inout uint seed)
{
    Material material = g_Materials[record.material];
    int type = material.type;

    // Lambertian
    if (type == 0)
    {   
        color *= material.albedo;
        ray.origin = record.position;
        ray.dir = normalize(record.normal + RandomUnitVector(seed));
        return true;
    }
    // Metal
    else if (type == 1)
    {
        color *= material.albedo;
        ray.origin = record.position;
        ray.dir = reflect(ray.dir, record.normal);
        ray.dir += material.fuzziness * RandomInUnitSphere(seed);
        ray.dir = normalize(ray.dir);
        return dot(ray.dir, record.normal) > 0;
    }
    // Dielectric
    else if (type == 2)
    {
        ray.origin = record.position;
        float refraction = material.refraction;
        if (record.isFrontFace)
        {
            refraction = 1.0 / refraction;
        }
        // Total Internal Reflection
        double cosTheta = min(dot(-ray.dir, record.normal), 1.0);
        double sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        bool cannotRefract = refraction * sinTheta > 1.0;

        // Use Schlick's approximation for reflectance
        float r0 = (1 - refraction) / (1 + refraction);
        r0 = r0 * r0;
        float reflectance = r0 + (1 - r0) * pow((1 - cosTheta), 5);

        if (cannotRefract || reflectance > RandomFloat01(seed))
            ray.dir = reflect(ray.dir, record.normal);
        else
            ray.dir = normalize(Refract(ray.dir, record.normal, refraction));

        return true;
    }
    return false;
}

float3 Trace(Ray ray, int count, inout uint seed)
{
    float3 color = 1;
    for (int depth = kMaxDepth; depth > 0; --depth)
    {
        HitRecord record;
        if (HitWorld(ray, kMinT, kMaxT, count, record))
        {
            if (!Scatter(record, color, ray, seed))
            {
                return float3(0, 0, 0);
            }
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
