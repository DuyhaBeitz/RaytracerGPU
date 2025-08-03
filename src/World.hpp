#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <memory>

#include "Material.h"
#include "Hittable.h"

#define MATERIAL_COUNT 4
#define OBJECT_COUNT 2

class World {
public:

    World() {
        mats[0] = Mat(Vector3{0.2, 0.8, 0.2}, LAMBERTIAN, -1, 0.0, 0.0);
        mats[1] = Mat(Vector3{0.5, 0.5, 0.5}, LAMBERTIAN, 0, 0.0, 0.0);
        mats[2] = Mat(WHITE, METAL, -1, 0.0, 0.0);
        mats[3] = Mat(WHITE, DIELECTRIC, -1, 0.0, 1.5);

        //objects[0] = Hittable::Sphere(Vector3{0.0, -1000.5, 0.0}, 1000, 0);
        //objects[1] = Hittable::Sphere(Vector3{0.0, 1.0, 0.0}, 1, 1);
        objects[1] = Hittable::Plane(Vector3{0.0, 0.0, 0.0}, Vector3{1.0, 0.0, 0.0}, Vector3{0.0, 0.0, 1.0}, 1);
    }

    void ApplyUniforms(Shader& shader) {
        ApplyMaterialUniforms(shader);
        ApplyObjectUniforms(shader);
    }

private:

    Mat mats[MATERIAL_COUNT];
    Hittable objects[OBJECT_COUNT];

    void ApplyMaterialUniforms(Shader& shader) {
        Vector3 u_albedo[MATERIAL_COUNT];
        int u_mat_type[MATERIAL_COUNT];
        int u_tex_id[MATERIAL_COUNT];
        float u_fuzz[MATERIAL_COUNT];
        float u_refraction_index[MATERIAL_COUNT];

        for (int i = 0; i < MATERIAL_COUNT; i++) {
            u_albedo[i].x = mats[i].albedo.x;
            u_albedo[i].y = mats[i].albedo.y;
            u_albedo[i].z = mats[i].albedo.z;
            u_mat_type[i] = mats[i].mat_type;
            u_tex_id[i] = mats[i].tex_id;
            u_fuzz[i] = mats[i].fuzz;
            u_refraction_index[i] = mats[i].refraction_index;
        }
        
        SetShaderValueV(shader, GetShaderLocation(shader, "u_albedo"), &u_albedo[0].x, SHADER_UNIFORM_VEC3, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_type"), u_mat_type, SHADER_UNIFORM_INT, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_tex_id"), u_tex_id, SHADER_UNIFORM_INT, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_fuzz"), u_fuzz, SHADER_UNIFORM_FLOAT, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_refraction_index"), u_refraction_index, SHADER_UNIFORM_FLOAT, MATERIAL_COUNT);
    }

    void ApplyObjectUniforms(Shader& shader) {
        int u_geometry_type[OBJECT_COUNT];
        int u_mat_id[OBJECT_COUNT];
        Vector3 u_a[OBJECT_COUNT];
        Vector3 u_b[OBJECT_COUNT];
        Vector3 u_c[OBJECT_COUNT];

        for (int i = 0; i < OBJECT_COUNT; i++) {
            u_geometry_type[i] = objects[i].geometry_type;
            u_mat_id[i] = objects[i].material_id;
            u_a[i] = objects[i].a;
            u_b[i] = objects[i].b;
            u_c[i] = objects[i].c;
        }

        SetShaderValueV(shader, GetShaderLocation(shader, "u_geometry_type"), u_geometry_type, SHADER_UNIFORM_INT, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_id"), u_mat_id, SHADER_UNIFORM_INT, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_a"), &u_a[0].x, SHADER_UNIFORM_VEC3, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_b"), &u_b[0].x, SHADER_UNIFORM_VEC3, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_c"), &u_c[0].x, SHADER_UNIFORM_VEC3, MATERIAL_COUNT);
    }
};