
#include <iostream>
#include <chrono>

#include "flecs/flecs.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "Plane.h"

#define GLSL_VERSION            330

#include "PerlinNoise.hpp"

#include <vector>
#include <string>
#include <map>

#include "CubeMeshData.h"
#include "DrawMeshInstancedFlattenedTransforms.h"

#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define ThreeDimensionalStdVectorUnorderedMap(_name, _type1, _type2, _size) std::vector<std::vector<std::vector<std::unordered_map<_type1, _type2>>>> _name(_size, std::vector<std::vector<std::unordered_map<_type1, _type2>>>(_size, std::vector<std::unordered_map<_type1, _type2>>(_size)));

#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

class VerticesPerFace {
public:

};

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<float3>>& transformOfVerticesOfFaceInParticularDir, Vector3 position);
Mesh PlaneFacingDir(Vector3 dir);

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera);

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };

const int numChunks = 10;
const int chunkSize = 16;

Color randColors[9] = {LIGHTGRAY, BLACK, RED, YELLOW, PINK, GREEN, BLUE, PURPLE, GOLD};

void SetRandomColour(float* ambient, int x, int y) {
    Color curColor = randColors[(x + y) % 9];
    ambient[0] = curColor.r;
    ambient[1] = curColor.g;
    ambient[2] = curColor.b;
    ambient[3] = curColor.a;
}

int PackThreeNumbers(int num1, int num2, int num3) {
    // Ensure that each number is within the range [0, 31]
    if (num1 < 0 || num1 > 31 || num2 < 0 || num2 > 31 || num3 < 0 || num3 > 31) {
        std::cerr << "Numbers " << num1 << ", " << num2 << ", " << num3 << "must be between 0 and 31" << std::endl;
        return -1;
    }

    // Pack the three numbers into a single int
    // Shift num1 by 10 bits, num2 by 5 bits, and leave num3 as it is
    int packed = (num1 << 10) | (num2 << 5) | num3;

    return packed;
}

void UnpackThreeNumbers(int packed, unsigned int& num1, unsigned int& num2, unsigned int& num3) {
    // Unpack the numbers by shifting and masking
    num1 = (packed >> 10) & 0x1F;  // 0x1F is the mask for 5 bits (11111)
    num2 = (packed >> 5) & 0x1F;
    num3 = packed & 0x1F;
}

