
#include <iostream>
#include <chrono>
#include <future>
#include <filesystem>

#include <queue>

#include "flecs/flecs.h"

#define GLSL_VERSION            430
#define GRAPHICS_API_OPENGL_43
#include "raylib/raylib.h"

#include "Plane.h"
#include "PerlinNoise.hpp"

#include "nlohmann/json.hpp"

#include <vector>
#include <string>
#include <fstream>

#include "WorldConstants.h"
#include "VertexPositions.h"

#include "CubeMeshData.h"
#include "InstancedDrawing.h"

#include "Instrumentor.h"

#define FORCE_DEDICATED_GPU 0
#if FORCE_DEDICATED_GPU
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#else
#endif

struct ChangesToMegaPositionsArray {
    int start   = 0;
    int size    = 0;
    int offset  = 0;
};

#define AllChunkVoxelStorage(_name, _type, _numChunks, _chunkSize) std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>> _name(_numChunks, std::vector<std::vector<std::vector<std::vector<std::vector<_type>>>>>(_numChunks, std::vector<std::vector<std::vector<std::vector<_type>>>>(_numChunks, std::vector<std::vector<std::vector<_type>>>(_chunkSize, std::vector<std::vector<_type>>(_chunkSize, std::vector<_type>(_chunkSize))))))
#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define ThreeDimensionalStdVectorUnorderedMap(_name, _type1, _type2, _size) std::vector<std::vector<std::vector<std::unordered_map<_type1, _type2>>>> _name(_size, std::vector<std::vector<std::unordered_map<_type1, _type2>>>(_size, std::vector<std::unordered_map<_type1, _type2>>(_size)));

#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

int PackThreeNumbers(int num1, int num2, int num3) {
    return num1 << 10 | num2 << 5 | num3;
}

static void MakeNoise3D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);
static void MakeNoiseForChunk(std::vector<std::vector<std::vector<float>>>& noiseStorage, int chunksX, int chunksY, int chunksZ, int numChunks, int numChunksY, int chunksSize, float scale);
static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);

static void ConvolutionSum(std::vector<std::vector<std::vector<float>>>& noiseStorage, int convolutionSize, int convolutionPositionX, int convolutionPositionY, int convolutionPositionZ, int lodLevel);
static void ConvoluteNoise(std::vector<std::vector<std::vector<float>>>& noiseStorage, int lodLevel);

static void GenMeshCustom3D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk);
static void GenMeshCustom2D(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , VertexPositions &megaVertPositions
    , Vector3 rescopedChunkIndex
    , Vector3 chunkIndex);

void PlaneFacingDir(Vector3 dir, GenerativeMesh & curMesh);

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera
    , Plane& nearPlane
    , Plane& farPlane
    , Plane& rightPlane
    , Plane& leftPlane
    , Plane& topPlane
    , Plane& bottomPlane);

static void ReadyIndirectDrawListOfDrawableChunksAndFaces(Vector3 innerChunkIndex, Vector3 drawChunkIndex
                                                        , Camera camera, Vector3 cameraChunkIndex
                                                        , Shader instanceShader, Material instancedMaterial
                                                        , VertexPositions& megaVertPositions
                                                        , std::vector<float3>& chunkPositions
                                                        , GenerativeMesh& renderQuad
                                                        , Plane& nearPlane
                                                        , Plane& farPlane
                                                        , Plane& rightPlane
                                                        , Plane& leftPlane
                                                        , Plane& topPlane
                                                        , Plane& bottomPlane);

static std::unordered_map<int, int> chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray;

static std::mutex chunkGeneratedMutex;
static std::mutex chunkBeingGeneratedCountMutex;
static std::mutex chunkUpdatedIndexWithVoxelMutex;
static void GenChunkMeshWithNoise(VertexPositions &megaVertPositions
                                , std::unordered_map<int, bool> &chunkGenerated
                                , std::unordered_map<int, int> &chunkUpdatedIndexWithVoxelMatchedToChunk
                                , int &chunkBeingGeneratedCount
                                , Vector3 chunkIndex, Vector3 innerChunkIndex
                                , int lodLevel)
{
    PROFILE_FUNCTION();

    std::vector<std::vector<std::vector<float>>> _noiseForCurChunk(chunkSize + 3, std::vector<std::vector<float>>(chunkSize + 3, std::vector<float>(chunkSize + 3)));
    MakeNoiseForChunk(_noiseForCurChunk, chunkIndex.x, chunkIndex.y, chunkIndex.z, numChunksFullWidth, numChunksFullWidth_Y, chunkSize, scale);

    ConvoluteNoise(_noiseForCurChunk, lodLevel);

    GenMeshCustom2D(_noiseForCurChunk, megaVertPositions, innerChunkIndex, chunkIndex);

    {
        {
            std::lock_guard<std::mutex> lockChunkUpdatedIndex(chunkUpdatedIndexWithVoxelMutex);
            chunkUpdatedIndexWithVoxelMatchedToChunk[megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex)] = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);
        }

        //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
        {
            std::lock_guard<std::mutex> lockChunkGenerated(chunkGeneratedMutex);
            int imaginaryChunkIndex = megaVertPositions.ImaginaryChunkFlatIndexWithoutVoxels(chunkIndex);
            chunkGenerated[imaginaryChunkIndex] = true;
        }

        {
            std::lock_guard<std::mutex> lockChunkBeingGeneratedCount(chunkBeingGeneratedCountMutex);
            chunkBeingGeneratedCount--;
        }
    }
}

