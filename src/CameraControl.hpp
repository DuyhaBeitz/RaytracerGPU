#pragma once

#include <raylib.h>
#include <raymath.h>
#include <vector>

struct CameraControl {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.005;
    float speed = 13.0;
    float vfov = 90.0;
    float defocus_angle = 0.0;
    float focus_dist = 0.5;

    bool updating = false;

    Vector3 Position = Vector3{0.0, 1.0, -3.0};
    Vector3 vup = Vector3{0.0, 1.0, 0.0};

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

    void DrawToBuffer(Shader& shader, std::vector<Texture2D>& textures) {
        BeginTextureMode(buffer);
            BeginShaderMode(shader);
                ApplyTextureUniforms(shader, textures);
                DrawRectangle(0, 0, GetRenderWith(), GetRenderHeight(), Fade(WHITE, 1.0));
            EndShaderMode();
        EndTextureMode();
    }

    void ApplyTextureUniforms(Shader& shader, std::vector<Texture2D>& textures) {
        SetShaderValueTexture(shader, GetShaderLocation(shader, "tex0"), textures[0]);
    }

    void DrawFromBuffer() {
        if (GetTime() > 0.2) {
            accumAlpha = 1.0f / (accumFrameCount + 1); // Accumulation weight
            BeginBlendMode(BLEND_ALPHA);
            DrawTexturePro(
                buffer.texture, 
                Rectangle{0, 0, float(GetRenderWith()), -float(GetRenderHeight())},
                Rectangle{0, 0, float(GetScreenWidth()), float(GetScreenHeight())},
                Vector2{0.0},
                0.f,
                Fade(WHITE, accumAlpha)
                //Fade(WHITE, 0.01)
            );
            EndBlendMode();
            accumFrameCount++;
        }
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

    Vector3 GetForwardVector() {
        return Vector3{cosf(pitch) * sinf(yaw), sinf(pitch), cosf(pitch) * cosf(yaw)};
    }

    Vector3 GetRightVector() {
        return Vector3Negate(Vector3Normalize(Vector3CrossProduct(vup, GetForwardVector())));
    }

    void UpdatePosition() {
        if (updating) {
            int df = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
            int dr = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
            int du = IsKeyDown(KEY_SPACE) - IsKeyDown(KEY_LEFT_CONTROL);
        
            Vector3 Velocity = Vector3{0.0};
            Velocity += GetForwardVector() * float(df) * GetFrameTime();
            Velocity += GetRightVector() * float(dr) * GetFrameTime();
            Velocity += vup * float(du) * GetFrameTime();
    
            Position += Velocity*speed;
            if (Velocity != Vector3{0.0}) RestartAccum();
    
            // otherwise shader crashes
            Position.x = Clamp(Position.x, -100, 100);
            Position.y = Clamp(Position.y, -100, 100);
            Position.z = Clamp(Position.z, -100, 100);
        }
    }

    void UpdateForward() {
        if (updating) {
            if (GetTime() > 0.2) {
                Vector2 mouseDelta = GetMouseDelta();
                yaw   -= mouseDelta.x * mouseSensitivity;
                pitch -= mouseDelta.y * mouseSensitivity;
                pitch = Clamp(pitch, -M_PI/2*0.9, M_PI/2*0.9);
        
                if (mouseDelta.x != 0.0 || mouseDelta.y != 0.0) RestartAccum();
            }
        }
    }

    void ApplyUniforms(Shader& shader) {
        float Lookfrom[3] = {
            float(Position.x),
            float(Position.y),
            float(Position.z)
        };
        SetShaderValue(shader, GetShaderLocation(shader, "lookfrom"), &Lookfrom, SHADER_UNIFORM_VEC3);

        Vector3 At = Position + GetForwardVector();
        float Lookat[3] = {
            float(At.x),
            float(At.y),
            float(At.z)
        };
        SetShaderValue(shader, GetShaderLocation(shader, "lookat"), &Lookat, SHADER_UNIFORM_VEC3);

        SetShaderValue(shader, GetShaderLocation(shader, "vfov"), &vfov, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "defocus_angle"), &defocus_angle, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "focus_dist"), &focus_dist, SHADER_UNIFORM_FLOAT);
    }
};