int main()
{

    flecs::world world;

    auto e = world.entity();

    InitWindow(800, 450, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Vector3 position = { 0.0f, 0.0f, 0.0f };

    Texture2D textureLoad = LoadTexture("texture_test_small.png");

    Color colours[9] = { MAGENTA, WHITE, LIGHTGRAY, YELLOW, BLUE, RED, GREEN, ORANGE, VIOLET };

    std::vector<Color> perChunkColours;

    Vector3 curPos = { 0, 0, 0 };

    const int numChunksWidth = (2 * numChunks) + 1;
    std::unordered_map<BlockFaceDirection, Mesh> chunkMeshFacingParticularDir;

    Mesh planeFacingUp = { 0 };
    Mesh planeFacingDown = { 0 };
    Mesh planeFacingFront = { 0 };
    Mesh planeFacingBack = { 0 };
    Mesh planeFacingRight = { 0 };
    Mesh planeFacingLeft = { 0 };

    planeFacingUp = PlaneFacingDir(up);

    planeFacingDown = PlaneFacingDir(down);
    planeFacingFront = PlaneFacingDir(front);
    planeFacingBack = PlaneFacingDir(back);
    planeFacingRight = PlaneFacingDir(right);
    planeFacingLeft = PlaneFacingDir(left);

    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::UP, planeFacingUp });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::DOWN, planeFacingDown });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::FRONT, planeFacingFront });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::BACK, planeFacingBack });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::RIGHT, planeFacingRight });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::LEFT, planeFacingLeft });

    //std::vector<std::vector<std::vector<std::unordered_map<BlockFaceDirection, std::vector<float16>>>>> chunkTransformOfVerticesOfFaceInParticularDir(numChunksWidth, std::vector<std::vector<std::unordered_map<BlockFaceDirection, std::vector<float16>>>(numChunksWidth, std::vector<std::unordered_map<BlockFaceDirection, std::vector<float16>>>(numChunksWidth)));
    ThreeDimensionalStdVectorUnorderedMap(chunkTransformOfVerticesOfFaceInParticularDir, BlockFaceDirection, std::vector<float3>, numChunksWidth);

    int numChunksY = 1;

    Vector3 curChunkPos = { 0, 0, 0 };
    for (int i = -numChunks; i < numChunks; i++)
    {
        curChunkPos.x = i * (2 * chunkSize);

        for (int j = -numChunks; j < numChunks; j++)
        {
            curChunkPos.z = j * (2 * chunkSize);

            for (int k = -numChunksY; k < numChunksY; k++)
            {
                curChunkPos.y = k * (2 * chunkSize);
                GenMeshCustom(chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY], curChunkPos);
            }

        }
    }


    // Load lighting instanceShader
    Shader instanceShader = LoadShader(TextFormat("Shaders/lighting_instancing.vert", GLSL_VERSION),
        TextFormat("Shaders/lighting.frag", GLSL_VERSION));
    // Get instanceShader locations
    instanceShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instanceShader, "mvp");
    instanceShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(instanceShader, "viewPos");
    instanceShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instanceShader, "instanceTransform");

    // Set instanceShader value: ambient light level
    int ambientLoc = GetShaderLocation(instanceShader, "ambient");
    float ambientValue[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    SetShaderValue(instanceShader, ambientLoc, ambientValue, SHADER_UNIFORM_VEC4);

    Material instancedMaterial = LoadMaterialDefault();
    instancedMaterial.shader = instanceShader;
    instancedMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = textureLoad;
    
    DisableCursor();

    while (!WindowShouldClose())
    {
        auto curTime = std::chrono::high_resolution_clock::now();

        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        BeginDrawing();
        ClearBackground(RAYWHITE);

            BeginMode3D(camera);

            for (int k = -numChunksY; k < numChunksY; k++)
            {
                for (int j = -numChunks; j < numChunks; j++)
                {
                    for (int i = -numChunks; i < numChunks; i++)
                    {
                        Vector3 curChunkPos = { i * ((chunkSize * 2)), k * ((chunkSize * 2)), (j * ((chunkSize * 2)))};

                        if (ShouldDrawChunk(curChunkPos, camera)) {

                            int curChunkPosLoc = GetShaderLocation(instanceShader, "curChunkPos");
                            int curChunkPosCoords[3] = { curChunkPos.x, curChunkPos.y, curChunkPos.z };
                            SetShaderValue(instanceShader, curChunkPosLoc, curChunkPosCoords, SHADER_UNIFORM_VEC3);

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::UP]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::UP].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::UP].size());

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::DOWN]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::DOWN].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::DOWN].size());

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::FRONT]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::FRONT].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::FRONT].size());

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::BACK]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::BACK].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::BACK].size());

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::RIGHT]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::RIGHT].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::RIGHT].size());

                            DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::LEFT]
                                , instancedMaterial
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::LEFT].data()
                                , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][k + numChunksY][BlockFaceDirection::LEFT].size());
                        }
                    }
                }
            }

            DrawGrid(10, 1.0);

            EndMode3D();

            DrawFPS(40, 40);

        EndDrawing();
    }

    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);

    CloseWindow();
    return 0;
}

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<float3>> &transformOfVerticesOfFaceInParticularDir
    , Vector3 position)
{
    const siv::PerlinNoise::seed_type seed = 123456u;

    const siv::PerlinNoise perlin{ seed };

    float scale = 0.1f;

    for (int y = -chunkSize; y < chunkSize; y++)
    {
        for (int x = -chunkSize; x < chunkSize; x++)
        {
            for (int z = -chunkSize; z < chunkSize; z++)
            {
                int _x = x + chunkSize + position.x;
                int _y = y + chunkSize + position.y;
                int _z = z + chunkSize + position.z;

                float curNoise = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)_y * scale);

                float emptyThreshold = 0.5f;
                bool curVoxelIsEmpty = curNoise < emptyThreshold ? true : false;

                if (!curVoxelIsEmpty) {

                    int curPositionPacked = PackThreeNumbers(x + chunkSize, y + chunkSize, z + chunkSize);
                    float3 curPosition = {_x, _y, _z};

                    float curNoiseTop = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)(_y + 1) * scale);

                    if (curNoiseTop < emptyThreshold || y == chunkSize) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPosition);
                    }

                    float curNoiseBottom = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)(_y - 1) * scale);

                    if (curNoiseBottom < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPosition);
                    }

                    float curNoiseFront = perlin.noise3D_01((double)_x * scale, (double)(_z + 1) * scale, (double)_y * scale);

                    if (curNoiseFront < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPosition);
                    }

                    float curNoiseBack = perlin.noise3D_01((double)_x * scale, (double)(_z - 1) * scale, (double)_y * scale);

                    if (curNoiseBack < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPosition);
                    }

                    float curNoiseRight = perlin.noise3D_01((double)(_x + 1) * scale, (double)_z * scale, (double)_y * scale);

                    if (curNoiseRight < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPosition);
                    }

                    float curNoiseLeft = perlin.noise3D_01((double)(_x - 1) * scale, (double)_z * scale, (double)_y * scale);

                    if (curNoiseLeft < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curPosition);
                    }

                }

            }
        }
    }
}

