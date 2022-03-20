#pragma once

#include "Config.h"

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
Camera MakeCamera(float3 origin, float3 lookAt, float3 up, float fov, float aspect, float aperture, float focusDist)
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
    float3 center;
    float radius;
};

///////////////////////////
struct ComputeParams
{
    int frames;
    Camera camera;
};
