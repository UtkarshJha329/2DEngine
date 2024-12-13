
#include <iostream>
#include <chrono>
#include <future>

#include "flecs/flecs.h"

#define GRAPHICS_API_OPENGL_43
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "Plane.h"

#define GLSL_VERSION            430

#define FACE_DIRECTION_POSITION 16

#define FACE_UP_INDEX 0
#define FACE_DOWN_INDEX 1
#define FACE_FRONT_INDEX 2
#define FACE_BACK_INDEX 3
#define FACE_RIGHT_INDEX 4
#define FACE_LEFT_INDEX 5

#include "PerlinNoise.hpp"

#include <vector>
#include <string>
#include <map>

#include "CubeMeshData.h"
#include "DrawMeshInstancedFlattenedTransforms.h"

#define AllChunkVoxelStorage(_name, _type, _numChunks, _chunkSize) std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>> _name(_numChunks, std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>(_numChunks, std::vector<std::vector<std::vector<std::vector<_type>>>>(_numChunks, std::vector<std::vector<std::vector<_type>>>(_chunkSize, std::vector<std::vector<_type>>(_chunkSize, std::vector<_type>(_chunkSize))))))
#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define ThreeDimensionalStdVectorUnorderedMap(_name, _type1, _type2, _size) std::vector<std::vector<std::vector<std::unordered_map<_type1, _type2>>>> _name(_size, std::vector<std::vector<std::unordered_map<_type1, _type2>>>(_size, std::vector<std::unordered_map<_type1, _type2>>(_size)));

#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

int PackThreeNumbers(int num1, int num2, int num3) {
    return num1 << 10 | num2 << 5 | num3;
}

static void MakeNoise3D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);
static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);

static void GenMeshCustom3D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk);
static void GenMeshCustom2D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::unordered_map<BlockFaceDirection, Vector2>& chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts
    , std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , std::vector<int>& megaArrayOfAllPositions);

void PlaneFacingDir(Vector3 dir, GenerativeMesh & curMesh);

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera);

static void ReadyIndirectDrawListOfDrawableChunksAndFaces(Vector3 chunkIndex, Camera camera, Vector3 cameraChunkIndex
    , bool chunkGenerated
    , Shader instanceShader, Material instancedMaterial
    , std::vector<std::vector<std::vector<std::unordered_map<BlockFaceDirection, std::vector<int>>>>>& chunkTransformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<std::unordered_map<BlockFaceDirection, Vector2>>>>& chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts
    , std::vector<int> &megaArrayOfAllPosition
    , std::vector<float3>& chunkPositions
    , GenerativeMesh& renderQuad);

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };

const int numChunks = 10;
const int numChunksY = 3;
const int chunkSize = 16;

static std::mutex chunkGeneratedMutex;

static void GenChunkMesh(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::unordered_map<BlockFaceDirection, Vector2>& chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts
    , std::vector<std::vector<std::vector<float>>> &noiseForCurChunk
    , std::vector<int> &arrayOfAllPositions
    , std::unordered_map<int, bool> &chunkGenerated, int chunkIndex) {
    
    //GenMeshCustom3D(transformOfVerticesOfFaceInParticularDir, noiseForCurChunk);
    GenMeshCustom2D(transformOfVerticesOfFaceInParticularDir, chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts, noiseForCurChunk, arrayOfAllPositions);

    std::lock_guard<std::mutex> lock(chunkGeneratedMutex);
    chunkGenerated[chunkIndex] = true;
}

const siv::PerlinNoise::seed_type seed = 76554893u;

const siv::PerlinNoise perlin{ seed };

const int farPlaneDistance = 1000;

std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommands;

//std::vector<float> chunkPositionsFlattened;

