#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <glad/glad.h>
#include <iostream>

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static GLuint createTextureFromSurface(SDL_Surface *surface)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

static GLuint createTexture(const char *path)
{
    // Use SDL_IOFromFile to correctly read the file from assets on Android and from disk on PC
    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io)
    {
        SDL_Log("Failed to open file stream for %s: %s", path, SDL_GetError());
        return 0;
    }

    // Load the image from the SDL stream
    SDL_Surface *surface = IMG_Load_IO(io, 1); // 1 (true) will automatically close and free the SDL_IOStream
    if (!surface)
    {
        SDL_Log("Image loading failed: %s", SDL_GetError());
        return 0;
    }

    // Convert to RGBA / ABGR8888 for OpenGL
    SDL_Surface *optimizedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surface);

    if (!optimizedSurface)
    {
        SDL_Log("Surface conversion failed: %s", SDL_GetError());
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic"))
    {
        float maxAnisotropy = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

        // Set the quality to maximum (usually 8.0 or 16.0)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);

        printf("Anisotropic filtering enabled. Max: %.1f\n", maxAnisotropy);
    }
    else
    {
        SDL_Log("Anisotropic filtering not supported.");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, optimizedSurface->w, optimizedSurface->h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, optimizedSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_DestroySurface(optimizedSurface);

    return textureID;
}

static const char *vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char *fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
)";

struct AppState
{
    SDL_Window *window = nullptr;
    SDL_GLContext glContext = nullptr;
    GLuint texture = 0;
    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "60"))
    {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL Initialization failed: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    AppState *state = new AppState();
    *appstate = state;

    state->window = SDL_CreateWindow("SDL3_image + WebP (OpenGL 3.3) - Callbacks", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!state->window)
    {
        return SDL_APP_FAILURE;
    }

    state->glContext = SDL_GL_CreateContext(state->window);
    if (!state->glContext)
    {
        return SDL_APP_FAILURE;
    }

    SDL_GL_MakeCurrent(state->window, state->glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return SDL_APP_FAILURE;
    }

    // Shaders & Geometry
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    state->shaderProgram = glCreateProgram();
    glAttachShader(state->shaderProgram, vertexShader);
    glAttachShader(state->shaderProgram, fragmentShader);
    glLinkProgram(state->shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        // Positions   // TexCoords
        -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 1.0f, 1.0f,

        -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &state->VAO);
    glGenBuffers(1, &state->VBO);

    glBindVertexArray(state->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, state->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Load WebP Texture
    state->texture = createTexture("assets/textures/clear_01.webp");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    auto *state = static_cast<AppState *>(appstate);

    int w, h;
    SDL_GetWindowSizeInPixels(state->window, &w, &h);
    glViewport(0, 0, w, h);

    glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state->texture)
    {
        glUseProgram(state->shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, state->texture);
        glBindVertexArray(state->VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    SDL_GL_SwapWindow(state->window);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (appstate)
    {
        auto *state = static_cast<AppState *>(appstate);

        if (state->texture)
        {
            glDeleteTextures(1, &state->texture);
        }
        if (state->VAO)
        {
            glDeleteVertexArrays(1, &state->VAO);
        }
        if (state->VBO)
        {
            glDeleteBuffers(1, &state->VBO);
        }
        if (state->shaderProgram)
        {
            glDeleteProgram(state->shaderProgram);
        }
        if (state->glContext)
        {
            SDL_GL_DestroyContext(state->glContext);
        }
        if (state->window)
        {
            SDL_DestroyWindow(state->window);
        }
        delete state;
    }
    SDL_Quit();
}
