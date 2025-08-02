#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <memory>
#include <time.h>
#include <stdio.h>
#include <iostream>

#include "CameraControl.hpp"
#include "World.hpp"

Shader shader;

std::shared_ptr<CameraControl> camera;
std::shared_ptr<World> world;

bool only_normals = false;
bool show_info = false;


float runTime = 0.f;

void TakeTimestampedScreenshot() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[64];
    strftime(filename, sizeof(filename), "screenshot-%Y-%m-%d_%H-%M-%S.png", t);
    TakeScreenshot(filename);
    std::cout << "Saved screenshot: " << filename << '\n';
}

bool Init();
void HandleInput();
void ApplyUniforms();
void DrawScreen();

int main() {
    if (!Init()) {
        std::cerr << "Failed to initialize\n";
        return 1;
    }

    while (!WindowShouldClose()) {
        camera->UpdatePosition();
        camera->UpdateForward();

        HandleInput();
    
        ApplyUniforms();

        DrawScreen();
        runTime += GetFrameTime();
    }
    CloseWindow();
    return 0;
}

bool Init() {
    InitWindow(0, 0, "RTX GPU");
    if (!IsWindowReady()) {
        return false;
    }
    ToggleBorderlessWindowed();
    DisableCursor();

    /*
    std::string fragCode = LoadShaderRecursive("assets/RTX_GPU.f");
    WriteToFile(fragCode, "assets/final.frag");
    shader = LoadShaderFromMemory(nullptr, fragCode.c_str());
    */
    shader = LoadShader(0, "assets/RTX_GPU.frag");
    camera = std::make_shared<CameraControl>(3840/4, 2160/4);

    world = std::make_shared<World>();
    world->ApplyUniforms(shader);

    Texture2D shapes_texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(shapes_texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

    return true;
}

void HandleInput()
{
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
}

void ApplyUniforms() {
    SetShaderValue(shader, GetShaderLocation(shader, "iTime"), &runTime, SHADER_UNIFORM_FLOAT);
    float Resolution[2] = {
        float(camera->GetRenderWith()),
        float(camera->GetRenderHeight())
    };
    SetShaderValue(shader, GetShaderLocation(shader, "iResolution"), &Resolution, SHADER_UNIFORM_VEC2);
    int ionly_normals = only_normals ? 1 : 0;
    SetShaderValue(shader, GetShaderLocation(shader, "ionly_normals"), &ionly_normals, SHADER_UNIFORM_INT);
    camera->ApplyUniforms(shader);
    camera->DrawToBuffer(shader);
}

void DrawScreen() {
    BeginDrawing();
    camera->DrawFromBuffer();
    if (show_info) {
        DrawRectangle(0, 0, 800, 200, GRAY);
        DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 64, WHITE);
        DrawText(TextFormat("Resolution: %i, %i", camera->GetRenderWith(), camera->GetRenderHeight()), 10, 84, 64, WHITE);
    }
    EndDrawing();
}