static void GenChunkMeshWithNoiseAndSaveToFile(VertexPositions& megaVertPositions
    , std::unordered_map<int, bool>& chunkGenerated
    , std::unordered_map<int, int>& chunkUpdatedIndexWithVoxel
    , int& chunkBeingGeneratedCount
    , Vector3 chunkIndex, Vector3 innerChunkIndex
    , int lodLevel
    , bool saveChunkToFile) {

    PROFILE_FUNCTION();

    GenChunkMeshWithNoise(megaVertPositions, chunkGenerated, chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray, chunkBeingGeneratedCount, chunkIndex, innerChunkIndex, lodLevel);

    if (saveChunkToFile) {

        std::string curChunkSaveFileName = CHUNK_SAVE_STRING(chunkIndex);

        //std::cout << "SAVED: " << curChunkSaveFileName << std::endl;

        std::ofstream os(worldDataDir + curChunkSaveFileName, std::ios::binary);
        cereal::BinaryOutputArchive archive(os);

        int chunkFlatIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        int start = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
        std::span<int> curChunkSlice(megaVertPositions.megaArrayOfAllPositions.begin() + start, totalNumVoxelsPerChunk * NUM_FACES);

        //cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size())

        archive(cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size())
            , megaVertPositions.upEndVoxelPositions[chunkFlatIndexWithoutVoxels]
            , megaVertPositions.downEndVoxelPositions[chunkFlatIndexWithoutVoxels]
            , megaVertPositions.frontEndVoxelPositions[chunkFlatIndexWithoutVoxels]
            , megaVertPositions.backEndVoxelPositions[chunkFlatIndexWithoutVoxels]
            , megaVertPositions.rightEndVoxelPositions[chunkFlatIndexWithoutVoxels]
            , megaVertPositions.leftEndVoxelPositions[chunkFlatIndexWithoutVoxels]);

        //if (chunkIndex.x == 0 && chunkIndex.y == 0 && chunkIndex.z == 0) {

        //    std::cout << megaVertPositions.upEndVoxelPositions[chunkFlatIndexWithoutVoxels] << ", "
        //        << megaVertPositions.downEndVoxelPositions[chunkFlatIndexWithoutVoxels] << ", "
        //        << megaVertPositions.frontEndVoxelPositions[chunkFlatIndexWithoutVoxels] << ", "
        //        << megaVertPositions.backEndVoxelPositions[chunkFlatIndexWithoutVoxels] << ", "
        //        << megaVertPositions.rightEndVoxelPositions[chunkFlatIndexWithoutVoxels] << ", "
        //        << megaVertPositions.leftEndVoxelPositions[chunkFlatIndexWithoutVoxels] << std::endl;
        //}
    }
}

static void ReloadChunkDataFromFile(std::string curChunkFileName
                                    , VertexPositions &megaVertPositions
                                    , std::unordered_map<int, bool>& chunkGenerated
                                    , std::unordered_map<int, int>& chunkUpdatedIndexWithVoxel
                                    , int& chunkBeingGeneratedCount
                                    , Vector3 curChunkIndex, Vector3 innerChunkIndex)
{
    PROFILE_FUNCTION();

    std::ifstream is(curChunkFileName, std::ios::binary);
    cereal::BinaryInputArchive iarchive(is);

    int chunkFlatIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);
    int start = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
    std::span<int> curChunkSlice(megaVertPositions.megaArrayOfAllPositions.begin() + start, totalNumVoxelsPerChunk * NUM_FACES);

    iarchive(cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size())
        , megaVertPositions.upEndVoxelPositions[chunkFlatIndexWithoutVoxels]
        , megaVertPositions.downEndVoxelPositions[chunkFlatIndexWithoutVoxels]
        , megaVertPositions.frontEndVoxelPositions[chunkFlatIndexWithoutVoxels]
        , megaVertPositions.backEndVoxelPositions[chunkFlatIndexWithoutVoxels]
        , megaVertPositions.rightEndVoxelPositions[chunkFlatIndexWithoutVoxels]
        , megaVertPositions.leftEndVoxelPositions[chunkFlatIndexWithoutVoxels]);

    {
        {
            std::lock_guard<std::mutex> lockChunkUpdatedIndex(chunkUpdatedIndexWithVoxelMutex);
            chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray[megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex)] = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);
        }

        //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
        {
            std::lock_guard<std::mutex> lockChunkGenerated(chunkGeneratedMutex);
            int imaginaryChunkIndex = megaVertPositions.ImaginaryChunkFlatIndexWithoutVoxels(curChunkIndex);
            chunkGenerated[imaginaryChunkIndex] = true;
        }

        {
            std::lock_guard<std::mutex> lockChunkBeingGeneratedCount(chunkBeingGeneratedCountMutex);
            chunkBeingGeneratedCount--;
        }
    }

}

