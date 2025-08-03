#pragma once

#include <raylib.h>
#include <raymath.h>

#define LAMBERTIAN    0
#define METAL         1
#define DIELECTRIC    2
#define DIFFUSE_LIGHT 3

Vector3 FromColor(Color color) {
    return Vector3{int(color.r)/255.f, int(color.g)/255.f, int(color.b)/255.f};
}

struct Mat {
    Vector3 albedo;
    int mat_type;
    int tex_id;
    int emit_tex_id;
    float fuzz; // for metalic
    float refraction_index; // for dielectric

    Mat() {}

    Mat(Color _albedo, int _mat_type, int _tex_id, int _emit_tex_id, float _fuzz, float _refraction_index) :
    albedo(FromColor(_albedo)), mat_type(_mat_type), tex_id(_tex_id), emit_tex_id(_emit_tex_id), fuzz(_fuzz), refraction_index(_refraction_index) {}

    Mat(Vector3 _albedo, int _mat_type, int _tex_id, int _emit_tex_id, float _fuzz, float _refraction_index) :
    albedo(_albedo), mat_type(_mat_type), tex_id(_tex_id), emit_tex_id(_emit_tex_id), fuzz(_fuzz), refraction_index(_refraction_index) {}
};
