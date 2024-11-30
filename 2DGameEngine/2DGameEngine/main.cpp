
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "entt/entt.hpp"
#include "flecs/flecs.h"

#include "Shader.h"
#include "Components/Mesh.h"

#define window_width 800
#define window_height 600

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

void InitializeMesh(entt::entity& curEntity, entt::registry& curRegistery, float* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices) {

    auto curMesh = curRegistery.try_get<Mesh>(curEntity);

    curMesh->vertices.assign(vertices, vertices + numVertices);
    curMesh->indices.assign(indices, indices + numIndices);

    glGenVertexArrays(1, &curMesh->VAO);
    glGenBuffers(1, &curMesh->VBO);
    glGenBuffers(1, &curMesh->EBO);

    glBindVertexArray(curMesh->VAO);
    // 2. copy our vertices array in a buffer for OpenGL to use
    glBindBuffer(GL_ARRAY_BUFFER, curMesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, curMesh->vertices.size() * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curMesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, curMesh->indices.size() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    // 3. then set our vertex attributes pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void InitializeMesh(Mesh &curMesh, float* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices) {

    curMesh.vertices.assign(vertices, vertices + numVertices);
    curMesh.indices.assign(indices, indices + numIndices);

    glGenVertexArrays(1, &curMesh.VAO);
    glGenBuffers(1, &curMesh.VBO);
    glGenBuffers(1, &curMesh.EBO);

    glBindVertexArray(curMesh.VAO);
    // 2. copy our vertices array in a buffer for OpenGL to use
    glBindBuffer(GL_ARRAY_BUFFER, curMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, curMesh.vertices.size() * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, curMesh.indices.size() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    // 3. then set our vertex attributes pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "2D Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, window_width, window_height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    flecs::world world;
    entt::registry registry;

    auto entity = registry.create();
    registry.emplace<Mesh>(entity);

    auto e = world.entity();
    auto e_child1 = world.entity().child_of(e);
    auto e_child2 = world.entity().child_of(e);
    auto e_child3 = world.entity().child_of(e);
    auto e_child4 = world.entity().child_of(e);
    e.add<Mesh>();

    e.children([](flecs::entity children) {std::cout << children.id() << std::endl; });

    Shader shaderProgram("Shaders/SimpleShader.vert", "Shaders/SimpleShader.frag");

    float vertices[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left 
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    InitializeMesh(entity, registry, vertices, 12, indices, 6);

    auto q = world.query_builder<Mesh>();
    q.each([&](Mesh& mesh) {
        InitializeMesh(mesh, vertices, 12, indices, 6);
        });

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram.ID);

        auto &curMesh = registry.get<Mesh>(entity);

        glBindVertexArray(e.get<Mesh>()->VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}