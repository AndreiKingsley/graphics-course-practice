#ifdef WIN32
#include <SDL.h>
#undef main
#else

#include <SDL2/SDL.h>

#endif

#include <GL/glew.h>

#include "shader_program.h"
#include <string_view>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
#include "shader.h"
#include "mesh.h"
#include "model.h"


#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>

#include "common.h"
#include "scene_object.h"
#include "scene.h"

std::string to_string(std::string_view str) {
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message) {
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char vertex_shader_source[] = R"(
#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoords;
out vec2 tex_coords;
out vec3 position;
out vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec4 pos;

void main()
{
    tex_coords = in_texcoords;
    pos = vec4(in_position, 1.0);
	position = (model * pos).xyz;
    normal = in_normal;
    gl_Position = projection * view * model * pos;
}
)";

const char fragment_shader_source[] = R"(
#version 330 core

uniform vec3 ambient;
uniform vec3 light_position[3];
uniform vec3 light_color[3];
uniform vec3 light_attenuation[3];
uniform mat4 transform;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D shadow_map;
uniform int has_norm;

in vec3 position;
in vec2 tex_coords;
in vec3 normal;

out vec4 out_color;

vec3 real_normal;

vec3 count_light(vec3 position, vec3 light_position, vec3 light_color, vec3 light_attenuation) {
    vec3 light_vector = light_position - position;
    vec3 light_direction = normalize(light_vector);
    float cosine = dot(real_normal, light_direction);
    float light_factor = max(0.0, cosine);
    float light_distance = length(light_vector);
    float light_intensity = 1.0 / dot(light_attenuation, vec3(1.0, light_distance, light_distance * light_distance));
    return light_factor * light_intensity * light_color;
}
void main()
{
    if (has_norm == 0) {
        real_normal = normal;
    } else {
        real_normal = (vec4(2 * texture(texture_normal, tex_coords).rgb - 1.0, 0.0)).xyz;
    }

    vec4 shadow_pos = transform * vec4(position, 1.0);
	shadow_pos = shadow_pos / shadow_pos.w  * 0.5 + vec4(0.5);

    bool is_shadowed =
        (shadow_pos.x > 0.0)
        && (shadow_pos.x < 1.0)
        && (shadow_pos.y > 0.0)
        && (shadow_pos.y < 1.0)
        && (shadow_pos.z > 0.0)
        && (shadow_pos.z < 1.0);

	float shadow_factor = 1.0;
	if (is_shadowed && texture(shadow_map, shadow_pos.xy).x < shadow_pos.z - 0.001) {
        shadow_factor = 0.0;
    }
    vec3 result_color = ambient;

    result_color += count_light(position, light_position[0], light_color[0], light_attenuation[0]) * shadow_factor;

    for (int i = 1; i < 3; i++) {
        result_color += count_light(position, light_position[i], light_color[i], light_attenuation[i]);
    }
    result_color = texture(texture_diffuse, tex_coords).rgb * result_color;
    out_color = vec4(result_color, 1.0);
}
)";

const char shadow_vertex_shader_source[] =R"(#version 330 core
uniform mat4 model;
uniform mat4 transform;
layout (location = 0) in vec3 in_position;
void main()
{
	gl_Position = transform * model * vec4(in_position, 1.0);
}
)";

const char shadow_fragment_shader_source[] =R"(#version 330 core
void main()
{}
)";


