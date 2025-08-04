#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <memory>

#include "Material.h"
#include "Hittable.h"

#include "CameraControl.hpp"

#define MATERIAL_COUNT 100
#define OBJECT_COUNT 18

// Scenes
#define SC_ROOM        0
#define SC_DARK        1
#define SC_CORNELL_BOX 2

// Procedural sky
#define SKY_DARK -2
#define SKY_BLUE -1

#define downscale 1.0

class World {
public:

    World(int _scene = SC_ROOM)
    : camera(std::make_shared<CameraControl>(3840/downscale, 2160/downscale)), scene(_scene)
    {
        switch (scene)
        {
        case SC_ROOM:
            Room();
            break;
        case SC_DARK:
            Dark();
            break;
        case SC_CORNELL_BOX:
            CornellBox();
            break;
        default:
            break;
        }
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

    std::vector<Texture2D> LoadTexturesForScene() {
        std::vector<Texture2D> textures;
        switch (scene)
        {

        case SC_ROOM:
        case SC_DARK:
            textures.push_back(LoadTexture("assets/earthmap.png"));
            textures.push_back(LoadTexture("assets/IndoorEnvironmentHDRI013_16K-TONEMAPPED.jpg"));
            break;

        case SC_CORNELL_BOX:
            break;

        default:
            break;
        }
        return textures;
    }

    std::shared_ptr<CameraControl> GetCamera() { return camera; }

private:

    int scene;
    int sky_tex_id;

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

        SetShaderValue(shader, GetShaderLocation(shader, "u_sky_tex_id"), &sky_tex_id, SHADER_UNIFORM_INT);
    }