int main()
{

    flecs::world world;

    auto e = world.entity();

    InitWindow(800, 450, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Vector3 position = { 0.0f, 0.0f, 0.0f };

    Texture2D textureLoad = LoadTexture("texture_test_small.png");

    ThreeDimensionalStdVectorUnorderedMap(chunkTransformOfVerticesOfFaceInParticularDir, BlockFaceDirection, std::vector<int>, chunkSize);
    ThreeDimensionalStdVectorUnorderedMap(chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts, BlockFaceDirection, Vector2, chunkSize);
    AllChunkVoxelStorage(noise3D, float, numChunks + 1, chunkSize + 3);

    std::unordered_map<int, bool> chunkGenerated;
    std::vector<std::future<void>> chunkMeshGenThreads;

    float scale = 0.1f;
    //MakeNoise3D(noise3D, numChunks, numChunksY, chunkSize, scale);
    MakeNoise2D(noise3D, numChunks, numChunksY, chunkSize, scale);

    std::vector<int> megaArrayOfAllPositions;
    std::vector<float3> chunkPositions;

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

                //chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });

                int curID = i << 10 | k << 5 | j;
                //std::cout << chunkIndex << std::endl;

                //why i,j,k instead of i, k, j? I don't know.
                chunkMeshGenThreads.push_back(std::async(std::launch::async, GenChunkMesh
                    , std::ref(chunkTransformOfVerticesOfFaceInParticularDir[i][j][k])
                    , std::ref(chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k])
                    , std::ref(noise3D[i][k][j])
                    , std::ref(megaArrayOfAllPositions)
                    , std::ref(chunkGenerated), curID));

            }

        }
    }

    GenerativeMesh renderQuad = { 0 };
    PlaneFacingDir(up, renderQuad);
    renderQuad.instanceVBOID = 0;

    int indirectBufferVBO = 0;

    std::vector<Vector3> renderTraversalOrder;
    renderTraversalOrder.push_back(Vector3{ 0, 0, 0 });

    int curLayerNum = 1;
    for (int y = 0; y < numChunksY; y++)
    {
        while (curLayerNum < numChunks) {

            for (int z = -curLayerNum; z <= curLayerNum; z++) {
                Vector3 chunkIndex = Vector3{ (float)(curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
            }

            for (int z = -curLayerNum; z <= curLayerNum; z++) {
                Vector3 chunkIndex = Vector3{ (float)(-curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
            }

            for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x++) {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
            }

            for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x++) {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(-curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
            }
            curLayerNum++;
        }
        curLayerNum = 0;
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

    int strayCounter = -1;

    while (!WindowShouldClose())
    {
        strayCounter++;
        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        chunkPositions.clear();
        //chunkPositionsFlattened.clear();
        drawArraysIndirectCommands.clear();
        
        BeginDrawing();
        ClearBackground(RAYWHITE);

            int numVoxelsPerChunk = chunkSize;
            Vector3 cameraChunkPos = { (int)camera.position.x / numVoxelsPerChunk, (int)camera.position.y / numVoxelsPerChunk, (int)camera.position.z / numVoxelsPerChunk };

            BeginMode3D(camera);

            for (int i = 0; i < renderTraversalOrder.size(); i++)
            {

                Vector3 curChunkTraversalIndex = Vector3{ renderTraversalOrder[i].x + cameraChunkPos.x, renderTraversalOrder[i].y, renderTraversalOrder[i].z + cameraChunkPos.z };

                if (curChunkTraversalIndex.x < 0 || curChunkTraversalIndex.y < 0 || curChunkTraversalIndex.z < 0
                    || curChunkTraversalIndex.x >= numChunks || curChunkTraversalIndex.y >= numChunksY || curChunkTraversalIndex.z >= numChunks) {
                    //std::cout << "Skipping. " << curChunkTraversalIndex.x << ", " << curChunkTraversalIndex.y << ", " << curChunkTraversalIndex.z << std::endl;
                    continue;
                }
                Vector3 curChunkPos = { curChunkTraversalIndex.x * chunkSize, curChunkTraversalIndex.y * chunkSize, curChunkTraversalIndex.z * chunkSize };

                int curID = (int)curChunkTraversalIndex.x << 10 | (int)curChunkTraversalIndex.y << 5 | (int)curChunkTraversalIndex.z;
                //std::cout << "traversing: " << strayCounter << std::endl;
                ReadyIndirectDrawListOfDrawableChunksAndFaces(curChunkTraversalIndex, camera, cameraChunkPos, chunkGenerated[curID], instanceShader, instancedMaterial, chunkTransformOfVerticesOfFaceInParticularDir, chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts, megaArrayOfAllPositions, chunkPositions, renderQuad);
            }

            //std::cout << drawArraysIndirectCommands.size() << " : " << chunkPositions.size() << std::endl;

            unsigned int chunkPosSSBO = rlLoadShaderBuffer(chunkPositions.size() * sizeof(float3), chunkPositions.data(), RL_DYNAMIC_DRAW);
            rlBindShaderBuffer(chunkPosSSBO, 3);
            //std::cout << chunkPositions.size() << std::endl;
            //std::cout << drawArraysIndirectCommands.size() << std::endl;

            DrawMeshMultiInstancedDrawIndirect(renderQuad, instancedMaterial, megaArrayOfAllPositions.data(), megaArrayOfAllPositions.size(), drawArraysIndirectCommands, drawArraysIndirectCommands.size());

            DrawGrid(10, 1.0);

            EndMode3D();
            
            //int numVoxelsPerChunk = chunkSize * 2;
            //Vector3 cameraChunkPos = { (int)camera.position.x / numVoxelsPerChunk, (int)camera.position.y / numVoxelsPerChunk, (int)camera.position.z / numVoxelsPerChunk };
            //Vector3 cameraVoxelPos = { (int)camera.position.x - cameraChunkPos.x, (int)camera.position.y - cameraChunkPos.y, (int)camera.position.z - cameraChunkPos.z };
            //cameraVoxelPos = Vector3SubtractValue(cameraVoxelPos, numVoxelsPerChunk / 2);
            //std::string cameraVoxelPosText = std::to_string(cameraVoxelPos.x) + ", " + std::to_string(cameraVoxelPos.y) + ", " + std::to_string(cameraVoxelPos.z);
            std::string cameraChunkPosText = std::to_string(cameraChunkPos.x) + ", " + std::to_string(cameraChunkPos.y) + ", " + std::to_string(cameraChunkPos.z);
            //DrawText(cameraVoxelPosText.c_str(), 0, 0, 20, BLACK);
            DrawText(cameraChunkPosText.c_str(), 0, 120, 20, BLACK);
            //int x = (int)camera.position.x;
            //int y = (int)camera.position.y;
            //int z = (int)camera.position.z;
            //float scale = 0.1f;
            //std::string curNoiseValue = std::to_string(perlin.noise3D_01((double)x * scale, (double)z * scale, (double)(y) * scale));
            //DrawText(curNoiseValue.c_str(), 0, 140, 20, BLACK);
            DrawFPS(40, 40);

        EndDrawing();
    }

    //rlUnloadShaderBuffer(chunkPosSSBO);
    rlUnloadVertexBuffer(indirectBufferVBO);
    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);
    CloseWindow();
    return 0;
}

static void ReadyIndirectDrawListOfDrawableChunksAndFaces(Vector3 chunkIndex, Camera camera, Vector3 cameraChunkIndex
    , bool chunkGenerated
    , Shader instanceShader, Material instancedMaterial
    , std::vector<std::vector<std::vector<std::unordered_map<BlockFaceDirection, std::vector<int>>>>> &chunkTransformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<std::unordered_map<BlockFaceDirection, Vector2>>>> &chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts
    , std::vector<int> &megaArrayOfAllPositions
    , std::vector<float3> &chunkPositions
    , GenerativeMesh &renderQuad) {

    int i = chunkIndex.x;
    int k = chunkIndex.y;
    int j = chunkIndex.z;

    Vector3 curChunkPos = { i * chunkSize, k * chunkSize, j * chunkSize };

    bool cameraInThisChunkWidthAndBreadth = chunkIndex.x <= cameraChunkIndex.x || chunkIndex.z <= cameraChunkIndex.z;
    Vector3 dirToChunkFromCamera = curChunkPos - camera.position;

    dirToChunkFromCamera = Vector3Normalize(dirToChunkFromCamera);

    bool shouldDrawChunk = ShouldDrawChunk(curChunkPos, camera);
    //std::cout << shouldDrawChunk << std::endl;
    if (shouldDrawChunk) {

        if (chunkGenerated) {

            //std::cout << "Chunk Created." << std::endl;

            //int curChunkPosLoc = GetShaderLocation(instanceShader, "curChunkPos");
            //float curChunkPosValue[3] = { curChunkPos.x, curChunkPos.y, curChunkPos.z };
            //SetShaderValue(instanceShader, curChunkPosLoc, curChunkPosValue, SHADER_UNIFORM_VEC3);

            float dotUp = Vector3DotProduct(dirToChunkFromCamera, up);
            float dotDown = Vector3DotProduct(dirToChunkFromCamera, down);
            float dotFront = Vector3DotProduct(dirToChunkFromCamera, front);
            float dotBack = Vector3DotProduct(dirToChunkFromCamera, back);
            float dotRight = Vector3DotProduct(dirToChunkFromCamera, right);
            float dotLeft = Vector3DotProduct(dirToChunkFromCamera, left);

            if (dotUp < 0 || cameraInThisChunkWidthAndBreadth) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::UP].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::UP].size();
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start};
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });                    
                    //std::cout << "x : " << curChunkPos.x << " y : " << curChunkPos.y << " z : " << curChunkPos.z << std::endl;
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }

            if (dotDown < 0 || cameraInThisChunkWidthAndBreadth || true) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::DOWN].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::DOWN].size();
                if (numInstances > 0 || true) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }

            if (dotFront < 0 || cameraInThisChunkWidthAndBreadth || true) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::FRONT].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::FRONT].size();
                if (numInstances > 0 || true) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }

            if (dotBack < 0 || cameraInThisChunkWidthAndBreadth || true) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::BACK].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::BACK].size();
                if (numInstances > 0 || true) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }

            if (dotRight < 0 || cameraInThisChunkWidthAndBreadth || true) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::RIGHT].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::RIGHT].size();
                if (numInstances > 0 || true) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }

            if (dotLeft < 0 || cameraInThisChunkWidthAndBreadth || true) {
                int start = chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[i][j][k][BlockFaceDirection::LEFT].x;
                int numInstances = chunkTransformOfVerticesOfFaceInParticularDir[i][j][k][BlockFaceDirection::LEFT].size();
                if (numInstances > 0 || true) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    drawArraysIndirectCommands.push_back(curCommand);
                    chunkPositions.push_back(float3{ curChunkPos.x, curChunkPos.y, curChunkPos.z });
                    //chunkPositions.push_back(float3{ 0, 0, 0});
                }
            }
        }

    }
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

