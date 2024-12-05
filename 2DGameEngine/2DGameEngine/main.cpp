
#include <iostream>

#include "flecs/flecs.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "PerlinNoise.hpp"

#include <vector>
#include <string>

#include "CubeMeshData.h"

#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

static Mesh GenMeshCustom(Vector3 position);

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

    Color colours[9] = { MAGENTA, WHITE, LIGHTGRAY, YELLOW, BLUE, RED, GREEN, ORANGE, VIOLET };

    const int numChunks = 3;
    //Model genModel[(numChunks * 2) + 1][(numChunks * 2) + 1];
    TwoDimensionalStdVector(genModel, Model, (numChunks * 2) + 1);
    std::vector<Color> perChunkColours;

    const int chunkSize = 21;

    for (int x = -numChunks; x <= numChunks; x++)
    {
        for (int y = -numChunks; y <= numChunks; y++)
        {
            Vector3 curPos = { x * chunkSize, 0, y * chunkSize};
            genModel[x + numChunks][y + numChunks] = LoadModelFromMesh(GenMeshCustom(curPos));
            genModel[x + numChunks][y + numChunks].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
            perChunkColours.push_back(colours[GetRandomValue(0, 9)]);
        }

    }

    DisableCursor();

    Vector3 offsetPos = { 1.5f, 0.0f, 0.0f };


    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_FREE);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        DrawFPS(40, 40);

        std::string curNumMeshes = "Total Num Chunks -\n" + std::to_string(((numChunks * 2) + 1) * ((numChunks * 2) + 1));
        DrawText(curNumMeshes.c_str(), 300, 40, 30, MAGENTA);

            BeginMode3D(camera);
            //DrawModelWires(genModel, offsetPos, 1.0f, WHITE);
            for (int x = -numChunks; x <= numChunks; x++)
            {
                for (int y = -numChunks; y <= numChunks; y++)
                {
                    Vector3 curPos = { x * chunkSize, 0, y * chunkSize };
                    DrawModel(genModel[x + numChunks][y + numChunks], curPos, 1.0f, perChunkColours[((x + numChunks) * (numChunks)) + (y + numChunks)]);

                }

            }
            DrawGrid(10, 1.0);

            EndMode3D();
        EndDrawing();
    }

    UnloadTexture(texture);

    CloseWindow();
    return 0;
}

static Mesh GenMeshCustom(Vector3 position)
{
    Mesh mesh = { 0 };

    std::vector<float> vertices;
    std::vector<unsigned short> indices;
    std::vector<float> texCoords;
    std::vector<unsigned char> vertColour;

    const int size = 10;

    const siv::PerlinNoise::seed_type seed = 123456u;

    const siv::PerlinNoise perlin{ seed };

    for (int y = -size; y <= size; y++)
    {
        for (int x = -size; x <= size; x++)
        {
            for (int z = -size; z <= size; z++)
            {
                int _x = x + size + position.x;
                int _y = y + size + position.y;
                int _z = z + size + position.z;

                float curNoise = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                float emptyThreshold = 0.5f;
                bool curVoxelIsEmpty = curNoise < emptyThreshold ? true : false;

                if (!curVoxelIsEmpty) {

                    float curNoiseTop = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)(_y + 1) * 0.1f);

                    if (curNoiseTop < emptyThreshold || y == size) {
                        FaceIndicesTop(indices, vertices.size() / 3);
                        FaceVerticesTop(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                    float curNoiseBottom = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)(_y - 1) * 0.1f);

                    if (curNoiseBottom < emptyThreshold || y == -size) {
                        FaceIndicesBottom(indices, vertices.size() / 3);
                        FaceVerticesBottom(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                    float curNoiseFront = perlin.noise3D_01((double)_x * 0.1f, (double)(_z + 1) * 0.1f, (double)_y * 0.1f);

                    if (curNoiseFront < emptyThreshold || z == size) {

                        FaceIndicesFront(indices, vertices.size() / 3);
                        FaceVerticesFront(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                    float curNoiseBack = perlin.noise3D_01((double)_x * 0.1f, (double)(_z - 1) * 0.1f, (double)_y * 0.1f);

                    if (curNoiseBack < emptyThreshold || z == -size) {

                        FaceIndicesBack(indices, vertices.size() / 3);
                        FaceVerticesBack(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                    float curNoiseRight = perlin.noise3D_01((double)(_x + 1) * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                    if (curNoiseRight < emptyThreshold || x == size) {
                        FaceIndicesRight(indices, vertices.size() / 3);
                        FaceVerticesRight(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                    float curNoiseLeft = perlin.noise3D_01((double)(_x - 1) * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                    if (curNoiseLeft < emptyThreshold || x == -size) {
                        FaceIndicesLeft(indices, vertices.size() / 3);
                        FaceVerticesLeft(vertices, x, y, z);
                        TexCoords(texCoords);
                        VertColour(vertColour);
                    }

                }

            }
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