int main() try {

    int width, height;
    SDL_Window *window;
    SDL_GLContext gl_context;
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            sdl2_fail("SDL_Init: ");

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        window = SDL_CreateWindow("Graphics course practice 7",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  800, 600,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

        if (!window)
            sdl2_fail("SDL_CreateWindow: ");

        SDL_GetWindowSize(window, &width, &height);

        gl_context = SDL_GL_CreateContext(window);
        if (!gl_context)
            sdl2_fail("SDL_GL_CreateContext: ");

        if (auto result = glewInit(); result != GLEW_NO_ERROR)
            glew_fail("glewInit: ", result);

        if (!GLEW_VERSION_3_3)
            throw std::runtime_error("OpenGL 3.3 is not supported");
    }

    glEnable(GL_DEPTH_TEST);


    shader_program main_program;
    main_program.create(vertex_shader_source, fragment_shader_source);

    std::string path = PRACTICE_SOURCE_DIRECTORY;

    scene main_scene;
    main_scene.load(PRACTICE_SOURCE_DIRECTORY, "sponza.obj");

    scene bunny_scene;
    bunny_scene.load(PRACTICE_SOURCE_DIRECTORY, "bunny.obj");

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    std::map<SDL_Keycode, bool> button_down;



    glm::vec3 camera_position = glm::vec3(0.0f, 1.f, 0.0f);
    glm::vec3 camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float y_angle = 0.f;
    float x_angle = 0.f;

    auto sun_position = glm::vec3(-15.f, 40.f, 5.f);

    glm::vec3 light_position[3] = {
            sun_position,
            glm::vec3(10.0f, 3.5f, -4.f),
            glm::vec3(-12.f, 5.f, 5.69f),
    };
    glm::vec3 light_color[3] = {
            glm::vec3(8.0f, 8.0f, 4.0f),
            glm::vec3(2.5f, 9.f, 3.0f),
            glm::vec3(10.f, 0.f, 0.0f),
    };
    glm::vec3 light_attenuation[3] = {
            glm::vec3(1.0f, 0.00001, 0.01f),
            glm::vec3(1.0f, 0, 0.1f),
            glm::vec3(1.0f, 0, 0.1f),
    };

    main_program.use();

    auto ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    main_program.set_vec3("ambient", ambient);
    for (int i = 0; i < 3; i++) {
        main_program.set_vec3(("light_position[" + std::to_string(i) + "]").c_str(), light_position[i]);
        main_program.set_vec3(("light_color[" + std::to_string(i) + "]").c_str(), light_color[i]);
        main_program.set_vec3(("light_attenuation[" + std::to_string(i) + "]").c_str(), light_attenuation[i]);
    }

    GLsizei shadow_map_resolution = 4500;

    GLuint shadow_map;
    glGenTextures(1, &shadow_map);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_map_resolution, shadow_map_resolution, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    GLuint shadow_fbo;
    glGenFramebuffers(1, &shadow_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_map, 0);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer!");
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    shader_program shadow_program;
    shadow_program.create(shadow_vertex_shader_source, shadow_fragment_shader_source);

    for (auto &obj: main_scene.objects) {
        obj.textures.push_back({shadow_map, "shadow_map"});
    }

    bool running = true;
    while (running) {

        {
            for (SDL_Event event; SDL_PollEvent(&event);)
                switch (event.type) {
                    case SDL_QUIT:
                        running = false;
                        break;
                    case SDL_WINDOWEVENT:
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_RESIZED:
                                width = event.window.data1;
                                height = event.window.data2;
                                glViewport(0, 0, width, height);
                                break;
                        }
                        break;
                    case SDL_KEYDOWN:
                        button_down[event.key.keysym.sym] = true;
                        break;
                    case SDL_KEYUP:
                        button_down[event.key.keysym.sym] = false;
                        break;
                }
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float >>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;


        glm::vec3 direction;
        direction.x = cos(glm::radians(y_angle)) * cos(glm::radians(x_angle));
        direction.y = sin(glm::radians(x_angle));
        direction.z = sin(glm::radians(y_angle)) * cos(glm::radians(x_angle));
        camera_direction = glm::normalize(direction);

        {
            float rotation_speed = 100.f;
            if (button_down[SDLK_UP])
                x_angle += rotation_speed * dt;
            if (button_down[SDLK_DOWN])
                x_angle -= rotation_speed * dt;

            if (button_down[SDLK_LEFT])
                y_angle -= rotation_speed * dt;
            if (button_down[SDLK_RIGHT])
                y_angle += rotation_speed * dt;

            float speed = 0.1f;
            if (button_down[SDLK_w])
                camera_position += speed * camera_direction;

            if (button_down[SDLK_s])
                camera_position -= speed * camera_direction;

            if (button_down[SDLK_d])
                camera_position += glm::normalize(glm::cross(camera_direction, up)) * speed;

            if (button_down[SDLK_a])
                camera_position -= glm::normalize(glm::cross(camera_direction, up)) * speed;
        }

        glClearColor(0.8f, 0.8f, 0.9f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 light_direction = glm::normalize(sun_position);

        float near = 0.1f;
        float far = 100.f;

        glm::mat4 model = glm::mat4(1.f);
        glm::vec3 scale = glm::vec3(0.01f);
        model = glm::scale(model, scale);


        glm::mat4 view(1.f);

        view = glm::lookAt(camera_position, camera_position + camera_direction, up);

        glm::mat4 projection = glm::mat4(1.f);
        projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * width) / height, near, far);

        glm::vec3 light_z = -light_direction;
        glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 1.f, 0.f}));
        glm::vec3 light_y = glm::cross(light_x, light_z);
        float shadow_scale = 0.03f;

        glm::mat4 transform = glm::mat4(1.f);
        for (size_t i = 0; i < 3; ++i) {
            transform[i][0] = shadow_scale * light_x[i];
            transform[i][1] = shadow_scale * light_y[i];
            transform[i][2] = shadow_scale * light_z[i];
        }


        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, shadow_map_resolution, shadow_map_resolution);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shadow_program.use();

        shadow_program.set_matrix("model", model);
        shadow_program.set_matrix("transform", transform);

        for (auto obj: main_scene.objects) {
            obj.draw(shadow_program.id);
        }

        glBindTexture(GL_TEXTURE_2D, shadow_map);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);

        glClearColor(0.8f, 0.8f, 0.9f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        main_program.use();
        main_program.set_matrix("projection", projection);
        main_program.set_matrix("view", view);
        main_program.set_matrix("model", model);
        main_program.set_matrix("transform", transform);

        for (auto obj: main_scene.objects) {
            main_program.set_int("has_norm", obj.has_normal_tex);
            obj.draw(main_program.id);
        }

        model = glm::mat4(4.f);
        model = glm::translate(model, glm::vec3(time, 0.f, 0.f));
        main_program.set_matrix("model", model);


        for (auto obj: bunny_scene.objects) {
            main_program.set_int("has_norm", obj.has_normal_tex);
            obj.draw(main_program.id);
        }

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
