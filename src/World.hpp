#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <memory>

#include "Material.h"
#include "Hittable.h"

#include "CameraControl.hpp"

// Scenes
#define SC_ROOM          0
#define SC_DARK          1
#define SC_CORNELL_BOX   2
#define SC_TRIANGLE_TEST 3

// IF YOU ADD MORE 2D primitives don't forget to change THE LINE ~166
// u_geometry_type[i] == PLANE || ...

// Procedural sky
#define SKY_DARK -2
#define SKY_BLUE -1

#define downscale 16.0

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
        case SC_TRIANGLE_TEST:
            TriangleTest();
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
        std::cout << "\nLoading Textures\n";
        std::vector<Texture2D> textures;
        switch (scene) {
        case SC_ROOM:
        case SC_DARK:
            textures.push_back(LoadTexture("assets/earthmap.png"));
            textures.push_back(LoadTexture("assets/IndoorEnvironmentHDRI013_16K-TONEMAPPED.jpg"));
            break;

        case SC_CORNELL_BOX:
            break;

        case SC_TRIANGLE_TEST:
            break;

        default:
            break;
        }
        return textures;
    }

    // std::vector<Model> LoadModelsForScene() {
    //     std::cout << "\nLoading models\n";
    //     std::vector<Model> models;
    //     switch (scene) {
    //         case SC_ROOM:
    //             break;
    //         case SC_DARK:
    //             break;
    //         case SC_CORNELL_BOX:
    //             break;
    //         case SC_TRIANGLE_TEST:
    //             models.push_back(LoadModel("assets/monkey.glb"));
    //             std::cout << "\ntriangles: " << models[0].meshes[0].triangleCount << '\n';
    //             break;

    //         default:
    //             break;
    //     }
    //     return models;
    // }

    std::shared_ptr<CameraControl> GetCamera() { return camera; }

