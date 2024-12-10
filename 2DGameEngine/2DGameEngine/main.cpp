
#include <iostream>
#include <chrono>
#include <thread>

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

#define Noise3D(_name, _type, _numChunks, _chunkSize) std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>> _name(_numChunks, std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>(_numChunks, std::vector<std::vector<std::vector<std::vector<_type>>>>(_numChunks, std::vector<std::vector<std::vector<_type>>>(_chunkSize, std::vector<std::vector<_type>>(_chunkSize, std::vector<_type>(_chunkSize))))))
#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define ThreeDimensionalStdVectorUnorderedMap(_name, _type1, _type2, _size) std::vector<std::vector<std::vector<std::unordered_map<_type1, _type2>>>> _name(_size, std::vector<std::vector<std::unordered_map<_type1, _type2>>>(_size, std::vector<std::unordered_map<_type1, _type2>>(_size)));

#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

int PackThreeNumbers(int num1, int num2, int num3) {
    return (num1 << 10) | (num2 << 5) | num3;
}

static void MakeNoise3D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);
static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk);
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

static void GenChunkMesh(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>> &noiseForCurChunk 
    , std::unordered_map<int, bool> &chunkGenerated, int chunkIndex) {
    
    GenMeshCustom(transformOfVerticesOfFaceInParticularDir, noiseForCurChunk);
    chunkGenerated[chunkIndex] = true;
}

const siv::PerlinNoise::seed_type seed = 123456u;