const siv::PerlinNoise::seed_type seed = 76554893u;

const siv::PerlinNoise perlin{ seed };

std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommands;
std::unordered_map<int, Vector3> mappedInnerIndexMap;
std::unordered_map<int, bool> innerIndexWhereNewMeshNeedsToBeCalculated;

int main()
{
    Instrumentor::Instance().BeginSession("Profile");

    flecs::world world;

    auto e = world.entity();

    InitWindow(1280, 720, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Texture2D textureLoad = LoadTexture("texture_test_small.png");

    AllChunkVoxelStorage(noise3D, float, numChunksFullWidth, chunkSize + 3);

    std::unordered_map<int, bool> chunkGenerated;

    std::vector<std::future<void>> chunkMeshGenThreads;

    std::vector<float3> chunkPositions;
    VertexPositions megaVertPositions;

    Vector3 cameraChunkIndex = { (int)camera.position.x / chunkSize, (int)camera.position.y / chunkSize, (int)camera.position.z / chunkSize };
    Vector3 oldCameraChunkPosition = cameraChunkIndex;

    int chunkBeingGeneratedCount = 0;
    bool chunksChanged = false;

    GenerativeMesh renderQuad = { 0 };
    PlaneFacingDir(up, renderQuad);
    renderQuad.instanceVBOID = 0;

    int indirectBufferVBO = 0;

    std::vector<Vector3> renderTraversalOrder;

    for (int y = 0; y < numChunksFullWidth_Y; y++)
    {
        Vector3 chunkIndex = Vector3{ (float)(0), (float)(y), (float)0 };
        renderTraversalOrder.push_back(chunkIndex);
        //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
        mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
        innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
    }

    int curLayerNum = 1;
    while (curLayerNum <= numChunksHalfWidth) {

        for (int z = -curLayerNum; z <= curLayerNum; z++) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)(curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
            }

        }

        for (int z = -curLayerNum; z <= curLayerNum; z++) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)(-curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
            }
        }

        for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x++) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
            }
        }

        for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x++) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(-curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
            }
        }
        curLayerNum++;
    }


    // Load lighting instanceShader
    Shader instanceShader = LoadShader(TextFormat("Shaders/lighting_instancing.vert", GLSL_VERSION),
        TextFormat("Shaders/lighting.frag", GLSL_VERSION));
    // Get instanceShader locations
    instanceShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instanceShader, "mvp");
    instanceShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(instanceShader, "viewPos");
    instanceShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instanceShader, "instanceTransform");

    // Set instanceShader value: ambient light level
    //int ambientLoc = GetShaderLocation(instanceShader, "ambient");
    //float ambientValue[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    //SetShaderValue(instanceShader, ambientLoc, ambientValue, SHADER_UNIFORM_VEC4);

    Material instancedMaterial = LoadMaterialDefault();
    instancedMaterial.shader = instanceShader;
    instancedMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = textureLoad;
    
    DisableCursor();

    float randValue = 1.0f;
    int randValueLoc = GetShaderLocation(instanceShader, "switchColours");
    SetShaderValue(instanceShader, randValueLoc, &randValue, SHADER_UNIFORM_FLOAT);

    int lodLevelLoc = GetShaderLocation(instanceShader, "lodScale");
    float lodScale = pow(2, lodLevel);
    SetShaderValue(instanceShader, lodLevelLoc, &lodScale, SHADER_UNIFORM_FLOAT);

    //std::cout << renderTraversalOrder.size() << std::endl;

    while (!WindowShouldClose())
    {

        PROFILE_SCOPE("Game Loop");

        if (IsKeyPressed(KEY_ZERO)) {
            randValue = randValue == 0 ? 1 : 0;
            SetShaderValue(instanceShader, randValueLoc, &randValue, SHADER_UNIFORM_FLOAT);
        }

        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        chunkPositions.clear();
        //chunkPositionsFlattened.clear();
        drawArraysIndirectCommands.clear();

        cameraChunkIndex = { (float)((int)camera.position.x / chunkSize), (float)((int)camera.position.y / chunkSize), (float)((int)camera.position.z / chunkSize) };

        if (oldCameraChunkPosition.x != cameraChunkIndex.x || oldCameraChunkPosition.z != cameraChunkIndex.z) {

            //std::cout << std::endl;

            Vector3 offset = Vector3{ cameraChunkIndex.x - oldCameraChunkPosition.x, 0, cameraChunkIndex.z - oldCameraChunkPosition.z };

            //std::cout << "Offset: " << offset.x << ", " << offset.z << std::endl;
            for (auto& it : mappedInnerIndexMap) {

                //std::cout << "\tOriginal: " << it.second.x << ", " << it.second.z;

                it.second = Vector3{ it.second.x + offset.x, it.second.y, it.second.z + offset.z };

                if (it.second.x > numChunksHalfWidth) {
                    it.second.x = (it.second.x * -1) + 1;
                }

                if (it.second.x < -numChunksHalfWidth) {
                    it.second.x = (it.second.x * -1) - 1;
                }

                if (it.second.z > numChunksHalfWidth) {
                    it.second.z = (it.second.z * -1) + 1;
                }

                if (it.second.z < -numChunksHalfWidth) {
                    it.second.z = (it.second.z * -1) - 1;
                }

                //std::cout << "\tApplied Offset: " << it.second.x << ", " << it.second.z << std::endl;
            }

            //Mark All Chunks That Are New To Be Reaclculated
            for (int i = 0; i < renderTraversalOrder.size(); i++)
            {
                if ((offset.x != 0 && renderTraversalOrder[i].x == offset.x * numChunksHalfWidth) || (offset.z != 0 && renderTraversalOrder[i].z == offset.z * numChunksHalfWidth)) {
                    innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
                }

            }
        }

        if (IsKeyPressed(KEY_NINE)) {
            for (auto& it : mappedInnerIndexMap) {

                std::cout << it.second.x << ", " << it.second.y << ", " << it.second.z << std::endl;
            }
        }


        if (IsKeyPressed(KEY_EIGHT)) {

            std::cout << std::endl;

        }

        if (IsKeyPressed(KEY_COMMA)) {
            lodLevel++;
            if (lodLevel > 5) {
                lodLevel = 5;
            }

            for (int i = 0; i < renderTraversalOrder.size(); i++)
            {
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
            }

            lodScale = pow(2, lodLevel);
            SetShaderValue(instanceShader, lodLevelLoc, &lodScale, SHADER_UNIFORM_FLOAT);
        }

        if (IsKeyPressed(KEY_PERIOD)) {
            lodLevel--;
            if (lodLevel < 0) {
                lodLevel = 0;
            }

            for (int i = 0; i < renderTraversalOrder.size(); i++)
            {
                innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
            }

            lodScale = pow(2, lodLevel);
            SetShaderValue(instanceShader, lodLevelLoc, &lodScale, SHADER_UNIFORM_FLOAT);
        }


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


        BeginDrawing();
        ClearBackground(RAYWHITE);

            BeginMode3D(camera);
            {
                PROFILE_SCOPE("Drawing Chunks");
                for (int i = 0; i < renderTraversalOrder.size(); i++)
                {
                    Vector3 offsetRenderTraversalOrder = mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])];
                    //std::cout << offsetRenderTraversalOrder.x << ", " << offsetRenderTraversalOrder.y << ", " << offsetRenderTraversalOrder.z << std::endl;
                    Vector3 curChunkTraversalIndex = Vector3{ renderTraversalOrder[i].x + cameraChunkIndex.x, renderTraversalOrder[i].y, renderTraversalOrder[i].z + cameraChunkIndex.z };

                    Vector3 oldChunkTraversalIndex = Vector3{ renderTraversalOrder[i].x + oldCameraChunkPosition.x, renderTraversalOrder[i].y, renderTraversalOrder[i].z + oldCameraChunkPosition.z };

                    int renderTraversalIndexFlattened = megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i]);
                    int offsetFlattenedIndex = megaVertPositions.InnerIndexFlattened(offsetRenderTraversalOrder);

                    if (innerIndexWhereNewMeshNeedsToBeCalculated.contains(renderTraversalIndexFlattened) && innerIndexWhereNewMeshNeedsToBeCalculated[renderTraversalIndexFlattened]) {

                        int curPosInChunkStatusMegaArray = megaVertPositions.ImaginaryChunkFlatIndexWithoutVoxels(curChunkTraversalIndex);

                        {
                            std::lock_guard<std::mutex> lock(chunkBeingGeneratedCountMutex);
                            chunkBeingGeneratedCount++;
                        }

                        {
                            std::lock_guard<std::mutex> lock(chunkGeneratedMutex);
                            chunkGenerated[curPosInChunkStatusMegaArray] = false;
                        }


                        //std::cout << "Generating New Chunk: \n\t CurChunkTraversalIndex (" << curChunkTraversalIndex.x << ", " << curChunkTraversalIndex.y << ", " << curChunkTraversalIndex.z
                        //                                        << ")\n\t OffsetRenderTraversalOrder (" 
                        //                                        << offsetRenderTraversalOrder.x << ", " << offsetRenderTraversalOrder.y << ", " << offsetRenderTraversalOrder.z
                        //                                        << ")\n\t CameraChunkIndex (" 
                        //                                        << cameraChunkIndex.x << ", " << cameraChunkIndex.y << ", " << cameraChunkIndex.z
                        //                                        << ")\n\t RenderTraversalOrder ("
                        //                                        << renderTraversalOrder[i].x << ", " << renderTraversalOrder[i].y << ", " << renderTraversalOrder[i].z << ")" << std::endl;

                        std::string curChunkFileName = worldDataDir + CHUNK_SAVE_STRING(curChunkTraversalIndex);
                        if (std::filesystem::exists(curChunkFileName)) {
                            //std::cout << "CHUNK FILE FOUND!!" << std::endl;

                            //std::cout << "LOADED : " << curChunkFileName << std::endl;
                            chunkMeshGenThreads.push_back(std::async(std::launch::async, ReloadChunkDataFromFile
                                , curChunkFileName
                                , std::ref(megaVertPositions)
                                , std::ref(chunkGenerated)
                                , std::ref(chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray)
                                , std::ref(chunkBeingGeneratedCount)
                                , curChunkTraversalIndex
                                , offsetRenderTraversalOrder));


                        }
                        else {

                            //std::string curChunkSaveFileName = CHUNK_SAVE_STRING(oldChunkTraversalIndex);

                            //std::ofstream os(worldDataDir + curChunkSaveFileName, std::ios::binary);
                            //cereal::BinaryOutputArchive archive(os);
                            //int start = megaVertPositions.ChunkTotalFlatIndexWithVoxels(offsetRenderTraversalOrder);
                            //std::span<int> curChunkSlice(megaVertPositions.megaArrayOfAllPositions.begin() + start, totalNumVoxelsPerChunk * NUM_FACES);
                            //archive(cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size()));

                            chunkMeshGenThreads.push_back(std::async(std::launch::async, GenChunkMeshWithNoiseAndSaveToFile
                                , std::ref(megaVertPositions)
                                , std::ref(chunkGenerated)
                                , std::ref(chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray)
                                , std::ref(chunkBeingGeneratedCount)
                                , curChunkTraversalIndex
                                , offsetRenderTraversalOrder
                                , (int)lodLevel
                                , saveChunkToFile));

                            //GenChunkMeshWithNoise(std::ref(megaVertPositions)
                            //    , std::ref(chunkGenerated)
                            //    , std::ref(chunkBeingGeneratedCount)
                            //    , curChunkTraversalIndex
                            //    , offsetRenderTraversalOrder);
                        }


                        chunksChanged = true;
                        innerIndexWhereNewMeshNeedsToBeCalculated[renderTraversalIndexFlattened] = false;
                    }

                    {
                        int curPosInChunkStatusMegaArray = megaVertPositions.ImaginaryChunkFlatIndexWithoutVoxels(curChunkTraversalIndex);
                        int renderChunk = false;

                        {
                            std::lock_guard<std::mutex> lock(chunkGeneratedMutex);
                            renderChunk = chunkGenerated.contains(curPosInChunkStatusMegaArray) && chunkGenerated[curPosInChunkStatusMegaArray];
                        }


                        if (renderChunk) {

                            ReadyIndirectDrawListOfDrawableChunksAndFaces(/*renderTraversalOrder[i]*/
                                offsetRenderTraversalOrder, curChunkTraversalIndex
                                , camera, cameraChunkIndex
                                , instanceShader, instancedMaterial
                                , megaVertPositions, chunkPositions
                                , renderQuad
                                , nearPlane
                                , farPlane
                                , rightPlane
                                , leftPlane
                                , topPlane
                                , bottomPlane);

                            //std::cout << curChunkTraversalIndex.x << ", " << curChunkTraversalIndex.y << ", " <<curChunkTraversalIndex.z << std::endl;
                        }
                    }


                }
                unsigned int chunkPosSSBO = rlLoadShaderBuffer(chunkPositions.size() * sizeof(float3), chunkPositions.data(), RL_DYNAMIC_DRAW);
                rlBindShaderBuffer(chunkPosSSBO, 3);

                //OPTIMISE!!!!
                if ((chunkBeingGeneratedCount == 0 && chunksChanged) || IsKeyPressed(KEY_U)) {
                    rlEnableVertexArray(renderQuad.mesh.vaoId);

                    //renderQuad.instanceVBOID = rlLoadVertexBuffer(megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size() * sizeof(int), true);

                    for (const auto& it : chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray) {
                        int start = it.first;

                        int sizeUp = megaVertPositions.upEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeUp * sizeof(int), start * sizeof(int));
                        start += totalNumVoxelsPerChunk;

                        int sizeDown = megaVertPositions.downEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeDown * sizeof(int), start * sizeof(int));
                        start += totalNumVoxelsPerChunk;

                        int sizeFront = megaVertPositions.frontEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeFront * sizeof(int), start * sizeof(int));
                        start += totalNumVoxelsPerChunk;

                        int sizeBack = megaVertPositions.backEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeBack * sizeof(int), start * sizeof(int));
                        start += totalNumVoxelsPerChunk;

                        int sizeRight = megaVertPositions.rightEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeRight * sizeof(int), start * sizeof(int));
                        start += totalNumVoxelsPerChunk;

                        int sizeLeft = megaVertPositions.leftEndVoxelPositions[it.second];
                        rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + start, sizeLeft * sizeof(int), start * sizeof(int));
                    }

                    //std::cout << megaVertPositions.totalFilled << std::endl;

                    rlEnableVertexAttribute(3);
                    rlSetVertexAttributeI(3, 1, RL_INT, 0, 0, 0);
                    rlSetVertexAttributeDivisor(3, 1);

                    rlDisableVertexBuffer();
                    rlDisableVertexArray();
                    chunksChanged = false;
                    chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray.clear();
                }

                DrawMeshMultiInstancedDrawIndirect(renderQuad, instancedMaterial, megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size(), drawArraysIndirectCommands, drawArraysIndirectCommands.size());
                rlUnloadShaderBuffer(chunkPosSSBO);

            }



            DrawGrid(10, 1.0);

            EndMode3D();
            
            //int numVoxelsPerChunk = chunkSize * 2;
            //Vector3 cameraChunkPos = { (int)camera.position.x / numVoxelsPerChunk, (int)camera.position.y / numVoxelsPerChunk, (int)camera.position.z / numVoxelsPerChunk };
            //Vector3 cameraVoxelPos = { (int)camera.position.x - cameraChunkPos.x, (int)camera.position.y - cameraChunkPos.y, (int)camera.position.z - cameraChunkPos.z };
            //cameraVoxelPos = Vector3SubtractValue(cameraVoxelPos, numVoxelsPerChunk / 2);
            //std::string cameraVoxelPosText = std::to_string(cameraVoxelPos.x) + ", " + std::to_string(cameraVoxelPos.y) + ", " + std::to_string(cameraVoxelPos.z);
            std::string cameraChunkPosText = std::to_string(cameraChunkIndex.x) + ", " + std::to_string(cameraChunkIndex.y) + ", " + std::to_string(cameraChunkIndex.z);
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
        oldCameraChunkPosition = cameraChunkIndex;
    }

    //rlUnloadShaderBuffer(chunkPosSSBO);
    rlUnloadVertexBuffer(indirectBufferVBO);
    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);
    CloseWindow();

    Instrumentor::Instance().EndSession();

    return 0;
}