    void ApplyObjectUniforms(Shader& shader) {
        int u_geometry_type[OBJECT_COUNT];
        int u_mat_id[OBJECT_COUNT];
        Vector3 u_a[OBJECT_COUNT];
        Vector3 u_b[OBJECT_COUNT];
        Vector3 u_c[OBJECT_COUNT];

        Vector3 u_n[OBJECT_COUNT];
        float u_D[OBJECT_COUNT];
        Vector3 u_w[OBJECT_COUNT];

        for (int i = 0; i < OBJECT_COUNT; i++) {
            u_geometry_type[i] = objects[i].geometry_type;
            u_mat_id[i] = objects[i].material_id;
            u_a[i] = objects[i].a;
            u_b[i] = objects[i].b;
            u_c[i] = objects[i].c;

            if (u_geometry_type[i] == PLANE || u_geometry_type[i] == QUAD) {
                
                Vector3 n = Vector3CrossProduct(u_b[i], u_c[i]);
                Vector3 normal = Vector3Normalize(n);
                float D = Vector3DotProduct(normal, u_a[i]);
                Vector3 w = Vector3Scale(n, 1.0/Vector3DotProduct(n,n));

                u_n[i] = normal;
                u_D[i] = D;
                u_w[i] = w;
            }           
        }

        SetShaderValueV(shader, GetShaderLocation(shader, "u_geometry_type"), u_geometry_type, SHADER_UNIFORM_INT, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_id"), u_mat_id, SHADER_UNIFORM_INT, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_a"), &u_a[0].x, SHADER_UNIFORM_VEC3, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_b"), &u_b[0].x, SHADER_UNIFORM_VEC3, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_c"), &u_c[0].x, SHADER_UNIFORM_VEC3, OBJECT_COUNT);
        
        SetShaderValueV(shader, GetShaderLocation(shader, "u_n"), &u_n[0].x, SHADER_UNIFORM_VEC3, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_D"), u_D, SHADER_UNIFORM_FLOAT, OBJECT_COUNT);
        SetShaderValueV(shader, GetShaderLocation(shader, "u_w"), &u_w[0].x, SHADER_UNIFORM_VEC3, OBJECT_COUNT);
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

        float v = GetMouseWheelMove();
        if (v != 0) {
            if (IsKeyDown(KEY_Z)) camera->defocus_angle += v*0.05;
            else camera->focus_dist += v*0.05;
            camera->RestartAccum();
        }        
    }

    // Scenes

    void Room() {
        camera->vfov     = 90;
        camera->Position = Vector3{3,3,3};
        camera->yaw   = M_PI*1.41;

        mats[0] = Mat(YELLOW, LAMBERTIAN);
        objects[0] = Hittable::Sphere(Vector3{0, 0, -3}, 1, 0);

        mats[1] = Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3);
        objects[1] = Hittable::Sphere(Vector3{0, 0, 0}, 1, 1);        

        mats[2] = Mat(RED, METAL, -1, -1, 0.0, 0.0);
        objects[2] = Hittable::Sphere(Vector3{0, 0, 3}, 1, 2);        

        sky_tex_id = 1;
    }

    void Dark() {
        camera->vfov     = 90;
        camera->Position = Vector3{3,3,3};
        camera->yaw   = M_PI*1.41;

        mats[0] = Mat(YELLOW, LAMBERTIAN);
        objects[0] = Hittable::Sphere(Vector3{0, 0, -3}, 1, 0);

        mats[1] = Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3);
        objects[1] = Hittable::Sphere(Vector3{0, 0, 0}, 1, 1);        

        mats[2] = Mat(RED, METAL);
        objects[2] = Hittable::Sphere(Vector3{0, 0, 3}, 1, 2);        

        mats[3] = Mat(WHITE, DIFFUSE_LIGHT);
        
        float Y = 3;
        float Z = 6;
        objects[3] = Hittable::Quad(Vector3{-4, -Y/2, -Z/2}, Vector3{0, Y, 0}, Vector3{0, 0, Z}, 3);

        sky_tex_id = SKY_DARK;
    }

    void CornellBox() {
        camera->updating = false;
        camera->vfov     = 40;
        camera->Position = Vector3{0.5, 0.5, -1.2};
    
        camera->defocus_angle = 0;

        Mat red   = Mat(Vector3{0.65, 0.05, 0.05}, LAMBERTIAN);
        Mat white = Mat(Vector3{0.73, 0.73, 0.73}, LAMBERTIAN);
        Mat green = Mat(Vector3{0.12, 0.45, 0.15}, LAMBERTIAN);
        Mat light = Mat(Vector3{15, 15, 15}, DIFFUSE_LIGHT);

        mats[0] = red;
        mats[1] = white;
        mats[2] = green;
        mats[3] = light;
        
        objects[0] = Hittable::Quad(Vector3{1,0,0}, Vector3{0,1,0}, Vector3{0,0,1}, 2);
        objects[1] = Hittable::Quad(Vector3{0,0,0}, Vector3{0,1,0}, Vector3{0,0,1}, 0);

        // light quad
        objects[2] = Hittable::Quad(Vector3{0.6, 1, 0.5}, Vector3{-0.2,0,0}, Vector3{0,0,-0.2}, 3);

        objects[3] = Hittable::Quad(Vector3{0,0,0}, Vector3{1,0,0}, Vector3{0,0,1}, 1);
        objects[4] = Hittable::Quad(Vector3{1,1,1}, Vector3{-1,0,0}, Vector3{0,0,-1}, 1);
        objects[5] = Hittable::Quad(Vector3{0,0,1}, Vector3{1,0,0}, Vector3{0,1,0}, 1);

        // tall box (left)
        Hittable::Box(objects, 6, Vector3{0.5, 0, 0.6}, Vector3{0.3, 0, 0}, Vector3{0, 0.6, 0}, Vector3{0, 0, 0.3}, 1);
        Hittable::RotateBox(objects, 6, Vector3{0, 1, 0}, 20);

        // short box (right)
        Hittable::Box(objects, 12, Vector3{0.25, 0, 0.2}, Vector3{0.3, 0, 0}, Vector3{0, 0.3, 0}, Vector3{0, 0, 0.3}, 1);
        Hittable::RotateBox(objects, 12, Vector3{0, 1, 0}, -20);
        sky_tex_id = SKY_DARK;
    }
};