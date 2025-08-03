#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <memory>

#include "Material.h"
#include "Hittable.h"

#include "CameraControl.hpp"

#define MATERIAL_COUNT 5
#define OBJECT_COUNT 4

class World {
public:

    World()
    : camera(std::make_shared<CameraControl>(3840, 2160)) 
    {
        camera->vfov     = 90;
        camera->Position = Vector3{26,3,6};
        camera->yaw   = M_PI*1.41;

        // GROUND
        //mats[0] = Mat(DARKGRAY, LAMBERTIAN, -1, -1, 0.0, 0.0);
        //objects[0] = Hittable::Sphere(Vector3{0, -1000, 0}, 1000, 0);

        // SPHERE
        //mats[1] = Mat(GRAY, LAMBERTIAN, 0, -1, 0.0, 1/1.3);
        mats[1] = Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3);
        objects[1] = Hittable::Sphere(Vector3{0, 2, 0}, 2, 1);        

        // LIGHT QUAD
        mats[2] = Mat(Vector3{1.0, 1.0, 1.0}, DIFFUSE_LIGHT, -1, -1, 0.0, 0.0);
        objects[2] = Hittable::Quad(Vector3{3, 1, -2}, Vector3{4, 0, 0}, Vector3{0, 4, 0}, 2);

        // LIGHT SPHERE
        objects[3] = Hittable::Sphere(Vector3{0, 7, 0}, 2, 2);        
    }

    void Update() {
        camera->UpdatePosition();
        camera->UpdateForward();
        HandleInput();

        //objects[1].a.x += GetFrameTime();
        //objects[3].a.y += sinf(runTime*30)/500;

        runTime += GetFrameTime();
    }

    void ApplyUniforms(Shader& shader) {

        SetShaderValue(shader, GetShaderLocation(shader, "iTime"), &runTime, SHADER_UNIFORM_FLOAT);
        float Resolution[2] = {
            float(camera->GetRenderWith()),
            float(camera->GetRenderHeight())
        };
        SetShaderValue(shader, GetShaderLocation(shader, "iResolution"), &Resolution, SHADER_UNIFORM_VEC2);
        int ionly_normals = only_normals ? 1 : 0;
        SetShaderValue(shader, GetShaderLocation(shader, "ionly_normals"), &ionly_normals, SHADER_UNIFORM_INT);
        camera->ApplyUniforms(shader);

        ApplyMaterialUniforms(shader);
        ApplyObjectUniforms(shader);
    }

    std::shared_ptr<CameraControl> GetCamera() { return camera; }

private:

    Mat mats[MATERIAL_COUNT];
    Hittable objects[OBJECT_COUNT];
    std::shared_ptr<CameraControl> camera;
    
    float runTime = 0.f;
    bool only_normals = false;

    void ApplyMaterialUniforms(Shader& shader) {
        Vector3 u_albedo[MATERIAL_COUNT];
        int u_mat_type[MATERIAL_COUNT];
        int u_tex_id[MATERIAL_COUNT];
        int u_emit_tex_id[MATERIAL_COUNT];
        float u_fuzz[MATERIAL_COUNT];
        float u_refraction_index[MATERIAL_COUNT];

        for (int i = 0; i < MATERIAL_COUNT; i++) {
            u_albedo[i].x = mats[i].albedo.x;
            u_albedo[i].y = mats[i].albedo.y;
            u_albedo[i].z = mats[i].albedo.z;
            u_mat_type[i] = mats[i].mat_type;
            u_tex_id[i] = mats[i].tex_id;
            u_emit_tex_id[i] = mats[i].emit_tex_id;
            u_fuzz[i] = mats[i].fuzz;
            u_refraction_index[i] = mats[i].refraction_index;
        }
        
        SetShaderValueV(shader, GetShaderLocation(shader, "u_albedo"), &u_albedo[0].x, SHADER_UNIFORM_VEC3, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_type"), u_mat_type, SHADER_UNIFORM_INT, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_tex_id"), u_tex_id, SHADER_UNIFORM_INT, MATERIAL_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_emit_tex_id"), u_emit_tex_id, SHADER_UNIFORM_INT, MATERIAL_COUNT);
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

    void HandleInput() {
        if (IsKeyPressed(KEY_N)) {
            only_normals = !only_normals;
            camera->RestartAccum();
        }
        if (IsKeyPressed(KEY_UP)) {
            camera->NewResolution(camera->GetRenderWith()*2, camera->GetRenderHeight()*2);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            camera->NewResolution(camera->GetRenderWith()/2, camera->GetRenderHeight()/2);
        }
    }

};