static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale) {

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

                    for (int z = 0; z <= chunkSize + 1; z++)
                    {
                        _z = z + chunksZ * chunksSize;

                        float noise = perlin.noise2D_01((double)_x * scale, (double)_z * scale);

                        int scaledNoise = (int)(noise * chunksSize * numChunksY);

                        for (int y = 0; y <= chunkSize + 1; y++)
                        {
                            _y = y + chunksY * chunkSize;
                            if (_y < scaledNoise) {// This is the position under the noise height.
                                noiseStorage[chunksX][chunksY][chunksZ][x][y][z] = 0; // STONE BLOCK
                            }
                            else if (_y == scaledNoise) {// This is the noise height.
                                noiseStorage[chunksX][chunksY][chunksZ][x][y][z] = 1; // DIRT BLOCK
                            }
                            else if (_y > scaledNoise) {// This is the position above the noise height.
                                noiseStorage[chunksX][chunksY][chunksZ][x][y][z] = 2; // AIR BLOCK
                            }
                            
                            //float value = (float)_y / (float)scaledNoise;
                            //noiseStorage[chunksX][chunksY][chunksZ][x][y][z] = value > 1 ? ceil(value) : 0;

                        }
                    }
                }
            }
        }
    }

}

static void GenMeshCustom3D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
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

