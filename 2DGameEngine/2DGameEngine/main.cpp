
#include <iostream>

#include "flecs/flecs.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "PerlinNoise.hpp"

#include <vector>

#include "CubeMeshData.h"

static Mesh GenMeshCustom(void);    // Generate a simple triangle mesh from code

int main()
{

    flecs::world world;

    auto e = world.entity();

    InitWindow(800, 450, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    // Model drawing position
    Vector3 position = { 0.0f, 0.0f, 0.0f };

    // We generate a checked image for texturing
    Image checked = GenImageChecked(2, 2, 1, 1, RED, GREEN);
    Texture2D texture = LoadTextureFromImage(checked);
    UnloadImage(checked);

    Model genModel = LoadModelFromMesh(GenMeshCustom());

    genModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
            BeginMode3D(camera);

            //DrawModelWires(genModel, position, 1.0f, WHITE);
            DrawModel(genModel, position, 1.0f, WHITE);
            DrawGrid(10, 1.0);

            EndMode3D();
        EndDrawing();
    }

    UnloadTexture(texture);

    CloseWindow();
    return 0;
}

static Mesh GenMeshCustom(void)
{
    Mesh mesh = { 0 };

    std::vector<float> vertices;
    std::vector<unsigned short> indices;
    std::vector<float> texCoords;
    std::vector<unsigned char> vertColour;

    float noise[11][11];

    const siv::PerlinNoise::seed_type seed = 123456u;

    const siv::PerlinNoise perlin{ seed };

    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 11; j++)
        {
            noise[i][j] = perlin.noise2D_01((double)i, (double)j);
        }
    }

    int size = 5;
    for (int x = -size; x <= size; x++)
    {
        for (int z = -size; z <= size; z++)
        {
            FaceIndicesTop(indices, vertices.size() / 3);
            FaceVerticesTop(vertices, x, round(noise[x + size][z + size] * size), z);
            TexCoords(texCoords);
            VertColour(vertColour);
            
            FaceIndicesBottom(indices, vertices.size() / 3);
            FaceVerticesBottom(vertices, x, round(noise[x + size][z + size] * size), z);
            TexCoords(texCoords);
            VertColour(vertColour);

            FaceIndicesFront(indices, vertices.size() / 3);
            FaceVerticesFront(vertices, x, round(noise[x + size][z + size] * size), z);
            TexCoords(texCoords);
            VertColour(vertColour);

            FaceIndicesBack(indices, vertices.size() / 3);
            FaceVerticesBack(vertices, x, round(noise[x + size][z + size] * size), z);
            TexCoords(texCoords);
            VertColour(vertColour);

            FaceIndicesRight(indices, vertices.size() / 3);
            FaceVerticesRight(vertices, x, round(noise[x + size][z + size] * size), z);
            TexCoords(texCoords);
            VertColour(vertColour);

            FaceIndicesLeft(indices, vertices.size() / 3);
            FaceVerticesLeft(vertices, x, round(noise[x + size][z + size]* size), z);
            TexCoords(texCoords);
            VertColour(vertColour);
        }
    }

    mesh.triangleCount = indices.size() / 3;
    mesh.vertexCount = vertices.size() / 3;
    mesh.vertices = (float*)MemAlloc(vertices.size() * sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
    mesh.texcoords = (float*)MemAlloc(texCoords.size() * sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
    //mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)
    //mesh.colors = (unsigned char*)MemAlloc(vertColour.size() * sizeof(unsigned char));
    
    mesh.indices = (unsigned short*)MemAlloc(indices.size() * sizeof(unsigned short*));

    mesh.vertices = vertices.data();
    mesh.indices = indices.data();
    mesh.texcoords = texCoords.data();
    //mesh.colors = vertColour.data();

    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    UploadMesh(&mesh, false);

    return mesh;
}