Mesh PlaneFacingDir(Vector3 dir) {

    Mesh curMesh = { 0 };

    curMesh.vertices = (float*)MemAlloc(18 * sizeof(float));
    curMesh.texcoords = (float*)MemAlloc(12 * sizeof(float));
    curMesh.indices = (unsigned short*)MemAlloc(6 * sizeof(unsigned short*));

    if (dir.x == 0 && dir.y == 1 && dir.z == 0) {
        FaceIndicesTop(curMesh.indices, 0);
        FaceVerticesTop(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == -1 && dir.z == 0) {
        FaceIndicesBottom(curMesh.indices, 0);
        FaceVerticesBottom(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == 1) {
        FaceIndicesFront(curMesh.indices, 0);
        FaceVerticesFront(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == -1) {
        FaceIndicesBack(curMesh.indices, 0);
        FaceVerticesBack(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 1 && dir.y == 0 && dir.z == 0) {
        FaceIndicesRight(curMesh.indices, 0);
        FaceVerticesRight(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == -1 && dir.y == 0 && dir.z == 0) {
        FaceIndicesLeft(curMesh.indices, 0);
        FaceVerticesLeft(curMesh.vertices, 0, 0, 0);
    }
    TexCoords(curMesh.texcoords);

    curMesh.triangleCount = 6 / 3;
    curMesh.vertexCount = 18 / 3;

    UploadMesh(&curMesh, false);
    
    return curMesh;
}

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera) {

    Vector3 position = { 0, 0, 0 };

    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
    cameraDir = Vector3Normalize(cameraDir);

    Vector3 cameraRight = Vector3CrossProduct(cameraDir, { 0, 1, 0 });
    
    Plane nearPlane = { position, cameraDir };
    Plane farPlane = { position + (cameraDir * 100), cameraDir * -1 };
    Plane rightPlane = { position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * 0.5f), {0, 1, 0}) };
    Plane leftPlane = { position, Vector3CrossProduct({0, 1, 0}, Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * -0.5f)) };
    Plane topPlane = { position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * 0.5f)) };
    Plane bottomPlane = { position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * -0.5f)) };

    float dotProduct = Vector3DotProduct(cameraDir, Vector3Normalize(Vector3Subtract(curChunkPos, camera.position)));

    if (Vector3DotProduct(nearPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, nearPlane.pointOnPlane))) < 0) {
        return false;
    }

    if (Vector3DotProduct(farPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, farPlane.pointOnPlane))) < 0) {
        return false;
    }

    if (Vector3DotProduct(rightPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, rightPlane.pointOnPlane))) < 0) {
        return false;
    }

    if (Vector3DotProduct(leftPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, leftPlane.pointOnPlane))) < 0) {
        return false;
    }

    return true;
}