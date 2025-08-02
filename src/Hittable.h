#pragma once

#include <raylib.h>
#include <raymath.h>

#define SPHERE 0

struct Hittable {
    int geometry_type;
    int material_id;
    Vector3 a;
    Vector3 b;
    Vector3 c;

    Hittable () {}
    Hittable (int _geometry_type, int _material_id, Vector3 _a, Vector3 _b = Vector3{0.0}, Vector3 _c = Vector3{0.0}) :
    geometry_type(_geometry_type), material_id(_material_id), a(_a), b(_b), c(_c) {}

    
    static Hittable Sphere(Vector3 _center, float _radius, int _material_id) {
        return Hittable(SPHERE, _material_id, _center, Vector3{_radius, 0.0, 0.0});
    }
};