static std::mutex megaArrayInsertionMutex;
//Can be multithreaded to increase performance by doing one thread per face direction.
static void GenMeshCustom2D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::unordered_map<BlockFaceDirection, Vector2>& chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts
    , std::vector<std::vector<std::vector<float>>> &noiseForCurrentChunk
    , std::vector<int> &megaArrayOfAllPositions)
{
    float scale = 0.1f;

    for (int y = 1; y <= chunkSize; y++)
    {
        for (int x = 1; x <= chunkSize; x++)
        {
            for (int z = 1; z <= chunkSize; z++)
            {
                float curNoise = noiseForCurrentChunk[x][y][z];

                if (curNoise == 1 || curNoise == 0) {

                    int curPosition = PackThreeNumbers(x - 1, y - 1, z - 1);

                    float curNoiseTop = noiseForCurrentChunk[x][y + 1][z];

                    if (curNoiseTop == 2) {
                        int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPositionTemp);
                    }

                    float curNoiseBottom = noiseForCurrentChunk[x][y - 1][z];

                    if (curNoiseBottom == 2) {
                        int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPositionTemp);
                    }

                    float curNoiseFront = noiseForCurrentChunk[x][y][z + 1];

                    if (curNoiseFront == 2) {
                        int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPositionTemp);
                    }

                    float curNoiseBack = noiseForCurrentChunk[x][y][z - 1];

                    if (curNoiseBack == 2) {
                        int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPositionTemp);
                    }

                    float curNoiseRight = noiseForCurrentChunk[x + 1][y][z];

                    if (curNoiseRight == 2) {
                        int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPositionTemp);
                    }

                    float curNoiseLeft = noiseForCurrentChunk[x - 1][y][z];

                    if (curNoiseLeft == 2) {
                        int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curPositionTemp);
                    }

                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(megaArrayInsertionMutex);
    auto endOfMegaArray = megaArrayOfAllPositions.end();

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::UP].x = megaArrayOfAllPositions.size();
    auto startOfUpArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].begin();
    auto endOfUpArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfUpArray, endOfUpArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::UP].y = megaArrayOfAllPositions.size();

    //std::cout << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size() << " :- " << chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::UP].x << ", " << chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::UP].y << std::endl;

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::DOWN].x = megaArrayOfAllPositions.size();
    endOfMegaArray = megaArrayOfAllPositions.end();
    auto startOfDownArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].begin();
    auto endOfDownArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfDownArray, endOfDownArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::DOWN].y = megaArrayOfAllPositions.size();

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::FRONT].x = megaArrayOfAllPositions.size();
    endOfMegaArray = megaArrayOfAllPositions.end();
    auto startOfFrontArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].begin();
    auto endOfFrontArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfFrontArray, endOfFrontArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::FRONT].y = megaArrayOfAllPositions.size();

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::BACK].x = megaArrayOfAllPositions.size();
    endOfMegaArray = megaArrayOfAllPositions.end();
    auto startOfBackArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].begin();
    auto endOfBackArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfBackArray, endOfBackArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::BACK].y = megaArrayOfAllPositions.size();

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::RIGHT].x = megaArrayOfAllPositions.size();
    endOfMegaArray = megaArrayOfAllPositions.end();
    auto startOfRightArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].begin();
    auto endOfRightArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfRightArray, endOfRightArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::RIGHT].y = megaArrayOfAllPositions.size();

    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::LEFT].x = megaArrayOfAllPositions.size();
    endOfMegaArray = megaArrayOfAllPositions.end();
    auto startOfLeftArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].begin();
    auto endOfLeftArray = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].end();
    megaArrayOfAllPositions.insert(endOfMegaArray, startOfLeftArray, endOfLeftArray);
    chunkTransformOfVerticesOfFaceInParticularDirOffsetCounts[BlockFaceDirection::LEFT].y = megaArrayOfAllPositions.size();

}

