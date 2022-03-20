#pragma once

#include <math.h>

struct float3
{
    float3() : x(0), y(0), z(0) {}
    float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float3 operator-() const { return float3(-x, -y, -z); }
    float3& operator+=(const float3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    float3& operator-=(const float3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    float3& operator*=(const float3& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
    float3& operator*=(float o) { x *= o; y *= o; z *= o; return *this; }

    inline float getX() const { return x; }
    inline float getY() const { return y; }
    inline float getZ() const { return z; }
    inline void setX(float x_) { x = x_; }
    inline void setY(float y_) { y = y_; }
    inline void setZ(float z_) { z = z_; }
    inline void store(float* p) const { p[0] = getX(); p[1] = getY(); p[2] = getZ(); }

    float x, y, z;
};

inline float3 operator+(const float3& a, const float3& b) { return float3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline float3 operator-(const float3& a, const float3& b) { return float3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline float3 operator*(const float3& a, const float3& b) { return float3(a.x * b.x, a.y * b.y, a.z * b.z); }
inline float3 operator*(const float3& a, float b) { return float3(a.x * b, a.y * b, a.z * b); }
inline float3 operator*(float a, const float3& b) { return float3(a * b.x, a * b.y, a * b.z); }
inline float dot(const float3& a, const float3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float3 cross(const float3& a, const float3& b)
{
    return float3(
        a.y * b.z - a.z * b.y,
        -(a.x * b.z - a.z * b.x),
        a.x * b.y - a.y * b.x
    );
}

inline float length(float3 v) { return sqrtf(dot(v, v)); }
inline float sqLength(float3 v) { return dot(v, v); }
inline float3 normalize(float3 v) { return v * (1.0f / length(v)); }
inline float3 lerp(float3 a, float3 b, float t) { return a + (b - a) * t; }
