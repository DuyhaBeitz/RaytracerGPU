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

std::shared_ptr<World> world;
std::vector<Texture2D> textures;
//std::vector<Model> models;

bool show_info = false;

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
void DrawScreen();
void UnloadTextures();

int main() {
    if (!Init()) {
        std::cerr << "Failed to initialize\n";
        return 1;
    }

    while (!WindowShouldClose()) {
        world->Update();

        HandleInput();
    
        world->ApplyUniforms(shader);
        world->GetCamera()->DrawToBuffer(shader, textures);

        DrawScreen();
    }
    UnloadTextures();
    CloseWindow();
    return 0;
}

bool Init() {
    InitWindow(500, 500, "RTX GPU");
    if (!IsWindowReady()) {
        return false;
    }
    ToggleBorderlessWindowed();
    DisableCursor();


    shader = LoadShader(0, "assets/RTX_GPU.frag");

    world = std::make_shared<World>(SC_CORNELL_BOX);
    textures = world->LoadTexturesForScene();
    //models = world->LoadModelsForScene();

    Texture2D shapes_texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(shapes_texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

    return true;
}

void UnloadTextures() {
    for (auto texture : textures) {
        UnloadTexture(texture);
    }
}

void HandleInput()
{
    if (IsKeyPressed(KEY_L)) {
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }
    if (IsKeyPressed(KEY_F11)) {
        ToggleBorderlessWindowed();
    }
    if (IsKeyDown(KEY_ENTER)) {
        TakeTimestampedScreenshot();
    }
    if (IsKeyPressed(KEY_I)) {
        show_info = !show_info;
        world->GetCamera()->RestartAccum();
    }
}

void DrawScreen() {
    BeginDrawing();
    world->GetCamera()->DrawFromBuffer();
    if (show_info) {
        
        int dv = 64;
        DrawRectangle(0, 0, 800, dv*5, GRAY);
        
        DrawText(TextFormat("FPS: %i", GetFPS()), 10, 0, 64, WHITE);
        DrawText(TextFormat("Resolution: %i, %i", world->GetCamera()->GetRenderWith(), world->GetCamera()->GetRenderHeight()), 10, dv, 64, WHITE);
        DrawText(TextFormat("vfov: %f", world->GetCamera()->vfov), 10, 2*dv, 64, WHITE);
        DrawText(TextFormat("focus_dist: %f", world->GetCamera()->focus_dist), 10, 3*dv, 64, WHITE);
        DrawText(TextFormat("defocus_angle: %f", world->GetCamera()->defocus_angle), 10, 4*dv, 64, WHITE);
    }
    EndDrawing();
}
