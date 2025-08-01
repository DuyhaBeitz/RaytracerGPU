#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include "CameraControl.hpp"
#include "vec3.h"
#include <memory>

std::shared_ptr<CameraControl> camera;

int main() {
    InitWindow(500, 500, "RTX GPU");
    ToggleBorderlessWindowed();
    DisableCursor();

    Shader shader = LoadShader(0, "assets/RTX_GPU.frag");
    camera = std::make_shared<CameraControl>(3840, 2160);

    Texture2D shapes_texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(shapes_texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

    float runTime = 0.f;

    //SetTargetFPS(1);

    while (!WindowShouldClose()) {

        camera->UpdatePosition();
        camera->UpdateForward();

        if (IsKeyPressed(KEY_F11)) {
            ToggleBorderlessWindowed();
        }
    
        // Set uniforms
        SetShaderValue(shader, GetShaderLocation(shader, "iTime"), &runTime, SHADER_UNIFORM_FLOAT);
        float Resolution[2] = {camera->GetRenderWith(), camera->GetRenderHeight()};
        SetShaderValue(shader, GetShaderLocation(shader, "iResolution"), &Resolution, SHADER_UNIFORM_VEC2);
        camera->ApplyUniforms(shader);
        camera->DrawToBuffer(shader);

        BeginDrawing();
            camera->DrawFromBuffer();
            DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 64, DARKGRAY); // Display FPS
        EndDrawing();

        runTime += GetFrameTime();
    }

    CloseWindow();
}
