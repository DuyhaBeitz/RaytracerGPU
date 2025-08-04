#pragma once

#include <raylib.h>
#include <raymath.h>

#define EMPTY_GEOM -1
#define SPHERE      0
#define PLANE       1
#define QUAD        2

struct Hittable {
    int geometry_type = EMPTY_GEOM;
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

    static Hittable Plane(Vector3 _Q, Vector3 _U, Vector3 _V, int _material_id) {
        return Hittable(PLANE, _material_id, _Q, _U, _V);
    }

    static Hittable Quad(Vector3 _Q, Vector3 _U, Vector3 _V, int _material_id) {
        return Hittable(QUAD, _material_id, _Q, _U, _V);
    }

    static void Box(Hittable objects[], int i, Vector3 Origin, Vector3 V1, Vector3 V2, Vector3 V3, int mat_id) {
        objects[i] = Quad(Origin, V1, V2, mat_id);
        objects[i+1] = Quad(Vector3Add(Origin, V3), V1, V2, mat_id);

        objects[i+2] = Quad(Origin, V2, V3, mat_id);
        objects[i+3] = Quad(Vector3Add(Origin, V1), V2, V3, mat_id);

        objects[i+4] = Quad(Origin, V1, V3, mat_id);
        objects[i+5] = Quad(Vector3Add(Origin, V2), V1, V3, mat_id);
    }

    static void RotateBox(Hittable objects[], int i, Vector3 axis, float degrees) {
        Vector3 Origin = objects[i].a;
        Vector3 V1     = objects[i].b;
        Vector3 V2     = objects[i].c;
        Vector3 V3     = objects[i+2].c;

        float radians = degrees * M_PI / 180.0;

        V1 = Vector3RotateByAxisAngle(V1, axis, radians);
        V2 = Vector3RotateByAxisAngle(V2, axis, radians);
        V3 = Vector3RotateByAxisAngle(V3, axis, radians);
        Box(objects, i, Origin, V1, V2, V3, objects[i].material_id);
    }
};

