#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include "CameraControl.hpp"
#include "vec3.h"
#include <memory>
#include <time.h>
#include <stdio.h>

std::shared_ptr<CameraControl> camera;

bool only_normals = false;
bool show_info = false;

void TakeTimestampedScreenshot() {
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Format time string
    char filename[64];
    strftime(filename, sizeof(filename), "screenshot-%Y-%m-%d_%H-%M-%S.png", t);

    // Save screenshot
    TakeScreenshot(filename);

    // Optional: print filename
    std::cout << "Saved screenshot: " << filename << '\n';
}

int main() {

    InitWindow(500, 500, "RTX GPU");
    ToggleBorderlessWindowed();
    DisableCursor();

    Shader shader = LoadShader(0, "assets/RTX_GPU.frag");
    camera = std::make_shared<CameraControl>(3840/16, 2160/16);

    Texture2D shapes_texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(shapes_texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

    float runTime = 0.f;

    while (!WindowShouldClose()) {

        camera->UpdatePosition();
        camera->UpdateForward();

        if (IsKeyPressed(KEY_F11)) {
            ToggleBorderlessWindowed();
        }
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
        if (IsKeyDown(KEY_ENTER)) {
            TakeTimestampedScreenshot();
        }
        if (IsKeyPressed(KEY_I)) {
            show_info = !show_info;
            camera->RestartAccum();
        }
    
        // Set uniforms
        SetShaderValue(shader, GetShaderLocation(shader, "iTime"), &runTime, SHADER_UNIFORM_FLOAT);
        float Resolution[2] = {camera->GetRenderWith(), camera->GetRenderHeight()};
        SetShaderValue(shader, GetShaderLocation(shader, "iResolution"), &Resolution, SHADER_UNIFORM_VEC2);
        int ionly_normals = only_normals ? 1 : 0;
        SetShaderValue(shader, GetShaderLocation(shader, "ionly_normals"), &ionly_normals, SHADER_UNIFORM_INT);
        camera->ApplyUniforms(shader);
        camera->DrawToBuffer(shader);

        BeginDrawing();
            camera->DrawFromBuffer();
            if (show_info) {
                DrawRectangle(0, 0, 800, 200, GRAY);
                DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 64, WHITE); // Display FPS
                DrawText(TextFormat("Resolution: %i, %i", camera->GetRenderWith(), camera->GetRenderHeight()), 10, 84, 64, WHITE); // Display current resolution
            }
        EndDrawing();
        runTime += GetFrameTime();
    }

    CloseWindow();
}