const siv::PerlinNoise perlin{ seed };

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
    //ThreeDimensionalStdVectorUnorderedMap(chunkTransformOfVerticesOfFaceInParticularDir, BlockFaceDirection, std::vector<float3>, numChunksWidth);
    ThreeDimensionalStdVectorUnorderedMap(chunkTransformOfVerticesOfFaceInParticularDir, BlockFaceDirection, std::vector<int>, chunkSize);
    Noise3D(noise3D, float, numChunks + 1, chunkSize + 3);
    std::unordered_map<int, bool> chunkGenerated;

    int numChunksY = 3;

    std::vector<std::thread> chunkMeshGenThreads;

    float scale = 0.1f;
    MakeNoise3D(noise3D, numChunks, numChunksY, chunkSize, scale);

    Vector3 curChunkPos = { 0, 0, 0 };
    for (int i = 0; i < numChunks; i++)
    {
        curChunkPos.x = i * chunkSize;

        for (int j = 0; j < numChunks; j++)
        {
            curChunkPos.z = j * chunkSize;

            for (int k = 0; k < numChunksY; k++)
            {
                curChunkPos.y = k * chunkSize;

                int curID = i << 10 | k << 5 | j;
                //std::cout << chunkIndex << std::endl;
                chunkMeshGenThreads.push_back(std::thread(GenChunkMesh
                    , std::ref(chunkTransformOfVerticesOfFaceInParticularDir[i][j][k])
                    , std::ref(noise3D[i][k][j])
                    , std::ref(chunkGenerated), curID));

                //MeshGeneratingThread.detach();
                
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
        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        BeginDrawing();
        ClearBackground(RAYWHITE);

            BeginMode3D(camera);

            for (int k = 0; k < numChunksY; k++)
            {
                for (int j = 0; j < numChunks; j++)
                {
                    for (int i = 0; i < numChunks; i++)
                    {
                        Vector3 curChunkPos = { i * chunkSize, k * chunkSize, j * chunkSize};

                        //Vector3 chunkIndex = { i + numChunks, j + numChunks, k + numChunksY };
                        if (ShouldDrawChunk(curChunkPos, camera)) {

                            int curID = i << 10 | k << 5 | j;

                            if (chunkGenerated[curID]) {

                                int curChunkPosLoc = GetShaderLocation(instanceShader, "curChunkPos");
                                float curChunkPosValue[3] = { curChunkPos.x, curChunkPos.y, curChunkPos.z };
                                SetShaderValue(instanceShader, curChunkPosLoc, curChunkPosValue, SHADER_UNIFORM_VEC3);

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::UP]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::UP].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::UP].size());

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::DOWN]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::DOWN].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::DOWN].size());

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::FRONT]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::FRONT].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::FRONT].size());

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::BACK]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::BACK].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::BACK].size());

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::RIGHT]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::RIGHT].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::RIGHT].size());

                                DrawMeshInstancedFlattenedPositions(chunkMeshFacingParticularDir[BlockFaceDirection::LEFT]
                                    , instancedMaterial
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::LEFT].data()
                                    , chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::LEFT].size());
                            }

                        }
                    }
                }
            }

            DrawGrid(10, 1.0);

            EndMode3D();
            
            //int numVoxelsPerChunk = chunkSize * 2;
            //Vector3 cameraChunkPos = { (int)camera.position.x / numVoxelsPerChunk, (int)camera.position.y / numVoxelsPerChunk, (int)camera.position.z / numVoxelsPerChunk };
            //Vector3 cameraVoxelPos = { (int)camera.position.x - cameraChunkPos.x, (int)camera.position.y - cameraChunkPos.y, (int)camera.position.z - cameraChunkPos.z };
            //cameraVoxelPos = Vector3SubtractValue(cameraVoxelPos, numVoxelsPerChunk / 2);
            //std::string cameraVoxelPosText = std::to_string(cameraVoxelPos.x) + ", " + std::to_string(cameraVoxelPos.y) + ", " + std::to_string(cameraVoxelPos.z);
            //std::string cameraChunkPosText = std::to_string(cameraChunkPos.x) + ", " + std::to_string(cameraChunkPos.y) + ", " + std::to_string(cameraChunkPos.z);
            //DrawText(cameraVoxelPosText.c_str(), 0, 0, 20, BLACK);
            //DrawText(cameraChunkPosText.c_str(), 0, 120, 20, BLACK);
            //int x = (int)camera.position.x;
            //int y = (int)camera.position.y;
            //int z = (int)camera.position.z;
            //float scale = 0.1f;
            //std::string curNoiseValue = std::to_string(perlin.noise3D_01((double)x * scale, (double)z * scale, (double)(y) * scale));
            //DrawText(curNoiseValue.c_str(), 0, 140, 20, BLACK);
            DrawFPS(40, 40);

        EndDrawing();
    }

    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);

    for (int i = 0; i < chunkMeshGenThreads.size(); i++)
    {
        chunkMeshGenThreads[i].join();
    }

    CloseWindow();
    return 0;
}

static void MakeNoise3D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>> &noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale) {

    int _x, _y, _z = 0;

    for (int chunksX = 0; chunksX < numChunks; chunksX++)
    {
        for (int chunksY = 0; chunksY < numChunksY; chunksY++)
        {
            for (int chunksZ = 0; chunksZ < numChunks; chunksZ++)
            {
                for (int x = 0; x <= chunkSize + 1; x++)
                {
                    _x = x + chunksX * chunkSize;

                    for (int y = 0; y <= chunkSize + 1; y++)
                    {
                        _y = y + chunksY * chunkSize;
                        for (int z = 0; z <= chunkSize + 1; z++)
                        {
                            _z = z + chunksZ * chunksSize;
                            noiseStorage[chunksX][chunksY][chunksZ][x][y][z] = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)_y * scale);
                        }
                    }
                }
            }
        }
    }

}

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>> &noiseForCurrentChunk)
{
    float scale = 0.1f;

    for (int y = 1; y <= chunkSize; y++)
    {
        for (int x = 1; x <= chunkSize; x++)
        {
            for (int z = 1; z <= chunkSize; z++)
            {
                float curNoise = noiseForCurrentChunk[x][y][z];

                float emptyThreshold = 0.5f;
                bool curVoxelIsEmpty = curNoise < emptyThreshold ? true : false;

                if (!curVoxelIsEmpty) {

                    int curPosition = PackThreeNumbers(x - 1, y - 1, z - 1);

                    float curNoiseTop = noiseForCurrentChunk[x][y + 1][z];

                    if (curNoiseTop < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPosition);
                    }

                    float curNoiseBottom = noiseForCurrentChunk[x][y - 1][z];

                    if (curNoiseBottom < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPosition);
                    }

                    float curNoiseFront = noiseForCurrentChunk[x][y][z + 1];

                    if (curNoiseFront < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPosition);
                    }

                    float curNoiseBack = noiseForCurrentChunk[x][y][z - 1];

                    if (curNoiseBack < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPosition);
                    }

                    float curNoiseRight = noiseForCurrentChunk[x + 1][y][z];

                    if (curNoiseRight < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPosition);
                    }

                    float curNoiseLeft = noiseForCurrentChunk[x - 1][y][z];

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

//bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera) {
//
//    Vector3 position = { 0, 0, 0 };
//
//    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
//    cameraDir = Vector3Normalize(cameraDir);
//
//    Vector3 cameraRight = Vector3CrossProduct(cameraDir, { 0, 1, 0 });
//    
//    Plane nearPlane = { camera.position, cameraDir };
//    Plane farPlane = { camera.position + (cameraDir * 100), cameraDir * -1 };
//    Plane rightPlane = { camera.position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * 0.5f), {0, 1, 0}) };
//    Plane leftPlane = { camera.position, Vector3CrossProduct({0, 1, 0}, Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * -0.5f)) };
//    Plane topPlane = { camera.position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * 0.5f), cameraRight) };
//    Plane bottomPlane = { camera.position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * -0.5f)) };
//
//
//    if (Vector3DotProduct(nearPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, nearPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    if (Vector3DotProduct(farPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, farPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    if (Vector3DotProduct(rightPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, rightPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    if (Vector3DotProduct(leftPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, leftPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    if (Vector3DotProduct(topPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, topPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    if (Vector3DotProduct(bottomPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, bottomPlane.pointOnPlane))) < 0) {
//        return false;
//    }
//
//    return true;
//}

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera) {

    float diagonalDist = 2 * chunkSize * 1.732f;

    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
    cameraDir = Vector3Normalize(cameraDir);

    Vector3 cameraRight = Vector3CrossProduct(cameraDir, { 0, 1, 0 });
    Vector3 position = camera.position - cameraDir * diagonalDist * 1.414f;

    Plane nearPlane = { position, cameraDir };
    Plane farPlane = { position + (cameraDir * (100 + diagonalDist * 1.414f)), cameraDir * -1 };
    Plane rightPlane = { position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * 0.5f), {0, 1, 0}) };
    Plane leftPlane = { position, Vector3CrossProduct({0, 1, 0}, Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * -0.5f)) };
    Plane topPlane = { position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * 0.5f), cameraRight) };
    Plane bottomPlane = { position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * -0.5f)) };

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

    if (Vector3DotProduct(topPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, topPlane.pointOnPlane))) < 0) {
        return false;
    }

    if (Vector3DotProduct(bottomPlane.normal, Vector3Normalize(Vector3Subtract(curChunkPos, bottomPlane.pointOnPlane))) < 0) {
        return false;
    }

    return true;

    //float distFromNearPlane = Vector3DotProduct(nearPlane.normal, (Vector3Subtract(curChunkPos, nearPlane.pointOnPlane)));
    //float distFromFarPlane = Vector3DotProduct(farPlane.normal, (Vector3Subtract(curChunkPos, farPlane.pointOnPlane)));
    //float distFromRightPlane = Vector3DotProduct(rightPlane.normal, (Vector3Subtract(curChunkPos, rightPlane.pointOnPlane)));
    //float distFromLeftPlane = Vector3DotProduct(leftPlane.normal, (Vector3Subtract(curChunkPos, leftPlane.pointOnPlane)));
    //float distFromTopPlane = Vector3DotProduct(topPlane.normal, (Vector3Subtract(curChunkPos, topPlane.pointOnPlane)));
    //float distFromBottomPlane = Vector3DotProduct(bottomPlane.normal, (Vector3Subtract(curChunkPos, bottomPlane.pointOnPlane)));

    //std::cout << abs(distFromNearPlane) << ", " 
    //            << abs(distFromFarPlane) << ", " 
    //            << abs(distFromRightPlane) << ", " 
    //            << abs(distFromLeftPlane) << ", " 
    //            << abs(distFromTopPlane) << ", " 
    //            << abs(distFromBottomPlane) << std::endl;

    //THE PROBLEM
    //IS THAT
    //THE PLANES EXTEND TO INFINITY
    //SO EVERYTHING NEAR THOSE PLANES IS CONSIDERED LEGIBLE WITH THIS
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    //VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV

    //if (abs(distFromNearPlane) <= diagonalDist
    //    || abs(distFromFarPlane) <= diagonalDist
    //    || abs(distFromRightPlane) <= diagonalDist
    //    || abs(distFromLeftPlane) <= diagonalDist
    //    || abs(distFromTopPlane) <= diagonalDist
    //    || abs(distFromBottomPlane) <= diagonalDist) {
    //    return true;
    //}
    
    return false;
}

//bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera) {
//
//    Vector3 position = { 0, 0, 0 };
//
//    const int numCorners = 6;
//    Vector3 curChunkCornerVertices[numCorners] = {
//        {curChunkPos.x - chunkSize, curChunkPos.y - chunkSize, curChunkPos.z - chunkSize}
//        ,{curChunkPos.x - chunkSize, curChunkPos.y - chunkSize, curChunkPos.z + chunkSize}
//        ,{curChunkPos.x - chunkSize, curChunkPos.y + chunkSize, curChunkPos.z + chunkSize}
//        ,{curChunkPos.x + chunkSize, curChunkPos.y - chunkSize, curChunkPos.z - chunkSize}
//        ,{curChunkPos.x + chunkSize, curChunkPos.y + chunkSize, curChunkPos.z - chunkSize}
//        ,{curChunkPos.x + chunkSize, curChunkPos.y + chunkSize, curChunkPos.z + chunkSize}
//    };
//
//    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
//    cameraDir = Vector3Normalize(cameraDir);
//
//    Vector3 cameraRight = Vector3CrossProduct(cameraDir, { 0, 1, 0 });
//
//    Plane nearPlane = { camera.position, cameraDir };
//    Plane farPlane = { camera.position + (cameraDir * 100), cameraDir * -1 };
//    Plane rightPlane = { camera.position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * 0.5f), {0, 1, 0}) };
//    Plane leftPlane = { camera.position, Vector3CrossProduct({0, 1, 0}, Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * -0.5f)) };
//    Plane topPlane = { camera.position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * 0.5f), cameraRight) };
//    Plane bottomPlane = { camera.position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * -0.5f)) };
//
//    float dotProduct = Vector3DotProduct(cameraDir, Vector3Normalize(Vector3Subtract(curChunkPos, camera.position)));
//
//    for (int i = 0; i < numCorners; i++)
//    {
//        if (Vector3DotProduct(nearPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], nearPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        if (Vector3DotProduct(farPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], farPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        if (Vector3DotProduct(rightPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], rightPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        if (Vector3DotProduct(leftPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], leftPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        if (Vector3DotProduct(topPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], topPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        if (Vector3DotProduct(bottomPlane.normal, Vector3Normalize(Vector3Subtract(curChunkCornerVertices[i], bottomPlane.pointOnPlane))) < 0) {
//            break;
//        }
//
//        return true;
//    }
//
//    return false;
//}