private:

    int scene;
    int sky_tex_id;

    std::vector<Mat> mats;
    std::vector<Hittable> objects;
    std::shared_ptr<CameraControl> camera;
    
    float runTime = 0.f;
    bool only_normals = false;

    void ApplyMaterialUniforms(Shader& shader) {
        Vector3 u_albedo[mats.size()];
        int u_mat_type[mats.size()];
        int u_tex_id[mats.size()];
        int u_emit_tex_id[mats.size()];
        float u_fuzz[mats.size()];
        float u_refraction_index[mats.size()];

        for (int i = 0; i < mats.size(); i++) {
            u_albedo[i].x = mats[i].albedo.x;
            u_albedo[i].y = mats[i].albedo.y;
            u_albedo[i].z = mats[i].albedo.z;
            u_mat_type[i] = mats[i].mat_type;
            u_tex_id[i] = mats[i].tex_id;
            u_emit_tex_id[i] = mats[i].emit_tex_id;
            u_fuzz[i] = mats[i].fuzz;
            u_refraction_index[i] = mats[i].refraction_index;
        }
        
        SetShaderValueV(shader, GetShaderLocation(shader, "u_albedo"), &u_albedo[0].x, SHADER_UNIFORM_VEC3, mats.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_type"), u_mat_type, SHADER_UNIFORM_INT, mats.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_tex_id"), u_tex_id, SHADER_UNIFORM_INT, mats.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_emit_tex_id"), u_emit_tex_id, SHADER_UNIFORM_INT, mats.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_fuzz"), u_fuzz, SHADER_UNIFORM_FLOAT, mats.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_refraction_index"), u_refraction_index, SHADER_UNIFORM_FLOAT, mats.size());

        SetShaderValue(shader, GetShaderLocation(shader, "u_sky_tex_id"), &sky_tex_id, SHADER_UNIFORM_INT);
    }

    void ApplyObjectUniforms(Shader& shader) {
        int u_geometry_type[objects.size()];
        int u_mat_id[objects.size()];
        Vector3 u_a[objects.size()];
        Vector3 u_b[objects.size()];
        Vector3 u_c[objects.size()];

        Vector3 u_n[objects.size()];
        float u_D[objects.size()];
        Vector3 u_w[objects.size()];

        for (int i = 0; i < objects.size(); i++) {
            u_geometry_type[i] = objects[i].geometry_type;
            u_mat_id[i] = objects[i].material_id;
            u_a[i] = objects[i].a;
            u_b[i] = objects[i].b;
            u_c[i] = objects[i].c;

            if (u_geometry_type[i] == PLANE || u_geometry_type[i] == QUAD || u_geometry_type[i] == TRIANGLE) {
                
                Vector3 n = Vector3CrossProduct(u_b[i], u_c[i]);
                Vector3 normal = Vector3Normalize(n);
                float D = Vector3DotProduct(normal, u_a[i]);
                Vector3 w = Vector3Scale(n, 1.0/Vector3DotProduct(n,n));

                u_n[i] = normal;
                u_D[i] = D;
                u_w[i] = w;
            }           
        }

        SetShaderValueV(shader, GetShaderLocation(shader, "u_geometry_type"), u_geometry_type, SHADER_UNIFORM_INT, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_mat_id"), u_mat_id, SHADER_UNIFORM_INT, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_a"), &u_a[0].x, SHADER_UNIFORM_VEC3, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_b"), &u_b[0].x, SHADER_UNIFORM_VEC3, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_c"), &u_c[0].x, SHADER_UNIFORM_VEC3, objects.size());
        
        SetShaderValueV(shader, GetShaderLocation(shader, "u_n"), &u_n[0].x, SHADER_UNIFORM_VEC3, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_D"), u_D, SHADER_UNIFORM_FLOAT, objects.size());
        SetShaderValueV(shader, GetShaderLocation(shader, "u_w"), &u_w[0].x, SHADER_UNIFORM_VEC3, objects.size());

        int obj_count = objects.size();
        SetShaderValue(shader, GetShaderLocation(shader, "u_object_count"), &obj_count, SHADER_UNIFORM_INT);
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

        mats.push_back(Mat(YELLOW, LAMBERTIAN));
        mats.push_back(Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3));
        mats.push_back(Mat(RED, METAL, -1, -1, 0.0, 0.0));

        objects.push_back(Hittable::Sphere(Vector3{0, 0, -3}, 1, 0));
        objects.push_back(Hittable::Sphere(Vector3{0, 0, 0}, 1, 1));        
        objects.push_back(Hittable::Sphere(Vector3{0, 0, 3}, 1, 2));        

        sky_tex_id = 1;

        Model model = LoadModel("assets/monkey.glb");
        Hittable::Model(objects, model, 2, Vector3{0, 0, 6}, 1, Vector3{0, 1, 0}, -90);
    }

    void Dark() {
        camera->vfov     = 90;
        camera->Position = Vector3{3,3,3};
        camera->yaw   = M_PI*1.41;

        mats.push_back(Mat(YELLOW, LAMBERTIAN));
        mats.push_back(Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3));
        mats.push_back(Mat(RED, METAL));
        mats.push_back(Mat(WHITE, DIFFUSE_LIGHT));

        objects.push_back(Hittable::Sphere(Vector3{0, 0, -3}, 1, 0));
        objects.push_back(Hittable::Sphere(Vector3{0, 0, 0}, 1, 1));        
        objects.push_back(Hittable::Sphere(Vector3{0, 0, 3}, 1, 2));      
        
        float Y = 3;
        float Z = 6;
        objects.push_back(Hittable::Quad(Vector3{-4, -Y/2, -Z/2}, Vector3{0, Y, 0}, Vector3{0, 0, Z}, 3));

        sky_tex_id = SKY_DARK;
    }

    void CornellBox() {
        camera->updating = true;
        camera->vfov     = 40;
        camera->Position = Vector3{0.5, 0.5, -1.2};
    
        camera->defocus_angle = 0;

        Mat red   = Mat(Vector3{0.65, 0.05, 0.05}, LAMBERTIAN);
        Mat white = Mat(Vector3{0.73, 0.73, 0.73}, LAMBERTIAN);
        Mat green = Mat(Vector3{0.12, 0.45, 0.15}, LAMBERTIAN);
        Mat blue = Mat(BLUE, LAMBERTIAN);
        Mat yellow = Mat(YELLOW, LAMBERTIAN);

        Mat light = Mat(Vector3{2, 2, 2}, DIFFUSE_LIGHT);
        Mat glass = Mat(WHITE, DIELECTRIC, -1, -1, 0.0, 1/1.3);
        Mat metal = Mat(WHITE, METAL, -1, -1, 0.0, 0.0);

        mats.push_back(red); // 0
        mats.push_back(white); // 1
        mats.push_back(green); // 2
        mats.push_back(blue); // 3
        mats.push_back(yellow); // 4
        mats.push_back(light); // 5
        mats.push_back(glass); // 6
        mats.push_back(metal); // 7
        
        objects.push_back(Hittable::Quad(Vector3{1,0,0}, Vector3{0,1,0}, Vector3{0,0,1}, 2));
        objects.push_back(Hittable::Quad(Vector3{0,0,0}, Vector3{0,1,0}, Vector3{0,0,1}, 0));

        // light quad
        objects.push_back(Hittable::Quad(Vector3{0.8, 1, 0.7}, Vector3{-0.6,0,0}, Vector3{0,0,-0.6}, 5));

        objects.push_back(Hittable::Quad(Vector3{0,0,0}, Vector3{1,0,0}, Vector3{0,0,1}, 1));
        objects.push_back(Hittable::Quad(Vector3{1,1,1}, Vector3{-1,0,0}, Vector3{0,0,-1}, 1));
        objects.push_back(Hittable::Quad(Vector3{0,0,1}, Vector3{1,0,0}, Vector3{0,1,0}, 1));

        // tall box (left)
        Hittable::Box(objects, Vector3{0.5, 0, 0.6}, Vector3{0.3, 0, 0}, Vector3{0, 0.6, 0}, Vector3{0, 0, 0.3}, 3, Vector3{0, 1, 0}, 20);

        // short box (right)
        Hittable::Box(objects, Vector3{0.25, 0, 0.2}, Vector3{0.3, 0, 0}, Vector3{0, 0.3, 0}, Vector3{0, 0, 0.3}, 4, Vector3{0, 1, 0}, -20);
        sky_tex_id = SKY_DARK;

        objects.push_back(Hittable::Sphere(Vector3{0.35, 0.5, 0.4}, 0.12, 6));

        Model model = LoadModel("assets/monkey.glb");
        Hittable::Model(objects, model, 7, Vector3{0.68, 0.72, 0.5}, 0.13, Vector3{0, 1, 0}, 180+10);
    }

    void TriangleTest() {
        camera->updating = true;
        camera->vfov     = 90;
        camera->Position = Vector3{0.0, 0.0, 4};
        camera->yaw = M_PI;
        camera->defocus_angle = 0;

        Mat red   = Mat(Vector3{0.65, 0.05, 0.05}, LAMBERTIAN);
        Mat white = Mat(Vector3{0.73, 0.73, 0.73}, LAMBERTIAN);
        Mat green = Mat(Vector3{0.12, 0.45, 0.15}, LAMBERTIAN);
        Mat light = Mat(Vector3{2, 2, 2}, DIFFUSE_LIGHT);

        mats.push_back(red);
        mats.push_back(white);
        mats.push_back(green);
        mats.push_back(light);

        Model model = LoadModel("assets/monkey.glb");

        //objects[0] = Hittable::Triangle(Vector3{-1.0, -1.0, 3.0}, Vector3{0.0, 2.0, 0.0}, Vector3{2.0, 0.0, 0.0}, 3);
        Hittable::Model(objects, model, 3);

        sky_tex_id = SKY_DARK;
    }
};