void PlaneFacingDir(Vector3 dir, GenerativeMesh &curMesh) {

    curMesh.mesh.vertices = (float*)MemAlloc(4 * 3 * sizeof(float));
    curMesh.mesh.texcoords = (float*)MemAlloc(4 * 2 * sizeof(float));
    //curMesh.indices = (unsigned short*)MemAlloc(6 * sizeof(unsigned short*));

    if (dir.x == 0 && dir.y == 1 && dir.z == 0) {
        //FaceIndicesTop(curMesh.indices, 0);
        FaceVerticesTop(curMesh.mesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == -1 && dir.z == 0) {
        //FaceIndicesBottom(curMesh.indices, 0);
        FaceVerticesBottom(curMesh.mesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == 1) {
        //FaceIndicesFront(curMesh.indices, 0);
        FaceVerticesFront(curMesh.mesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == -1) {
        //FaceIndicesBack(curMesh.indices, 0);
        FaceVerticesBack(curMesh.mesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 1 && dir.y == 0 && dir.z == 0) {
        //FaceIndicesRight(curMesh.indices, 0);
        FaceVerticesRight(curMesh.mesh.vertices, 0, 0, 0);
    }
    else if (dir.x == -1 && dir.y == 0 && dir.z == 0) {
        //FaceIndicesLeft(curMesh.indices, 0);
        FaceVerticesLeft(curMesh.mesh.vertices, 0, 0, 0);
    }
    TexCoords(curMesh.mesh.texcoords);

    curMesh.mesh.triangleCount = 2;
    curMesh.mesh.vertexCount = 4;

    UploadMesh(&curMesh.mesh, false);
}

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera) {

    float diagonalDist = 3 * chunkSize * 1.732f;

    Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
    cameraDir = Vector3Normalize(cameraDir);

    Vector3 cameraRight = Vector3CrossProduct(cameraDir, { 0, 1, 0 });
    Vector3 position = camera.position - cameraDir * diagonalDist * 2 * 1.414f;

    Plane nearPlane = { position, cameraDir };
    Plane farPlane = { position + (cameraDir * (farPlaneDistance + diagonalDist * 1.414f)), cameraDir * -1 };
    Plane rightPlane = { position, Vector3CrossProduct(Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * 0.5f), {0, 1, 0}) };
    Plane leftPlane = { position, Vector3CrossProduct({0, 1, 0}, Vector3RotateByAxisAngle(cameraDir, {0, 1, 0}, DEG2RAD * camera.fovy * -0.5f)) };
    Plane topPlane = { position, Vector3CrossProduct(cameraRight, Vector3RotateByAxisAngle(cameraDir, cameraRight, DEG2RAD * camera.fovy * 0.5f)) };
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

    return true;
    
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
}