static void ReadyIndirectDrawListOfDrawableChunksAndFaces(Vector3 innerChunkIndex, Vector3 drawChunkIndex
                                                        , Camera camera, Vector3 cameraChunkIndex
                                                        , Shader instanceShader, Material instancedMaterial
                                                        , VertexPositions &megaVertPositions
                                                        , std::vector<float3> &chunkPositions
                                                        , GenerativeMesh &renderQuad
                                                        , Plane& nearPlane
                                                        , Plane& farPlane
                                                        , Plane& rightPlane
                                                        , Plane& leftPlane
                                                        , Plane& topPlane
                                                        , Plane& bottomPlane)
{
    PROFILE_FUNCTION();

    //std::cout << shouldDrawChunk << std::endl;
    int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    Vector3 drawCurChunkPos = { drawChunkIndex.x * chunkSize, drawChunkIndex.y * chunkSize, drawChunkIndex.z * chunkSize };

    bool cameraInThisChunkWidthAndBreadth = drawChunkIndex.x <= cameraChunkIndex.x || drawChunkIndex.z <= cameraChunkIndex.z;

    Vector3 dirToChunkFromCamera = drawCurChunkPos - camera.position;
    dirToChunkFromCamera = Vector3Normalize(dirToChunkFromCamera);

    bool shouldDrawChunk = ShouldDrawChunk(drawCurChunkPos, camera, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);

    constexpr bool drawAll = true;

    if (shouldDrawChunk) {

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

        if (dotUp < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_UP_INDEX;
            int numInstances = megaVertPositions.upEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //std::cout << "x : " << curChunkPos.x << " y : " << curChunkPos.y << " z : " << curChunkPos.z << std::endl;
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotDown < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_DOWN_INDEX;
            int numInstances = megaVertPositions.downEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotFront < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_FRONT_INDEX;
            int numInstances = megaVertPositions.frontEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotBack < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_BACK_INDEX;
            int numInstances = megaVertPositions.backEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotRight < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_RIGHT_INDEX;
            int numInstances = megaVertPositions.rightEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotLeft < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = curChunkIndexInBigArray + totalNumVoxelsPerChunk * FACE_LEFT_INDEX;
            int numInstances = megaVertPositions.leftEndVoxelPositions[megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex)];
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }
    }
}

