#pragma once

#include "vec3.h"
#include <raylib.h>
#include <raymath.h>

struct CameraControl {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.005;
    float speed = 3.0;

    vec3 Position = vec3(0.0, 0.0, -3.0);
    vec3 vup = vec3(0.0, 1.0, 0.0);

    // counts frames. When camera is modified gets resets, so the accumulation starts again
    float accumFrameCount = 0.f;
    // Accumulation weight
    float accumAlpha = 1.0;

    RenderTexture2D buffer;
    RenderTexture2D tempbuffer;

    CameraControl(int render_width, int render_height) {
        buffer = LoadRenderTexture(render_width, render_height);
    }

    void NewResolution(int render_width, int render_height) {
        RestartAccum();
        UnloadRenderTexture(buffer);
        buffer = LoadRenderTexture(render_width, render_height);
    }

    void DrawToBuffer(Shader& shader) {
        BeginTextureMode(buffer);
            BeginShaderMode(shader);
                DrawRectangle(0, 0, GetRenderWith(), GetRenderHeight(), Fade(WHITE, 1.0));
            EndShaderMode();
        EndTextureMode();
    }

    void DrawFromBuffer() {
        accumAlpha = 1.0f / (accumFrameCount + 1); // Accumulation weight
        BeginBlendMode(BLEND_ALPHA);
        DrawTexturePro(
            buffer.texture, 
            Rectangle{0, 0, float(GetRenderWith()), -float(GetRenderHeight())},
            Rectangle{0, 0, float(GetScreenWidth()), float(GetScreenHeight())},
            Vector2{0.0},
            0.f,
            Fade(WHITE, accumAlpha)
        );
        EndBlendMode();
        accumFrameCount++;
    }

    void RestartAccum() {
        accumFrameCount = 0;
    }

    int GetRenderWith() {
        return buffer.texture.width;
    }

    int GetRenderHeight() {
        return buffer.texture.height;
    }

    /***************************************************************************************/

    vec3 GetForwardVector() {
        return vec3(cosf(pitch) * sinf(yaw), sinf(pitch), cosf(pitch) * cosf(yaw));
    }

    vec3 GetRightVector() {
        return -unit_vector(cross(vup, GetForwardVector()));
    }

    void UpdatePosition() {
        int df = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
        int dr = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
        int du = IsKeyDown(KEY_SPACE) - IsKeyDown(KEY_LEFT_CONTROL);
    
        vec3 Velocity = vec3(0.0);
        Velocity += GetForwardVector() * float(df) * GetFrameTime();
        Velocity += GetRightVector() * float(dr) * GetFrameTime();
        Velocity += vup * float(du) * GetFrameTime();

        Position += Velocity*speed;
        if (Velocity != vec3(0.0)) RestartAccum();
    }

    void UpdateForward() {
        Vector2 mouseDelta = GetMouseDelta();
        yaw   -= mouseDelta.x * mouseSensitivity;
        pitch -= mouseDelta.y * mouseSensitivity;
        pitch = Clamp(pitch, -M_PI/2*0.9, M_PI/2*0.9);

        if (mouseDelta.x != 0.0 || mouseDelta.y != 0.0) RestartAccum();
    }

    void ApplyUniforms(Shader& shader) {
        float Lookfrom[3] = {Position.x(), Position.y(), Position.z()};
        SetShaderValue(shader, GetShaderLocation(shader, "lookfrom"), &Lookfrom, SHADER_UNIFORM_VEC3);

        vec3 At = Position + GetForwardVector();
        float Lookat[3] = {At.x(), At.y(), At.z()};
        SetShaderValue(shader, GetShaderLocation(shader, "lookat"), &Lookat, SHADER_UNIFORM_VEC3);
    }
};