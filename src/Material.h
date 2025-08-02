#pragma once

#include <raylib.h>
#include <raymath.h>

#define LAMBERTIAN 0
#define METAL      1
#define DIELECTRIC 2

struct Mat {
    Color albedo;
    int mat_type;
    float fuzz; // for metalic
    float refraction_index; // for dielectric

    Mat() {}

    Mat(Color _albedo, int _mat_type, float _fuzz, float _refraction_index) :
    albedo(_albedo), mat_type(_mat_type), fuzz(_fuzz), refraction_index(_refraction_index) {}
};

Vector3 FromColor(Color color) {
    return Vector3{int(color.r)/255.f, int(color.g)/255.f, int(color.b)/255.f};
}