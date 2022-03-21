#pragma once

#define PI 3.1415926535898
#define DEGREES_TO_RANDIANS 0.017453292519943

///////////////////////////
struct Camera
{
    float3 origin;
    float3 lowerLeftCorner;
    float3 horizontal;
    float3 vertical;
    float3 u, v, w;
    float lensRadius;
};
static Camera MakeCamera(float3 origin, float3 lookAt, float3 up, float fov, float aspect, float aperture, float focusDist)
{
    Camera camera;
    camera.lensRadius = aperture / 2;
    float theta = fov * DEGREES_TO_RANDIANS;
    float halfHeight = tan(theta / 2);
    float halfWidth = aspect * halfHeight;
    camera.origin = origin;
    camera.w = normalize(origin - lookAt);
    camera.u = normalize(cross(up, camera.w));
    camera.v = cross(camera.w, camera.u);
    camera.lowerLeftCorner = origin - halfWidth * focusDist * camera.u - halfHeight * focusDist * camera.v - focusDist * camera.w;
    camera.horizontal = 2 * halfWidth * focusDist * camera.u;
    camera.vertical = 2 * halfHeight * focusDist * camera.v;
    return camera;
}

///////////////////////////
struct Sphere
{
    int material;
    float3 center;
    float radius;
};

///////////////////////////
struct Material
{
    int type;
    float3 albedo;
    float fuzziness;
    float refraction;
};

///////////////////////////
struct ComputeParams
{
    int frames;
    Camera camera;
    int count;
    float lerpFactor;
};