static void MakeNoise3D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>> &noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale) {

    PROFILE_FUNCTION();

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

static void ConvolutionSum(std::vector<std::vector<std::vector<float>>>& noiseStorage, int convolutionSize, int convolutionPositionX, int convolutionPositionY, int convolutionPositionZ, int lodLevel) {

    int adjustedConvolutionSize = convolutionSize - 1;
    int convolutionStartX = convolutionPositionX - adjustedConvolutionSize;
    int convolutionEndX = convolutionPositionX;

    int convolutionStartY = convolutionPositionY - adjustedConvolutionSize;
    int convolutionEndY = convolutionPositionY;

    int convolutionStartZ = convolutionPositionZ - adjustedConvolutionSize;
    int convolutionEndZ = convolutionPositionZ;

    int sum = 0;
    for (int x = convolutionStartX; x <= convolutionEndX; x++)
    {
        for (int z = convolutionStartZ; z <= convolutionEndZ; z++)
        {
            for (int y = convolutionStartY; y <= convolutionEndY; y++)
            {
                if (noiseStorage[x][y][z] <= 1) {
                    //std::cout << "\t" << x << ", " << y << ", " << z << std::endl;
                    sum++;
                }
            }
        }
    }
    //(pow(2, lodLevel) * pow(2, lodLevel))
    //noiseStorage[convolutionPositionX][convolutionPositionY][convolutionPositionZ]
    //1
    noiseStorage[convolutionPositionX][convolutionPositionY][convolutionPositionZ] = (sum >= (pow(2, lodLevel) * pow(2, lodLevel))) ? 1 : 2;

}

//Combine chunks into one large LOD?
static void ConvoluteNoise(std::vector<std::vector<std::vector<float>>>& noiseStorage, int lodLevel)
{
    int powerOfTwo = pow(2, lodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;

    int stepSizeForConvolution = powerOfTwo;
    for (int x = startX; x <= endX; x += stepSizeForConvolution)
    {
        for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
        {
            for (int y = startY; y <= endY; y += stepSizeForConvolution)
            {
                //std::cout << x << ", " << y << ", " << z << std::endl;
                ConvolutionSum(noiseStorage, stepSizeForConvolution, x, y, z, lodLevel);
                //std::cout << x << ", " << y << ", " << z << " : " << noise[x][y][z] << std::endl;
            }
        }
    }
}

static std::mutex sameYXZMutex;
static void MakeNoiseForChunk(std::vector<std::vector<std::vector<float>>> &noiseStorage, int chunksX, int chunksY, int chunksZ, int numChunks, int numChunksY, int chunksSize, float scale) {

    PROFILE_FUNCTION();

    int _x, _y, _z = 0;

    for (int x = -1; x <= chunkSize; x++)
    {
        _x = x + (chunksX * chunkSize);

        for (int z = -1; z <= chunkSize; z++)
        {
            _z = z + (chunksZ * chunksSize);

            float noise = perlin.noise2D_01((double)_x * scale, (double)_z * scale);

            int scaledNoise = (int)(noise * chunksSize * numChunksFullWidth_Y);

            for (int y = -1; y <= chunkSize; y++)
            {
                _y = y + chunksY * chunkSize;
                if (_y < scaledNoise) {// This is the position under the noise height.
                    noiseStorage[x + 1][y + 1][z + 1] = 0; // STONE BLOCK
                }
                else if (_y == scaledNoise) {// This is the noise height.
                    noiseStorage[x + 1][y + 1][z + 1] = 1; // DIRT BLOCK
                }
                else if (_y > scaledNoise) {// This is the position above the noise height.
                    noiseStorage[x + 1][y + 1][z + 1] = 2; // AIR BLOCK
                }
            }
        }
    }
}

static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale) {

    PROFILE_FUNCTION();

    for (int chunksX = 0; chunksX < numChunks; chunksX++)
    {
        for (int chunksY = 0; chunksY < numChunksY; chunksY++)
        {
            for (int chunksZ = 0; chunksZ < numChunks; chunksZ++)
            {
                MakeNoiseForChunk(noiseStorage[chunksX][chunksY][chunksZ], chunksX, chunksY, chunksZ, numChunks, numChunksY, chunkSize, scale);
            }
        }
    }

}

static void GenMeshCustom3D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>> &noiseForCurrentChunk)
{
    PROFILE_FUNCTION();

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

//Can be multithreaded to increase performance by doing one thread per face direction.
static void GenMeshCustom2D(std::vector<std::vector<std::vector<float>>> &noiseForCurrentChunk
                                                        , VertexPositions &megaVertPositions
                                                        , Vector3 innerChunkIndex
                                                        , Vector3 chunkIndex)
{
    PROFILE_FUNCTION();

    //std::cout << innerChunkIndex.x << ", " << innerChunkIndex.y << ", " << innerChunkIndex.z << std::endl;

    int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    megaVertPositions.ClearChunkData(innerChunkIndex);

    int powerOfTwo = pow(2, lodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;

    int stepSizeForConvolution = powerOfTwo;
    for (int x = startX; x <= endX; x += stepSizeForConvolution)
    {
        for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
        {
            for (int y = startY; y <= endY; y += stepSizeForConvolution)
            {
                float curNoise = noiseForCurrentChunk[x][y][z];

                //int curVoxelIndex = curChunkIndexInBigArray
                //    + curFace * chunkSize * chunkSize * chunkSize
                //    + (y - 1) * chunkSize * chunkSize
                //    + (x - 1) * chunkSize
                //    + (z - 1);

                int curPosition = PackThreeNumbers(x - stepSizeForConvolution, y - stepSizeForConvolution, z - stepSizeForConvolution);

                if (curNoise == 1 || curNoise == 0) {

                    Vector3 chunkIndexTop = chunkIndex + Vector3{ 0, 1, 0 };
                    Vector3 chunkIndexBottom = chunkIndex + Vector3{ 0, -1, 0 };
                    Vector3 chunkIndexFront = chunkIndex + Vector3{ 0, 0, 1 };
                    Vector3 chunkIndexBack = chunkIndex + Vector3{ 0, 0, -1 };
                    Vector3 chunkIndexRight = chunkIndex + Vector3{ 1, 0, 0 };
                    Vector3 chunkIndexLeft = chunkIndex + Vector3{ -1, 0, 0 };


                    //Optimize using data from large array, check if the number of faces is greater than 0 in that particular direction or something.
                    float curNoiseTop = (y + stepSizeForConvolution <= endY) ? noiseForCurrentChunk[x][y + stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y + 1][z]);

                    if (curNoiseTop == 2) {
                        int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPositionTemp);
                        megaVertPositions.AddUp(curPositionTemp, innerChunkIndex);
                        //std::cout << innerChunkIndex.y << std::endl;
                    }

                    float curNoiseBottom = (y - stepSizeForConvolution >= startY - 1) ? noiseForCurrentChunk[x][y - stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y - 1][z]);

                    if (curNoiseBottom == 2) {
                        int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPositionTemp);
                        megaVertPositions.AddDown(curPositionTemp, innerChunkIndex);
                    }

                    float curNoiseFront = (z + stepSizeForConvolution <= endZ) ? noiseForCurrentChunk[x][y][z + stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z + 1]);

                    if (curNoiseFront == 2) {
                        int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPositionTemp);
                        megaVertPositions.AddFront(curPositionTemp, innerChunkIndex);
                    }

                    float curNoiseBack = (z - stepSizeForConvolution >= startZ - 1) ? noiseForCurrentChunk[x][y][z - stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z - 1]);

                    if (curNoiseBack == 2) {
                        int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPositionTemp);
                        megaVertPositions.AddBack(curPositionTemp, innerChunkIndex);
                    }

                    float curNoiseRight = (x + stepSizeForConvolution <= endX) ? noiseForCurrentChunk[x + stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x + 1][y][z]);

                    if (curNoiseRight == 2) {
                        int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPositionTemp);
                        megaVertPositions.AddRight(curPositionTemp, innerChunkIndex);
                    }

                    float curNoiseLeft = (x - stepSizeForConvolution >= startX - 1) ? noiseForCurrentChunk[x - stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x - 1][y][z]);

                    if (curNoiseLeft == 2) {
                        int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curPositionTemp);
                        megaVertPositions.AddLeft(curPositionTemp, innerChunkIndex);
                    }
                }
            }
        }
    }
}

void PlaneFacingDir(Vector3 dir, GenerativeMesh &curMesh) {

    PROFILE_FUNCTION();

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

bool ShouldDrawChunk(Vector3 curChunkPos, Camera camera
    , Plane& nearPlane
    , Plane& farPlane
    , Plane& rightPlane
    , Plane& leftPlane
    , Plane& topPlane
    , Plane& bottomPlane) {


    PROFILE_FUNCTION();

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