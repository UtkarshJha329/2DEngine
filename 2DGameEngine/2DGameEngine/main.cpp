
#include <iostream>
#include <chrono>
#include <future>
#include <filesystem>

#include <stdint.h>

#include "flecs/flecs.h"

#define GLSL_VERSION            430
#define GRAPHICS_API_OPENGL_43
#define RLGL_RENDER_TEXTURES_HINT
#include "raylib/raylib.h"

#include "Plane.h"
#include "PerlinNoise.hpp"

#include "FastNoise/FastNoise.h"

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
static void MakeNoiseForChunk(std::vector<std::vector<std::vector<float>>>& noiseStorage, FastNoise::SmartNode<FastNoise::FractalFBm>& fnFractal, int chunksX, int chunksY, int chunksZ, int numChunks, int numChunksY, int chunksSize, int sideVoxelsToConsider, float scale);
static void MakeNoiseForChunkLOD(std::vector<std::vector<std::vector<float>>>& noiseStorage, FastNoise::SmartNode<FastNoise::FractalFBm>& fnFractal, int chunksX, int chunksY, int chunksZ, int numChunks, int numChunksY, int chunksSize, int sideVoxelsToConsider, float scale, int curLodLevel);
static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale);

static void ConvolutionSum(std::vector<std::vector<std::vector<float>>>& noiseStorage, int convolutionSize, int convolutionPositionX, int convolutionPositionY, int convolutionPositionZ, int lodLevel);
static void ConvoluteNoise(std::vector<std::vector<std::vector<float>>>& noiseStorage, int lodLevel);

static void GenMeshCustom3D(std::unordered_map<BlockFaceDirection, std::vector<int>>& transformOfVerticesOfFaceInParticularDir
    , std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk);
static void GenMeshCustom2D(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , std::vector<int>& chunkMeshData
    , ChunkFacesMetadata& chunkFacesMetadata
    , VertexPositions &megaVertPositions
    , Vector3 rescopedChunkIndex
    , Vector3 chunkIndex
    , int curLodLevel);

static void GenMeshCustom2DLOD(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , std::vector<int>& chunkMeshData
    , ChunkFacesMetadata& chunkFacesMetadata
    , VertexPositions &megaVertPositions
    , Vector3 rescopedChunkIndex
    , Vector3 chunkIndex
    , int curLodLevel);

static void GreedyMesh2D(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , std::vector<std::vector<std::vector<float>>>& scaleForEachVoxelInChunk
    , std::vector<int>& chunkMeshData
    , ChunkFacesMetadata& chunkFacesMetadata
    , VertexPositions& megaVertPositions
    , Vector3 rescopedChunkIndex
    , Vector3 chunkIndex
    , int curLodLevel);


void PlaneFacingDir(Vector3 dir, GenerativeMesh & curMesh);
void PlaneFacingDirTriangle(Vector3 dir, GenerativeMesh& curMesh);

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
                                                        , Plane& bottomPlane
                                                        , int& numChunksDrawn
                                                        , int& numChunksDrawnWithoutFrustum);

static std::vector<int> chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray;

static std::mutex chunkGeneratedMutex;
static std::mutex chunkBeingGeneratedCountMutex;
static std::mutex chunkUpdatedIndexWithVoxelMutex;
static std::mutex chunkMappingAllocatingMutex;
static void GenChunkMeshWithNoise(VertexPositions &megaVertPositions
                                , std::unordered_map<int, bool> &chunkGenerated
                                , std::vector<int> &chunkUpdatedIndexWithVoxelMatchedToChunk
                                , int &chunkBeingGeneratedCount
                                , Vector3 chunkIndex, Vector3 innerChunkIndex
                                , int curLodLevel
                                , bool reclaimPreviousMemory)
{
    PROFILE_FUNCTION();

    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();

    fnFractal->SetSource(fnSimplex);
    fnFractal->SetOctaveCount(5);

    int extraVoxelsToCompute = 2 * (pow(2, curLodLevel) + 1);
    //int extraVoxelsToCompute = 2 + 1;

    std::vector<std::vector<std::vector<float>>> _noiseForCurChunk(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    MakeNoiseForChunk(_noiseForCurChunk, fnFractal, chunkIndex.x, chunkIndex.y, chunkIndex.z, numChunksFullWidth, numChunksFullWidth_Y, chunkSize, extraVoxelsToCompute, scale);
    //MakeNoiseForChunkLOD(_noiseForCurChunk, fnFractal, chunkIndex.x, chunkIndex.y, chunkIndex.z, numChunksFullWidth, numChunksFullWidth_Y, chunkSize, extraVoxelsToCompute, scale, curLodLevel);

    ConvoluteNoise(_noiseForCurChunk, curLodLevel);

    std::vector<int> chunkMeshData;
    ChunkFacesMetadata chunkFacesMetadata;
    //GenMeshCustom2D(_noiseForCurChunk, chunkMeshData, chunkFacesMetadata, megaVertPositions, innerChunkIndex, chunkIndex, curLodLevel);
    //GenMeshCustom2DLOD(_noiseForCurChunk, chunkMeshData, chunkFacesMetadata, megaVertPositions, innerChunkIndex, chunkIndex, curLodLevel);

    std::vector<std::vector<std::vector<float>>> _scaleForEachVoxelInChunk(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    GreedyMesh2D(_noiseForCurChunk, _scaleForEachVoxelInChunk, chunkMeshData, chunkFacesMetadata, megaVertPositions, innerChunkIndex, chunkIndex, curLodLevel);

    {
        {
            std::lock_guard<std::mutex> lockChunkMappingAndAllocatingMutex(chunkMappingAllocatingMutex);

            if (reclaimPreviousMemory) {
                megaVertPositions.ClearChunkData(innerChunkIndex);
            }

            for (int i = 0; i < NUM_FACES; i++)
            {
                megaVertPositions.CopyDataToMegaArray(megaVertPositions.megaArrayOfAllPositions/*, megaVertPositions.nextToFill*//*megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + (i * totalNumVoxelsPerChunkWorstCase)*/
                                                    , chunkMeshData, i * totalNumVoxelsPerChunkWorstCase, chunkFacesMetadata.GetSizeOfFaceDirPositions(i)
                                                    , chunkFacesMetadata.GetAppropriateStartIndexBasedOnFaceDir(i));

            }


            megaVertPositions.MapChunkMemoryToBigArray(innerChunkIndex, chunkFacesMetadata);

        }

        {
            std::lock_guard<std::mutex> lockChunkUpdatedIndex(chunkUpdatedIndexWithVoxelMutex);
            chunkUpdatedIndexWithVoxelMatchedToChunk.push_back(megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex));
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
    , std::vector<int>& chunkUpdatedIndexWithVoxel
    , int& chunkBeingGeneratedCount
    , Vector3 chunkIndex, Vector3 innerChunkIndex
    , int lodLevel
    , bool reclaimPreviousMemory
    , bool saveChunkToFile) {

    PROFILE_FUNCTION();

    GenChunkMeshWithNoise(megaVertPositions, chunkGenerated, chunkUpdatedIndexWithVoxel, chunkBeingGeneratedCount, chunkIndex, innerChunkIndex, lodLevel, reclaimPreviousMemory);

    if (saveChunkToFile) {

        {
            std::string curChunkFileMetadaName = CHUNK_METADATA_SAVE_STRING(chunkIndex);

            std::ofstream os(worldDataDir + curChunkFileMetadaName, std::ios::binary);
            cereal::BinaryOutputArchive archive(os);

            int chunkFlatIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(chunkIndex);
            archive(megaVertPositions.upFacesMetadata[chunkFlatIndexWithoutVoxels].size
                , megaVertPositions.downFacesMetadata[chunkFlatIndexWithoutVoxels].size
                , megaVertPositions.frontFacesMetadata[chunkFlatIndexWithoutVoxels].size
                , megaVertPositions.backFacesMetadata[chunkFlatIndexWithoutVoxels].size
                , megaVertPositions.rightFacesMetadata[chunkFlatIndexWithoutVoxels].size
                , megaVertPositions.leftFacesMetadata[chunkFlatIndexWithoutVoxels].size);
        }

        std::string curChunkSaveFileName = CHUNK_SAVE_STRING(chunkIndex);

        std::ofstream os(worldDataDir + curChunkSaveFileName, std::ios::binary);
        cereal::BinaryOutputArchive archive(os);

        std::span<int> upFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_UP_INDEX);
        std::span<int> downFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_DOWN_INDEX);
        std::span<int> frontFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_FRONT_INDEX);
        std::span<int> backFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_BACK_INDEX);
        std::span<int> rightFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_RIGHT_INDEX);
        std::span<int> leftFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(chunkIndex, FACE_LEFT_INDEX);

        archive(
            cereal::binary_data(upFacePositions.data(), sizeof(int) * upFacePositions.size())
            , cereal::binary_data(downFacePositions.data(), sizeof(int) * downFacePositions.size())
            , cereal::binary_data(frontFacePositions.data(), sizeof(int) * frontFacePositions.size())
            , cereal::binary_data(backFacePositions.data(), sizeof(int) * backFacePositions.size())
            , cereal::binary_data(rightFacePositions.data(), sizeof(int) * rightFacePositions.size())
            , cereal::binary_data(leftFacePositions.data(), sizeof(int) * leftFacePositions.size())
        );

    }
}

static void ReloadChunkDataFromFile(std::string curChunkFileName
                                    , VertexPositions &megaVertPositions
                                    , std::unordered_map<int, bool>& chunkGenerated
                                    , std::vector<int>& chunkUpdatedIndexWithVoxel
                                    , int& chunkBeingGeneratedCount
                                    , Vector3 curChunkIndex, Vector3 innerChunkIndex)
{
    PROFILE_FUNCTION();

    {
        std::string curChunkFileMetadaName = CHUNK_METADATA_SAVE_STRING(curChunkIndex);

        std::ifstream is(worldDataDir + curChunkFileMetadaName, std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);

        int chunkFlatIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(curChunkIndex);
        iarchive(megaVertPositions.upFacesMetadata[chunkFlatIndexWithoutVoxels].size
            , megaVertPositions.downFacesMetadata[chunkFlatIndexWithoutVoxels].size
            , megaVertPositions.frontFacesMetadata[chunkFlatIndexWithoutVoxels].size
            , megaVertPositions.backFacesMetadata[chunkFlatIndexWithoutVoxels].size
            , megaVertPositions.rightFacesMetadata[chunkFlatIndexWithoutVoxels].size
            , megaVertPositions.leftFacesMetadata[chunkFlatIndexWithoutVoxels].size);
    }

    {
        std::string curChunkSaveFileName = CHUNK_SAVE_STRING(curChunkIndex);

        std::ifstream is(worldDataDir + curChunkSaveFileName, std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);

        std::span<int> upFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_UP_INDEX);
        std::span<int> downFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_DOWN_INDEX);
        std::span<int> frontFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_FRONT_INDEX);
        std::span<int> backFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_BACK_INDEX);
        std::span<int> rightFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_RIGHT_INDEX);
        std::span<int> leftFacePositions = megaVertPositions.GetCurChunkCurDirVoxelData(curChunkIndex, FACE_LEFT_INDEX);

        iarchive(
            cereal::binary_data(upFacePositions.data(), sizeof(int) * upFacePositions.size())
            , cereal::binary_data(downFacePositions.data(), sizeof(int) * downFacePositions.size())
            , cereal::binary_data(frontFacePositions.data(), sizeof(int) * frontFacePositions.size())
            , cereal::binary_data(backFacePositions.data(), sizeof(int) * backFacePositions.size())
            , cereal::binary_data(rightFacePositions.data(), sizeof(int) * rightFacePositions.size())
            , cereal::binary_data(leftFacePositions.data(), sizeof(int) * leftFacePositions.size())
        );
    }

    {
        {
            std::lock_guard<std::mutex> lockChunkUpdatedIndex(chunkUpdatedIndexWithVoxelMutex);
            chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray.push_back(megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex));
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

bool LODBorderMesh(Vector3 relativePosition) {

    PROFILE_FUNCTION();

    int dist = (int)(Vector3Length(relativePosition));

    return dist == lodDistance1.y
        || dist == lodDistance2.x || dist == lodDistance2.y
        || dist == lodDistance3.x || dist == lodDistance3.y
        || dist == lodDistance4.x || dist == lodDistance4.y
        || dist == lodDistance5.x || dist == lodDistance5.y;
}

struct SecondRenderTexture : public RenderTexture {
    Texture secondColourTexture;
    Texture depthColourTexture;
};

SecondRenderTexture LoadRenderTextureDepthTex(int width, int height)
{
    PROFILE_FUNCTION();

    SecondRenderTexture target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Create color texture (default to RGBA)
        target.secondColourTexture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.secondColourTexture.width = width;
        target.secondColourTexture.height = height;
        target.secondColourTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.secondColourTexture.mipmaps = 1;

        target.depthColourTexture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.depthColourTexture.width = width;
        target.depthColourTexture.height = height;
        target.depthColourTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.depthColourTexture.mipmaps = 1;

        rlActiveDrawBuffers(3);

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.secondColourTexture.id, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depthColourTexture.id, RL_ATTACHMENT_COLOR_CHANNEL2, RL_ATTACHMENT_TEXTURE2D, 0);

        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload render texture from GPU memory (VRAM)
void UnloadRenderTextureDepthTex(SecondRenderTexture target)
{
    PROFILE_FUNCTION();

    if (target.id > 0)
    {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.secondColourTexture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}

const siv::PerlinNoise::seed_type seed = 76554893u;

const siv::PerlinNoise perlin{ seed };

std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommands;
std::unordered_map<int, Vector3> mappedInnerIndexMap;
//std::unordered_map<int, bool> innerIndexWhereNewMeshNeedsToBeCalculated;
std::vector<Vector3> chunksWhereNewMeshNeedsToBeCalculated;

const int screenWidth = 1280;
const int screenHeight = 720;

int main()
{
    PROFILE_FUNCTION();

    Instrumentor::Instance().BeginSession("Profile");

    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();

    fnFractal->SetSource(fnSimplex);
    fnFractal->SetOctaveCount(5);

    flecs::world world;

    auto e = world.entity();

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 2.0f * 32.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    rlSetClipPlanes(0.1f, farPlaneDistance);

    Texture2D textureLoad = LoadTexture("texture_test_small.png");
    Texture2D textureLoad2 = LoadTexture("texture_test.png");
    SecondRenderTexture target = LoadRenderTextureDepthTex(screenWidth, screenHeight);

    std::unordered_map<int, bool> chunkGenerated;

    std::vector<std::future<void>> chunkMeshGenThreads;

    std::vector<float3> chunkPositions;
    std::vector<float3> chunksGridCoordinates;
    VertexPositions megaVertPositions;

    std::vector<int> chunkVisibility(totalNumChunks, 0);

    std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommandsGPU(totalNumChunks * NUM_FACES, { 0, 0, 0, 0 });
    std::vector<float3> drawArraysIndirectChunkPositions(totalNumChunks * NUM_FACES, { 0, 0, 0});
    int nextIndirectdrawCommandIndexBufferValue = -1;

    for (int i = 0; i < chunkVisibility.size(); i++)
    {
        chunkVisibility[i] = 1;
    }

    Vector3 cameraChunkIndex = { (int)camera.position.x / chunkSize, (int)camera.position.y / chunkSize, (int)camera.position.z / chunkSize };
    Vector3 oldCameraChunkPosition = cameraChunkIndex;
    Vector3 oldCameraPos = camera.position;

    int chunkBeingGeneratedCount = 0;
    bool chunksChanged = false;

    GenerativeMesh renderQuad = { 0 };
    PlaneFacingDir(up, renderQuad);
    //PlaneFacingDirTriangle(up, renderQuad);
    renderQuad.instanceVBOID = 0;

    GenerativeMesh cullingRenderQuad = { 0 };
    PlaneFacingDir(up, cullingRenderQuad);
    cullingRenderQuad.instanceVBOID = 0;

    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int quadEBO = 0;

    //float vertices[] = {
    //    // Positions         Texcoords
    //   -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,
    //   -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
    //    1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
    //    1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
    //};

    //unsigned short indices[] = {
    //    0, 1, 2,
    //    1, 3, 2
    //};

    float vertices[] = {
        // positions         // texture coords
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // top right
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f  // top left 
    };
    
    unsigned short indices[] = {
        3, 1, 0, // first triangle
        3, 2, 1  // second triangle
    };

    // Gen VAO to contain VBO
    quadVAO = rlLoadVertexArray();
    rlEnableVertexArray(quadVAO);

    // Gen and fill vertex buffer (VBO)
    quadVBO = rlLoadVertexBuffer(&vertices, sizeof(vertices), false);
    quadEBO = rlLoadVertexBufferElement(&indices, sizeof(indices), false);

    // Bind vertex attributes (position, texcoords)
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 5 * sizeof(float), 0);
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(1, 2, RL_FLOAT, 0, 5 * sizeof(float), (3 * sizeof(float)));
    rlEnableVertexAttribute(1);

    rlEnableVertexArray(0);


    int indirectBufferVBO = 0;

    std::vector<Vector3> renderTraversalOrder;

    for (int y = 0; y < numChunksFullWidth_Y; y++)
    {
        Vector3 chunkIndex = Vector3{ (float)(0), (float)(y), (float)0 };
        renderTraversalOrder.push_back(chunkIndex);
        //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
        mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
        //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
        chunksWhereNewMeshNeedsToBeCalculated.push_back(chunkIndex);
        chunksGridCoordinates.push_back(float3{ 0, (float)y, 0 });
    }

    int curLayerNum = 1;
    while (curLayerNum <= numChunksHalfWidth) {

        int stepSize = pow(2, LODLevel);
        stepSize = 1;

        for (int z = -curLayerNum; z <= curLayerNum; z += stepSize) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)(curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
                chunksWhereNewMeshNeedsToBeCalculated.push_back(chunkIndex);
                chunksGridCoordinates.push_back(float3{ (float)curLayerNum, (float)y, (float)z });
            }

        }

        for (int z = -curLayerNum; z <= curLayerNum; z += stepSize) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)(-curLayerNum), (float)(y), (float)z };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
                chunksWhereNewMeshNeedsToBeCalculated.push_back(chunkIndex);
                chunksGridCoordinates.push_back(float3{ (float)-curLayerNum, (float)y, (float)z });
            }
        }

        for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x += stepSize) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
                chunksWhereNewMeshNeedsToBeCalculated.push_back(chunkIndex);
                chunksGridCoordinates.push_back(float3{ (float)x, (float)y, (float)curLayerNum });
            }
        }

        for (int x = -curLayerNum + 1; x <= curLayerNum - 1; x += stepSize) {
            for (int y = 0; y < numChunksFullWidth_Y; y++)
            {
                Vector3 chunkIndex = Vector3{ (float)x, (float)(y), (float)(-curLayerNum) };
                renderTraversalOrder.push_back(chunkIndex);
                //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
                mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
                //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
                chunksWhereNewMeshNeedsToBeCalculated.push_back(chunkIndex);
                chunksGridCoordinates.push_back(float3{ (float)x, (float)y, (float)-curLayerNum });

            }
        }
        curLayerNum++;
    }

    char* frustumAndFaceCullingComputeCode = LoadFileText("Shaders/FrustumAndFaceCulling.glsl");
    unsigned int frustumAndFaceCullingComputeShader = rlCompileShader(frustumAndFaceCullingComputeCode, RL_COMPUTE_SHADER);
    unsigned int frustumAndFaceCullingComputeProgram = rlLoadComputeShaderProgram(frustumAndFaceCullingComputeShader);
    UnloadFileText(frustumAndFaceCullingComputeCode);

    char* resetChunkVisibilityComputeCode = LoadFileText("Shaders/ResetChunkVisibility.glsl");
    unsigned int resetChunkVisibilityComputeShader = rlCompileShader(resetChunkVisibilityComputeCode, RL_COMPUTE_SHADER);
    unsigned int resetChunkVisibilityComputeProgram = rlLoadComputeShaderProgram(resetChunkVisibilityComputeShader);
    UnloadFileText(resetChunkVisibilityComputeCode);

    const unsigned int computeTextureWidth = 512;
    const unsigned int computeTextureHeight = 512;

    Image randImage = GenImageColor(computeTextureWidth, computeTextureHeight, DARKGREEN);
    ImageFormat(&randImage, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
    Texture2D textureTestingCompute = LoadTextureFromImage(randImage);

    // Load lighting instanceShader
    Shader instanceShader = LoadShader(TextFormat("Shaders/lighting_instancing.vert", GLSL_VERSION),
        TextFormat("Shaders/lighting.frag", GLSL_VERSION));
    // Get instanceShader locations
    instanceShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instanceShader, "mvp");
    instanceShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(instanceShader, "viewPos");
    instanceShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instanceShader, "instanceTransform");

    Shader simpleShader = LoadShader(TextFormat("Shaders/SimpleShader.vert", GLSL_VERSION),
        TextFormat("Shaders/SimpleShader.frag", GLSL_VERSION));


    int textureSlot = 0;
    int textureLoc = GetShaderLocation(simpleShader, "ourTexture");
    rlSetUniform(textureLoc, &textureSlot, SHADER_UNIFORM_INT, 1);

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
    float lodScale = pow(2, LODLevel);
    SetShaderValue(instanceShader, lodLevelLoc, &lodScale, SHADER_UNIFORM_FLOAT);

    int numChunksPerLodLoc = GetShaderLocation(instanceShader, "numChunksPerLOD");
    float numChunksPerLod = lodLevelOffset;
    SetShaderValue(instanceShader, numChunksPerLodLoc, &numChunksPerLod, SHADER_UNIFORM_FLOAT);

    int numChunksLoc = GetShaderLocation(instanceShader, "halfNumChunksWidth");
    SetShaderValue(instanceShader, numChunksLoc, &numChunksHalfWidth, SHADER_UNIFORM_INT);

    int chunkSizeLoc = GetShaderLocation(instanceShader, "chunkSize");
    SetShaderValue(instanceShader, chunkSizeLoc, &chunkSize, SHADER_UNIFORM_INT);

    int renderAllLoc = GetShaderLocation(instanceShader, "renderAll");
    float renderAllValue = 1;
    SetShaderValue(instanceShader, renderAllLoc, &renderAllValue, SHADER_UNIFORM_FLOAT);

    int cameraPosLoc = GetShaderLocation(instanceShader, "cameraPos");
    float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(instanceShader, cameraPosLoc, cameraPos, SHADER_UNIFORM_VEC3);

    Image chunksIDImage = LoadImageFromTexture(target.secondColourTexture);
    Image chunksDepthImage = LoadImageFromTexture(target.depthColourTexture);

    Shader cullingShader = LoadShader(TextFormat("Shaders/SimpleShaderCullingTest.vert", GLSL_VERSION),
        TextFormat("Shaders/SimpleShaderCullingTest.frag", GLSL_VERSION));
    //cullingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(cullingShader, "viewPosition");

    SecondRenderTexture rd2D = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    Material cullingRenderMaterial = LoadMaterialDefault();
    cullingRenderMaterial.shader = cullingShader;

    const int bindDepthTextureAtPosition = 0;
    rlEnableShader(cullingShader.id);
        int textureLocation = rlGetLocationUniform(cullingShader.id, "depthValueTexture");
        rlSetUniform(textureLocation, &bindDepthTextureAtPosition, RL_SHADER_UNIFORM_SAMPLER2D, 1);
    rlDisableShader();

    int cutOffDepthLoc = GetShaderLocation(cullingShader, "cutOffDepth");
    float cutOffDepthValue = 0.25f;
    SetShaderValue(cullingShader, cutOffDepthLoc, &cutOffDepthValue, SHADER_UNIFORM_FLOAT);

    int chunkSizeInCullingShaderLoc = GetShaderLocation(cullingShader, "chunkSize");
    SetShaderValue(cullingShader, chunkSizeInCullingShaderLoc, &chunkSize, SHADER_UNIFORM_INT);

    int numChunksHalfWidthLocInCullingShader = GetShaderLocation(cullingShader, "numChunksHalfWidth");
    SetShaderValue(cullingShader, numChunksHalfWidthLocInCullingShader, &numChunksHalfWidth, SHADER_UNIFORM_INT);

    int cameraPosLocInCullingShader = GetShaderLocation(cullingShader, "cameraPos");
    SetShaderValue(cullingShader, cameraPosLocInCullingShader, cameraPos, SHADER_UNIFORM_VEC3);


    int numChunkSizeLocInComputeShader = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "chunkSize");
    int numChunksHalfWidthLocInComputeShader = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "numChunksHalfWidth");

    int numChunkSizeLocInChunkVisibilityResetComputeShader = rlGetLocationUniform(resetChunkVisibilityComputeProgram, "chunkSize");
    int numChunksHalfWidthLocInChunkVisibilityResetComputeShader = rlGetLocationUniform(resetChunkVisibilityComputeProgram, "numChunksHalfWidth");

    rlEnableShader(frustumAndFaceCullingComputeProgram);
    rlSetUniform(numChunkSizeLocInComputeShader, &chunkSize, SHADER_UNIFORM_INT, 1);
    rlSetUniform(numChunksHalfWidthLocInComputeShader, &numChunksHalfWidth, SHADER_UNIFORM_INT, 1);
    rlDisableShader();

    rlEnableShader(resetChunkVisibilityComputeProgram);
    rlSetUniform(numChunkSizeLocInChunkVisibilityResetComputeShader, &chunkSize, SHADER_UNIFORM_INT, 1);
    rlSetUniform(numChunksHalfWidthLocInChunkVisibilityResetComputeShader, &numChunksHalfWidth, SHADER_UNIFORM_INT, 1);
    rlDisableShader();

    unsigned int chunksGridPosSSBO = rlLoadShaderBuffer(chunksGridCoordinates.size() * sizeof(float3), chunksGridCoordinates.data(), RL_DYNAMIC_DRAW);

    Image initialCullingImage = GenImageColor(screenWidth, screenHeight, WHITE);
    ImageFormat(&initialCullingImage, target.depthColourTexture.format);
    UpdateTexture(target.depthColourTexture, initialCullingImage.data);

    std::vector<int> megaArrayOfAllPositions2;
    std::vector<int> startPositions2;
    std::vector<int> sizes2;
    std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommands2;

    for (int i = 0; i < NUM_FACES; i++)
    {
        startPositions2.push_back(megaArrayOfAllPositions2.size());
        for (int y = 0; y < numChunksFullWidth_Y; y++)
        {
            for (int x = 0; x < numChunksFullWidth; x++)
            {
                for (int z = 0; z < numChunksFullWidth; z++)
                {
                    int largeChunkCentrePos = x << 16;
                    largeChunkCentrePos = largeChunkCentrePos | y << 8;
                    largeChunkCentrePos = largeChunkCentrePos | z;
                    largeChunkCentrePos = largeChunkCentrePos | (i << 24);
                    largeChunkCentrePos = largeChunkCentrePos | (1 << 27);

                    //std::cout << largeChunkCentrePos << std::endl;

                    megaArrayOfAllPositions2.push_back(largeChunkCentrePos);
                }
            }
        }
        sizes2.push_back(megaArrayOfAllPositions2.size());
    }

    for (int i = 0; i < startPositions2.size(); i++)
    {
        DrawArraysIndirectCommand curCommand = { 4, sizes2[i], 0, startPositions2[i] };
        drawArraysIndirectCommands2.push_back(curCommand);
    }

    renderQuad.upFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.upFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.upFacesMetadata.data(), RL_DYNAMIC_DRAW);
    renderQuad.downFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.downFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.downFacesMetadata.data(), RL_DYNAMIC_DRAW);
    renderQuad.frontFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.frontFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.frontFacesMetadata.data(), RL_DYNAMIC_DRAW);
    renderQuad.backFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.backFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.backFacesMetadata.data(), RL_DYNAMIC_DRAW);
    renderQuad.rightFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.rightFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.rightFacesMetadata.data(), RL_DYNAMIC_DRAW);
    renderQuad.leftFacesMetadaBufferID = rlLoadShaderBuffer(megaVertPositions.leftFacesMetadata.size() * sizeof(ChunkFacePositionMetaData), megaVertPositions.leftFacesMetadata.data(), RL_DYNAMIC_DRAW);

    renderQuad.commandsLengthBOID = rlLoadShaderBuffer(sizeof(int), &nextIndirectdrawCommandIndexBufferValue, RL_DYNAMIC_DRAW);

    renderQuad.commandsBufferVBOID = rlLoadShaderBuffer(drawArraysIndirectCommandsGPU.size() * sizeof(DrawArraysIndirectCommand), drawArraysIndirectCommandsGPU.data(), RL_DYNAMIC_DRAW);
    renderQuad.chunkPositionsVBOID = rlLoadShaderBuffer(drawArraysIndirectChunkPositions.size() * sizeof(float3), drawArraysIndirectChunkPositions.data(), RL_DYNAMIC_DRAW);

    int chunkVisibilityValue = 0;
    int chunkVisibilityValueBuffer = rlLoadShaderBuffer(1 * sizeof(int), &chunkVisibilityValue, RL_DYNAMIC_DRAW);

    unsigned int chunkVisibilitySSBO = rlLoadShaderBuffer(chunkVisibility.size() * sizeof(int), chunkVisibility.data(), RL_DYNAMIC_DRAW);
    int frameCounter = -1;
    int numChunksDrawn = 0;
    int numChunksDrawnWithoutFrustum = 0;

    int cullingCommandsLengthValue = drawArraysIndirectCommands2.size();
    cullingRenderQuad.commandsLengthBOID = rlLoadShaderBuffer(1 * sizeof(int), &cullingCommandsLengthValue, RL_STATIC_READ);
    cullingRenderQuad.commandsBufferVBOID = rlLoadShaderBuffer(drawArraysIndirectCommands2.size() * sizeof(DrawArraysIndirectCommand), drawArraysIndirectCommands2.data(), RL_STATIC_READ);

    while (!WindowShouldClose())
    {

        PROFILE_SCOPE("Game Loop");

        //PollInputEvents();              // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

        frameCounter++;

        //std::cout << GetMousePosition().x << ", " << GetMousePosition().y << std::endl;

        if (IsKeyPressed(KEY_ONE)) {
            shouldPerformOcclusionCulling = !shouldPerformOcclusionCulling;
            occlusionCullingStateValue = (float)shouldPerformOcclusionCulling;
        }

        if (IsKeyPressed(KEY_FIVE)) {

            chunksIDImage = LoadImageFromTexture(target.secondColourTexture);
            chunksDepthImage = LoadImageFromTexture(target.depthColourTexture);

            Color valueAtCoord = GetImageColor(chunksIDImage, screenWidth / 2, screenHeight / 2);
            float depth = GetImageColor(chunksDepthImage, screenWidth / 2, screenHeight / 2).r;
            //int x = floor(((1 - (valueAtCoord.r / 255.0f))) * numChunksHalfWidth);
            //int y = floor(((1 - (valueAtCoord.g / 255.0f))) * numChunksHalfWidth);
            //int z = floor(((1 - (valueAtCoord.b / 255.0f))) * numChunksHalfWidth);

            int x = round(((1 - (valueAtCoord.r / 255.0f))) * numChunksFullWidth);
            int y = round(((1 - (valueAtCoord.g / 255.0f))) * numChunksFullWidth_Y);
            int z = round(((1 - (valueAtCoord.b / 255.0f))) * numChunksFullWidth);

            x -= numChunksHalfWidth;
            z -= numChunksHalfWidth;

            std::cout << std::to_string(depth) << " : "
                << std::to_string(x) << ", "
                << std::to_string(y) << ", "
                << std::to_string(z) << std::endl;
        }

        cameraPos[0] = camera.position.x;
        cameraPos[1] = camera.position.y;
        cameraPos[2] = camera.position.z;
        SetShaderValue(instanceShader, cameraPosLoc, cameraPos, SHADER_UNIFORM_VEC3);

        if (IsKeyPressed(KEY_ZERO)) {
            randValue++;
            randValue = randValue > maxRandValue ? 0 : randValue;
            SetShaderValue(instanceShader, randValueLoc, &randValue, SHADER_UNIFORM_FLOAT);
        }

        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        chunkPositions.clear();
        drawArraysIndirectCommands.clear();

        cameraChunkIndex = { (float)((int)camera.position.x / chunkSize), (float)((int)camera.position.y / chunkSize), (float)((int)camera.position.z / chunkSize) };

        if (oldCameraChunkPosition.x != cameraChunkIndex.x || oldCameraChunkPosition.z != cameraChunkIndex.z) {

            //std::cout << "Detected change in camera chunk position." << std::endl;
            Vector3 offset = Vector3{ cameraChunkIndex.x - oldCameraChunkPosition.x, 0, cameraChunkIndex.z - oldCameraChunkPosition.z };

            for (auto& it : mappedInnerIndexMap) {

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
            }

            //Mark All Chunks That Are New To Be Reaclculated
            for (int i = 0; i < renderTraversalOrder.size(); i++)
            {
                if ((offset.x != 0 && renderTraversalOrder[i].x == offset.x * numChunksHalfWidth) || (offset.z != 0 && renderTraversalOrder[i].z == offset.z * numChunksHalfWidth)) {
                    //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
                    chunksWhereNewMeshNeedsToBeCalculated.push_back(renderTraversalOrder[i]);
                }

                if (LODBorderMesh(renderTraversalOrder[i]))
                {
                    //std::cout << "NEED TO RECALCULATE" << std::endl;
                    //innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
                    chunksWhereNewMeshNeedsToBeCalculated.push_back(renderTraversalOrder[i]);
                }
            }
        }

        {
            if (shouldPerformOcclusionCulling && false)
            {
                PROFILE_SCOPE("GPU OCCLUSION CULLING.");

                //chunkVisibilityValue = 0;
                //rlUpdateShaderBuffer(chunkVisibilityValueBuffer, &chunkVisibilityValue, 1 * sizeof(int), 0);

                //rlEnableShader(resetChunkVisibilityComputeShader);
                //rlBindShaderBuffer(chunkVisibilitySSBO, 4);
                //rlBindShaderBuffer(chunkVisibilityValueBuffer, 5);

                //int numDispatchXZ = (numChunksFullWidth % localSizeOfComputeXZ != 0) ? (numChunksFullWidth / localSizeOfComputeXZ) + 1 : (numChunksFullWidth / localSizeOfComputeXZ);
                //if (localSizeOfComputeXZ > numChunksFullWidth) {
                //    numDispatchXZ = 1;
                //}
                ////rlComputeShaderDispatch(numDispatchXZ, numChunksFullWidth_Y, numDispatchXZ);
                //rlComputeShaderDispatch(6, 3, 6);

                //rlMemoryBarrierShaderStorage();
                //rlDisableShader();

                //rlReadShaderBuffer(chunkVisibilitySSBO, chunkVisibility.data(), chunkVisibility.size() * sizeof(int), 0);

                //for (int i = 0; i < chunkVisibility.size(); i++)
                //{
                //    if (chunkVisibility[i] != 0) {
                //        std::cout << "NOT SET TO 0!!!!!!" << std::endl;
                //        break;
                //    }
                //}

                for (int i = 0; i < chunkVisibility.size(); i++)
                {
                    chunkVisibility[i] = 0;
                }
                rlUpdateShaderBuffer(chunkVisibilitySSBO, chunkVisibility.data(), chunkVisibility.size() * sizeof(int), 0);
                rlMemoryBarrierShaderStorage();

                BeginTextureMode(rd2D);
                {
                    ClearBackground(WHITE);

                    rlEnableShader(cullingRenderMaterial.shader.id);

                    int cameraPosLocInCullingShader = GetShaderLocation(cullingRenderMaterial.shader, "cameraPos");
                    float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
                    SetShaderValue(cullingRenderMaterial.shader, cameraPosLocInCullingShader, cameraPos, SHADER_UNIFORM_VEC3);

                    BeginMode3D(camera);

                    rlActiveTextureSlot(bindDepthTextureAtPosition);
                    rlEnableTexture(target.depthColourTexture.id);

                    rlBindShaderBuffer(chunksGridPosSSBO, 3);
                    rlBindShaderBuffer(chunkVisibilitySSBO, 4);

                    //rlEnableWireMode();
                    DrawMeshMultiInstancedDrawIndirectGPU2(cullingRenderQuad, cullingRenderMaterial
                        , megaArrayOfAllPositions2.data(), megaArrayOfAllPositions2.size()
                        , drawArraysIndirectCommands2.size()
                        , false);
                    //rlDisableWireMode();

                    rlMemoryBarrierShaderStorage();

                    EndMode3D();

                    rlDisableShader();

                    //for (int i = 0; i < chunkVisibility.size(); i++)
                    //{
                    //    chunkVisibility[i] = 1;
                    //}
                    //rlUpdateShaderBuffer(chunkVisibilitySSBO, chunkVisibility.data(), chunkVisibility.size() * sizeof(int), 0);
                    //rlMemoryBarrierShaderStorage();

                }
                EndTextureMode();
            }
            else {

                for (int i = 0; i < chunkVisibility.size(); i++)
                {
                    chunkVisibility[i] = 1;
                }
                rlUpdateShaderBuffer(chunkVisibilitySSBO, chunkVisibility.data(), chunkVisibility.size() * sizeof(int), 0);
                rlMemoryBarrierShaderStorage();
            }

        }


        {
            PROFILE_SCOPE("GPU FRUSTUM AND BACK FACE CULLING.");

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

            nextIndirectdrawCommandIndexBufferValue = 0;
            rlUpdateShaderBuffer(renderQuad.commandsLengthBOID, &nextIndirectdrawCommandIndexBufferValue, 1 * sizeof(int), 0);

            rlEnableShader(frustumAndFaceCullingComputeProgram);
            rlBindShaderBuffer(chunkVisibilitySSBO, 4);

            rlBindShaderBuffer(renderQuad.commandsLengthBOID, 5);
            rlBindShaderBuffer(renderQuad.commandsBufferVBOID, 6);

            rlBindShaderBuffer(renderQuad.upFacesMetadaBufferID, 7);
            rlBindShaderBuffer(renderQuad.downFacesMetadaBufferID, 8);
            rlBindShaderBuffer(renderQuad.frontFacesMetadaBufferID, 9);
            rlBindShaderBuffer(renderQuad.backFacesMetadaBufferID, 10);
            rlBindShaderBuffer(renderQuad.rightFacesMetadaBufferID, 11);
            rlBindShaderBuffer(renderQuad.leftFacesMetadaBufferID, 12);

            rlBindShaderBuffer(renderQuad.chunkPositionsVBOID, 13);

            float3 nearPlaneNormal = { nearPlane.normal.x,  nearPlane.normal.y,  nearPlane.normal.z };
            float3 farPlaneNormal = { farPlane.normal.x,  farPlane.normal.y,  farPlane.normal.z };
            float3 rightPlaneNormal = { rightPlane.normal.x,  rightPlane.normal.y,  rightPlane.normal.z };
            float3 leftPlaneNormal = { leftPlane.normal.x,  leftPlane.normal.y,  leftPlane.normal.z };
            float3 topPlaneNormal = { topPlane.normal.x,  topPlane.normal.y,  topPlane.normal.z };
            float3 bottomPlaneNormal = { bottomPlane.normal.x,  bottomPlane.normal.y,  bottomPlane.normal.z };
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "nearPlaneNormal"), &nearPlaneNormal, SHADER_UNIFORM_VEC3, 1);
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "farPlaneNormal"), &farPlaneNormal, SHADER_UNIFORM_VEC3, 1);
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "rightPlaneNormal"), &rightPlaneNormal, SHADER_UNIFORM_VEC3, 1);
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "leftPlaneNormal"), &leftPlaneNormal, SHADER_UNIFORM_VEC3, 1);
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "topPlaneNormal"), &topPlaneNormal, SHADER_UNIFORM_VEC3, 1);
            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "bottomPlaneNormal"), &bottomPlaneNormal, SHADER_UNIFORM_VEC3, 1);

            Vector3 cameraDirection = Vector3Subtract(camera.target, camera.position);
            cameraDirection = Vector3Normalize(cameraDirection);
            float3 cameraDirectionToSend = { cameraDirection.x, cameraDirection.y, cameraDirection.z };
            int cameraDirLoc = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "cameraDir");
            rlSetUniform(cameraDirLoc, &cameraDirectionToSend, SHADER_UNIFORM_VEC3, 1);

            float3 cameraPositionToSend = { camera.position.x, camera.position.y, camera.position.z };
            int cameraPositionLoc = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "cameraPosition");
            rlSetUniform(cameraPositionLoc, &cameraPositionToSend, SHADER_UNIFORM_VEC3, 1);

            //Set MVP matrix in compute shader.
            Matrix modelMatrix = MatrixIdentity();
            Matrix projectionMatrix = MatrixPerspective(camera.fovy, (float)GetScreenWidth() / (float)GetScreenHeight(), 0.1f, farPlaneDistance);
            Matrix viewMatrix = MatrixLookAt(camera.position, camera.target, camera.up);

            Matrix mvpMatrix = MatrixMultiply(projectionMatrix, viewMatrix);
            mvpMatrix = MatrixMultiply(mvpMatrix, modelMatrix);

            int mvpLoc = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "mvp");
            rlSetUniformMatrix(mvpLoc, mvpMatrix);
            //End.

            int shouldPerformOcclusionCullingLoc = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "shouldPerformOcclusionCulling");
            rlSetUniform(shouldPerformOcclusionCullingLoc, &occlusionCullingStateValue, SHADER_UNIFORM_FLOAT, 1);

            int previousDepthInformationTexLoc = rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "previousDepthInformationTex");
            rlSetUniform(previousDepthInformationTexLoc, &bindDepthTextureAtPosition, SHADER_UNIFORM_SAMPLER2D, 1);

            rlActiveTextureSlot(bindDepthTextureAtPosition);
            rlEnableTexture(target.depthColourTexture.id);

            rlSetUniform(rlGetLocationUniform(frustumAndFaceCullingComputeProgram, "diagonalDist"), &diagonalDist, SHADER_UNIFORM_FLOAT, 1);

            int numDispatchXZ = (numChunksFullWidth % chunkSize != 0) ? (numChunksFullWidth / localSizeOfComputeXZ) + 1 : (numChunksFullWidth / localSizeOfComputeXZ);
            if (localSizeOfComputeXZ > numChunksFullWidth) {
                numDispatchXZ = 1;
            }
            rlComputeShaderDispatch(numDispatchXZ, numChunksFullWidth_Y, numDispatchXZ);

            rlMemoryBarrierShaderStorage();

            //int numDrawCounts = 0;
            //rlReadShaderBuffer(renderQuad.commandsLengthBOID, &numDrawCounts, 1, 0);
            //std::cout << numDrawCounts << std::endl;

            rlDisableShader();
        }


        BeginTextureMode(target);
        {
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
            {
                PROFILE_SCOPE("Drawing Chunks");

                {
                    {
                        PROFILE_SCOPE("Checking and Sending Recalculations For Changed Chunks.");

                        // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV NEEDS TO BE OPTIMISED!!!!!!!!!! VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
                        //for (int i = 0; i < renderTraversalOrder.size(); i++)
                        for (int i = 0; i < chunksWhereNewMeshNeedsToBeCalculated.size(); i++)
                        {
                            //PROFILE_SCOPE("Time for checking one chunk.");


                            //int renderTraversalIndexFlattened = megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i]);
                            int renderTraversalIndexFlattened = megaVertPositions.InnerIndexFlattened(chunksWhereNewMeshNeedsToBeCalculated[i]);

                            {
                                if (/*innerIndexWhereNewMeshNeedsToBeCalculated.contains(renderTraversalIndexFlattened) && innerIndexWhereNewMeshNeedsToBeCalculated[renderTraversalIndexFlattened]*/ true) {

                                    PROFILE_SCOPE("Recalculating Data.");

                                    //Vector3 offsetRenderTraversalOrder = mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])];
                                    Vector3 offsetRenderTraversalOrder = mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunksWhereNewMeshNeedsToBeCalculated[i])];
                                    //std::cout << offsetRenderTraversalOrder.x << ", " << offsetRenderTraversalOrder.y << ", " << offsetRenderTraversalOrder.z << std::endl;
                                    //Vector3 curChunkTraversalIndex = Vector3{ renderTraversalOrder[i].x + cameraChunkIndex.x, renderTraversalOrder[i].y, renderTraversalOrder[i].z + cameraChunkIndex.z };
                                    Vector3 curChunkTraversalIndex = Vector3{ chunksWhereNewMeshNeedsToBeCalculated[i].x + cameraChunkIndex.x, chunksWhereNewMeshNeedsToBeCalculated[i].y, chunksWhereNewMeshNeedsToBeCalculated[i].z + cameraChunkIndex.z };

                                    //Vector3 oldChunkTraversalIndex = Vector3{ renderTraversalOrder[i].x + oldCameraChunkPosition.x, renderTraversalOrder[i].y, renderTraversalOrder[i].z + oldCameraChunkPosition.z };
                                    Vector3 oldChunkTraversalIndex = Vector3{ chunksWhereNewMeshNeedsToBeCalculated[i].x + oldCameraChunkPosition.x, chunksWhereNewMeshNeedsToBeCalculated[i].y, chunksWhereNewMeshNeedsToBeCalculated[i].z + oldCameraChunkPosition.z };
                                    int offsetFlattenedIndex = megaVertPositions.InnerIndexFlattened(offsetRenderTraversalOrder);

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

                                        int curLodLevel = 0;
                                        //int curDistFromCamera = (int)(Vector3Length(Vector3{ renderTraversalOrder[i].x, 0, renderTraversalOrder[i].z }));
                                        int curDistFromCamera = (int)(Vector3Length(Vector3{ chunksWhereNewMeshNeedsToBeCalculated[i].x, 0, chunksWhereNewMeshNeedsToBeCalculated[i].z }));
                                        if (curDistFromCamera >= lodDistance1.x && curDistFromCamera <= lodDistance1.y) {
                                            curLodLevel = LODLevel + 0;
                                            //std::cout << "x < 4 : " << curLodLevel << std::endl;;
                                        }
                                        else if (curDistFromCamera >= lodDistance2.x && curDistFromCamera <= lodDistance2.y) {
                                            curLodLevel = LODLevel + 1;
                                            //std::cout << "x >=4 && x <= 6 : " << curLodLevel << std::endl;;
                                        }
                                        else if (curDistFromCamera >= lodDistance3.x && curDistFromCamera <= lodDistance3.y) {
                                            curLodLevel = LODLevel + 2;
                                            //std::cout << "x >= 7 && x <= 9 : " << curLodLevel << std::endl;;
                                        }
                                        else if (curDistFromCamera >= lodDistance4.x && curDistFromCamera <= lodDistance4.y) {
                                            curLodLevel = LODLevel + 3;
                                            //std::cout << "x >= 10 : " << curLodLevel << std::endl;;
                                        }
                                        else if (curDistFromCamera >= lodDistance5.x) {
                                            curLodLevel = LODLevel + 4;
                                            //std::cout << "x >= 10 : " << curLodLevel << std::endl;;
                                        }

                                        if (curLodLevel > 5) {
                                            curLodLevel = 5;
                                        }

                                        bool reclaimMemory = !megaVertPositions.IsFirstTimeAllocatingMemory(offsetRenderTraversalOrder);

                                        //GenChunkMeshWithNoiseAndSaveToFile(
                                        //    std::ref(megaVertPositions)
                                        //    , std::ref(chunkGenerated)
                                        //    , std::ref(chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray)
                                        //    , std::ref(chunkBeingGeneratedCount)
                                        //    , curChunkTraversalIndex
                                        //    , offsetRenderTraversalOrder
                                        //    , (int)curLodLevel
                                        //    , reclaimMemory
                                        //    , saveChunkToFile);

                                        chunkMeshGenThreads.push_back(std::async(std::launch::async, GenChunkMeshWithNoiseAndSaveToFile
                                            , std::ref(megaVertPositions)
                                            , std::ref(chunkGenerated)
                                            , std::ref(chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray)
                                            , std::ref(chunkBeingGeneratedCount)
                                            , curChunkTraversalIndex
                                            , offsetRenderTraversalOrder
                                            , (int)curLodLevel
                                            , reclaimMemory
                                            , saveChunkToFile));

                                        //GenChunkMeshWithNoise(std::ref(megaVertPositions)
                                        //    , std::ref(chunkGenerated)
                                        //    , std::ref(chunkBeingGeneratedCount)
                                        //    , curChunkTraversalIndex
                                        //    , offsetRenderTraversalOrder);
                                    }


                                    chunksChanged = true;
                                    //innerIndexWhereNewMeshNeedsToBeCalculated[renderTraversalIndexFlattened] = false;
                                }
                            }

                        }
                    }

                    {
                        PROFILE_SCOPE("Updating GPU data and Rendering.");

                        //OPTIMISE!!!!
                        if ((chunkBeingGeneratedCount == 0 && chunksChanged)) {
                            rlEnableVertexArray(renderQuad.mesh.vaoId);

                            //renderQuad.instanceVBOID = rlLoadVertexBuffer(megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size() * sizeof(int), true);

                            for (const auto& it : chunkUpdatedVoxelPositionInBigArrayMappedToChunkPositionInArray) {

                                int startUp = megaVertPositions.upFacesMetadata[it].startPositionInBigArray;
                                int sizeUp = megaVertPositions.upFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startUp, sizeUp * sizeof(int), startUp * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.upFacesMetadaBufferID, megaVertPositions.upFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));

                                int startDown = megaVertPositions.downFacesMetadata[it].startPositionInBigArray;
                                int sizeDown = megaVertPositions.downFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startDown, sizeDown * sizeof(int), startDown * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.downFacesMetadaBufferID, megaVertPositions.downFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));

                                int startFront = megaVertPositions.frontFacesMetadata[it].startPositionInBigArray;
                                int sizeFront = megaVertPositions.frontFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startFront, sizeFront * sizeof(int), startFront * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.frontFacesMetadaBufferID, megaVertPositions.frontFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));

                                int startBack = megaVertPositions.backFacesMetadata[it].startPositionInBigArray;
                                int sizeBack = megaVertPositions.backFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startBack, sizeBack * sizeof(int), startBack * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.backFacesMetadaBufferID, megaVertPositions.backFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));

                                int startRight = megaVertPositions.rightFacesMetadata[it].startPositionInBigArray;
                                int sizeRight = megaVertPositions.rightFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startRight, sizeRight * sizeof(int), startRight * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.rightFacesMetadaBufferID, megaVertPositions.rightFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));

                                int startLeft = megaVertPositions.leftFacesMetadata[it].startPositionInBigArray;
                                int sizeLeft = megaVertPositions.leftFacesMetadata[it].size;
                                rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startLeft, sizeLeft * sizeof(int), startLeft * sizeof(int));
                                rlUpdateShaderBuffer(renderQuad.leftFacesMetadaBufferID, megaVertPositions.leftFacesMetadata.data() + it, 1 * sizeof(ChunkFacePositionMetaData), it * sizeof(ChunkFacePositionMetaData));
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

                        rlBindShaderBuffer(renderQuad.chunkPositionsVBOID, 13);
                        rlBindShaderBuffer(chunkVisibilitySSBO, 4);

                        //rlEnableWireMode();

                        DrawMeshMultiInstancedDrawIndirectGPU2(renderQuad, instancedMaterial
                            , megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size()
                            , totalNumChunks * NUM_FACES);

                        //rlDisableWireMode();
                        if (false) {
                            // Draw quad(?)

                            rlEnableShader(simpleShader.id);

                            rlEnableVertexArray(quadVAO);
                            //rlDrawVertexArray(0, 4);
                            rlDrawVertexArrayElements(0, 6, 0);
                            
                            rlEnableVertexArray(0);
                            rlDisableShader();
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
            std::string cameraChunkPosText = std::to_string(cameraChunkIndex.x) + ", " + std::to_string(cameraChunkIndex.y) + ", " + std::to_string(cameraChunkIndex.z);
            //DrawText(cameraVoxelPosText.c_str(), 0, 0, 20, BLACK);
            DrawText(cameraChunkPosText.c_str(), 0, 120, 20, BLACK);
            //int x = (int)camera.position.x;
            //int y = (int)camera.position.y;
            //int z = (int)camera.position.z;
            //float scale = 0.1f;
            //std::string curNoiseValue = std::to_string(perlin.noise3D_01((double)x * scale, (double)z * scale, (double)(y) * scale));
            //DrawText(curNoiseValue.c_str(), 0, 140, 20, BLACK);

            oldCameraChunkPosition = cameraChunkIndex;
            oldCameraPos = camera.position;
        }
        EndTextureMode();

        if (frameCounter >= 100)
        {
            //std::cout << "DON'T RENDER ALL ANYMORE!" << std::endl;
            renderAllValue = 0;
            SetShaderValue(instanceShader, renderAllLoc, &renderAllValue, SHADER_UNIFORM_FLOAT);
        }

        {
            //int defFB = 0;
            if(true)
            {
                PROFILE_SCOPE("Drawing To Screen");

                BeginDrawing();
                {

                    PROFILE_SCOPE("Draws");

                    //defFB = rlGetActiveFramebuffer();
                    //std::cout << defFB << std::endl;

                    //std::cout << randValue << std::endl;

                    ClearBackground(RAYWHITE);
                    if (randValue == 0) {
                        DrawTextureRec(target.texture, Rectangle{ 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2{ 0, 0 }, WHITE);
                    }
                    else if (randValue == 1) {
                        DrawTextureRec(target.texture, Rectangle{ 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2{ 0, 0 }, WHITE);
                    }
                    else if (randValue == 2) {
                        DrawTextureRec(target.secondColourTexture, Rectangle{ 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2{ 0, 0 }, WHITE);
                    }
                    else if (randValue == 3) {
                        DrawTextureRec(target.depthColourTexture, Rectangle{ 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2{ 0, 0 }, WHITE);
                    }
                    else if (randValue == 4) {
                        DrawTextureRec(rd2D.texture, Rectangle{ 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2{ 0, 0 }, WHITE);
                    }

                    DrawRectangle(screenWidth - 40, 10, 30, 30, shouldPerformOcclusionCulling ? GREEN : RED);

                    DrawCircle(screenWidth / 2, screenHeight / 2, 1.0f, RED);
                    DrawFPS(40, 40);

                    if (false) {
                        rlEnableShader(simpleShader.id);

                        rlEnableVertexArray(quadVAO);
                        rlDrawVertexArray(0, 4);
                        rlEnableVertexArray(0);

                        rlDisableShader();
                    }
                }
                EndDrawing();
            }

            if (false) {
                // Draw quad(?)

                //rlEnableFramebuffer(defFB);

                PROFILE_SCOPE("Empty Draws");

                //std::cout << "Here!" << std::endl;

                BeginDrawing();

                    ClearBackground(RAYWHITE);

                        rlEnableShader(simpleShader.id);

                            rlActiveTextureSlot(0);
                            rlEnableTexture(target.texture.id);
                            //rlEnableTexture(textureLoad.id);

                            rlEnableVertexArray(quadVAO);
                            //rlDrawVertexArray(0, 4);
                            rlDrawVertexArrayElements(0, 6, 0);

                        rlEnableVertexArray(0);
                        rlDisableShader();

                    DrawFPS(40, 40);

                EndDrawing();
            }
        }
        chunksWhereNewMeshNeedsToBeCalculated.clear();

        //if(false)
        //{
        //    PROFILE_SCOPE("Swap Screen buffers");

        //    SwapScreenBuffer();
        //}
    }

    // Delete buffers (VBO and VAO)
    rlUnloadVertexBuffer(quadVBO);
    rlUnloadVertexArray(quadVAO);


    //rlUnloadShaderBuffer(chunkPosSSBO);
    UnloadRenderTextureDepthTex(target);
    UnloadRenderTextureDepthTex(rd2D);

    rlUnloadVertexBuffer(indirectBufferVBO);

    UnloadShader(instanceShader);
    UnloadShader(cullingShader);

    UnloadTexture(textureLoad);

    CloseWindow();

    Instrumentor::Instance().EndSession();

    return 0;
}

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
    , Plane& bottomPlane
    , int &numChunksDrawn
    , int &numChunksDrawnWithoutFrustum)
{
    PROFILE_FUNCTION();

    //std::cout << shouldDrawChunk << std::endl;
    int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    Vector3 drawCurChunkPos = { drawChunkIndex.x * chunkSize, drawChunkIndex.y * chunkSize, drawChunkIndex.z * chunkSize };

    bool cameraInThisChunkWidthAndBreadth = drawChunkIndex.x <= cameraChunkIndex.x || drawChunkIndex.z <= cameraChunkIndex.z;

    Vector3 dirToChunkFromCamera = drawCurChunkPos - camera.position;
    dirToChunkFromCamera = Vector3Normalize(dirToChunkFromCamera);

    bool shouldDrawChunk = ShouldDrawChunk(drawCurChunkPos, camera, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);

    constexpr bool drawAll = false;
    numChunksDrawnWithoutFrustum++;
    if (shouldDrawChunk) {

        numChunksDrawn++;

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

        int curChunkIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        if (dotUp < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.upFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.upFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //std::cout << "x : " << curChunkPos.x << " y : " << curChunkPos.y << " z : " << curChunkPos.z << std::endl;
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotDown < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.downFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.downFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotFront < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.frontFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.frontFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotBack < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.backFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.backFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotRight < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.rightFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.rightFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0 || drawAll) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                drawArraysIndirectCommands.push_back(curCommand);
                chunkPositions.push_back(float3{ drawCurChunkPos.x, drawCurChunkPos.y, drawCurChunkPos.z });
                //chunkPositions.push_back(float3{ 0, 0, 0});
            }
        }

        if (dotLeft < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
            int start = megaVertPositions.leftFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = megaVertPositions.leftFacesMetadata[curChunkIndexWithoutVoxels].size;
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

    //PROFILE_FUNCTION();

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
    PROFILE_FUNCTION();

    int powerOfTwo = pow(2, lodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize + powerOfTwo;

    int startY = powerOfTwo;
    int endY = chunkSize + powerOfTwo;

    int startZ = powerOfTwo;
    int endZ = chunkSize + powerOfTwo;

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

static void MakeNoiseForChunkLOD(std::vector<std::vector<std::vector<float>>>& noiseStorage
                                , FastNoise::SmartNode<FastNoise::FractalFBm>& fnFractal
                                , int chunksX, int chunksY, int chunksZ
                                , int numChunks, int numChunksY, int chunksSize
                                , int sideVoxelsToConsider, float scale
                                , int curLodLevel) {

    PROFILE_FUNCTION();

    int _x, _y, _z = 0;

    int _sideVoxelsToConsider = (sideVoxelsToConsider / 2);
    int start = _sideVoxelsToConsider * -1;
    int end = chunkSize + _sideVoxelsToConsider;

    int voxelScale = pow(2, curLodLevel);


    for (int x = start; x < end; x++)
    {
        _x = (x * voxelScale) + (chunksX * chunkSize);

        for (int z = start; z < end; z++)
        {
            _z = (z * voxelScale) + (chunksZ * chunksSize);

            //float noise = perlin.noise2D_01((double)_x * scale, (double)_z * scale);
            //float noise = perlin.normalizedOctave2D_01((double)_x * scale, (double)_z * scale, 4);
            float noise = 0;
            fnFractal->GenUniformGrid2D(&noise, _x, _z, 1, 1, scale, 1337);

            int scaledNoise = (int)(noise * chunksSize * numChunksFullWidth_Y);

            for (int y = start; y < end; y++)
            {
                _y = y + chunksY * chunkSize;
                if (_y < scaledNoise) {// This is the position under the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 0; // STONE BLOCK
                }
                else if (_y == scaledNoise) {// This is the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 1; // DIRT BLOCK
                }
                else if (_y > scaledNoise) {// This is the position above the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 2; // AIR BLOCK
                }
            }
        }
    }
}


static void MakeNoiseForChunk(std::vector<std::vector<std::vector<float>>> &noiseStorage, FastNoise::SmartNode<FastNoise::FractalFBm>& fnFractal, int chunksX, int chunksY, int chunksZ, int numChunks, int numChunksY, int chunksSize, int sideVoxelsToConsider, float scale) {

    PROFILE_FUNCTION();

    int _x, _y, _z = 0;

    int _sideVoxelsToConsider = (sideVoxelsToConsider / 2);
    int start = _sideVoxelsToConsider * -1;
    int end = chunkSize + _sideVoxelsToConsider;

    for (int x = start; x < end; x++)
    {
        _x = x + (chunksX * chunkSize);

        for (int z = start; z < end; z++)
        {
            _z = z + (chunksZ * chunksSize);

            //float noise = perlin.noise2D_01((double)_x * scale, (double)_z * scale);
            //float noise = perlin.normalizedOctave2D_01((double)_x * scale, (double)_z * scale, 4);
            float noise = 0;
            fnFractal->GenUniformGrid2D(&noise, _x, _z, 1, 1, scale, 1337);

            int scaledNoise = (int)(noise * chunksSize * numChunksFullWidth_Y);

            for (int y = start; y < end; y++)
            {
                _y = y + chunksY * chunkSize;
                if (_y < scaledNoise) {// This is the position under the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 0; // STONE BLOCK
                }
                else if (_y == scaledNoise) {// This is the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 1; // DIRT BLOCK
                }
                else if (_y > scaledNoise) {// This is the position above the noise height.
                    noiseStorage[x + _sideVoxelsToConsider][y + _sideVoxelsToConsider][z + _sideVoxelsToConsider] = 2; // AIR BLOCK
                }
            }
        }
    }
}

static void MakeNoise2D(std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>>>& noiseStorage, int numChunks, int numChunksY, int chunksSize, float scale) {

    PROFILE_FUNCTION();

    auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();

    fnFractal->SetSource(fnSimplex);
    fnFractal->SetOctaveCount(5);

    for (int chunksX = 0; chunksX < numChunks; chunksX++)
    {
        for (int chunksY = 0; chunksY < numChunksY; chunksY++)
        {
            for (int chunksZ = 0; chunksZ < numChunks; chunksZ++)
            {
                MakeNoiseForChunk(noiseStorage[chunksX][chunksY][chunksZ], fnFractal, chunksX, chunksY, chunksZ, numChunks, numChunksY, chunkSize, 1, scale);
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
                                                        , std::vector<int> &chunkMeshData
                                                        , ChunkFacesMetadata &chunkFacesMetadata
                                                        , VertexPositions &megaVertPositions
                                                        , Vector3 innerChunkIndex
                                                        , Vector3 chunkIndex
                                                        , int curLodLevel)
{
    PROFILE_FUNCTION();

    //std::cout << innerChunkIndex.x << ", " << innerChunkIndex.y << ", " << innerChunkIndex.z << std::endl;

    chunkMeshData = std::vector<int>(totalNumVoxelsPerChunkWorstCase * NUM_FACES);

    chunkFacesMetadata.upFacesStartIndex = FACE_UP_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.downFacesStartIndex = FACE_DOWN_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.frontFacesStartIndex = FACE_FRONT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.backFacesStartIndex = FACE_BACK_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.rightFacesStartIndex = FACE_RIGHT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.leftFacesStartIndex = FACE_LEFT_INDEX * totalNumVoxelsPerChunkWorstCase;

    int scale = pow(2, curLodLevel);
    //int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    int powerOfTwo = pow(2, curLodLevel);
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

                    //float curNoiseTop = (y + stepSizeForConvolution <= endY) ? noiseForCurrentChunk[x][y + stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y + 1][z]);
                    float curNoiseTop = noiseForCurrentChunk[x][y + stepSizeForConvolution][z];

                    if (curNoiseTop == 2) {
                        int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPositionTemp);
                        //megaVertPositions.AddUp(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.upFacesStartIndex + chunkFacesMetadata.numUpFaces] = curPositionTemp;
                        chunkFacesMetadata.numUpFaces++;
                        //std::cout << innerChunkIndex.y << std::endl;
                    }

                    //float curNoiseBottom = (y - stepSizeForConvolution >= startY - 1) ? noiseForCurrentChunk[x][y - stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y - 1][z]);
                    float curNoiseBottom = noiseForCurrentChunk[x][y - stepSizeForConvolution][z];

                    if (curNoiseBottom == 2) {
                        int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPositionTemp);
                        //megaVertPositions.AddDown(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.downFacesStartIndex + chunkFacesMetadata.numDownFaces] = curPositionTemp;
                        chunkFacesMetadata.numDownFaces++;
                    }

                    //float curNoiseFront = (z + stepSizeForConvolution <= endZ) ? noiseForCurrentChunk[x][y][z + stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z + 1]);
                    float curNoiseFront = noiseForCurrentChunk[x][y][z + stepSizeForConvolution];

                    if (curNoiseFront == 2) {
                        int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPositionTemp);
                        //megaVertPositions.AddFront(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.frontFacesStartIndex + chunkFacesMetadata.numFrontFaces] = curPositionTemp;
                        chunkFacesMetadata.numFrontFaces++;
                    }

                    //float curNoiseBack = (z - stepSizeForConvolution >= startZ - 1) ? noiseForCurrentChunk[x][y][z - stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z - 1]);
                    float curNoiseBack = noiseForCurrentChunk[x][y][z - stepSizeForConvolution];

                    if (curNoiseBack == 2) {
                        int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPositionTemp);
                        //megaVertPositions.AddBack(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.backFacesStartIndex + chunkFacesMetadata.numBackFaces] = curPositionTemp;
                        chunkFacesMetadata.numBackFaces++;
                    }

                    //float curNoiseRight = (x + stepSizeForConvolution <= endX) ? noiseForCurrentChunk[x + stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x + 1][y][z]);
                    float curNoiseRight = noiseForCurrentChunk[x + stepSizeForConvolution][y][z];

                    if (curNoiseRight == 2) {
                        int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPositionTemp);
                        //megaVertPositions.AddRight(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.rightFacesStartIndex + chunkFacesMetadata.numRightFaces] = curPositionTemp;
                        chunkFacesMetadata.numRightFaces++;
                    }

                    //float curNoiseLeft = (x - stepSizeForConvolution >= startX - 1) ? noiseForCurrentChunk[x - stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x - 1][y][z]);
                    float curNoiseLeft = noiseForCurrentChunk[x - stepSizeForConvolution][y][z];

                    if (curNoiseLeft == 2) {
                        int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curPositionTemp);
                        //megaVertPositions.AddLeft(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.leftFacesStartIndex + chunkFacesMetadata.numLeftFaces] = curPositionTemp;
                        chunkFacesMetadata.numLeftFaces++;
                    }
                }
            }
        }
    }
}

static void GenMeshCustom2DLOD(std::vector<std::vector<std::vector<float>>> &noiseForCurrentChunk
                                                        , std::vector<int> &chunkMeshData
                                                        , ChunkFacesMetadata &chunkFacesMetadata
                                                        , VertexPositions &megaVertPositions
                                                        , Vector3 innerChunkIndex
                                                        , Vector3 chunkIndex
                                                        , int curLodLevel)
{
    PROFILE_FUNCTION();

    //std::cout << innerChunkIndex.x << ", " << innerChunkIndex.y << ", " << innerChunkIndex.z << std::endl;

    chunkMeshData = std::vector<int>(totalNumVoxelsPerChunkWorstCase * NUM_FACES);

    chunkFacesMetadata.upFacesStartIndex = FACE_UP_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.downFacesStartIndex = FACE_DOWN_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.frontFacesStartIndex = FACE_FRONT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.backFacesStartIndex = FACE_BACK_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.rightFacesStartIndex = FACE_RIGHT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.leftFacesStartIndex = FACE_LEFT_INDEX * totalNumVoxelsPerChunkWorstCase;

    int scale = pow(2, curLodLevel);
    //int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    int powerOfTwo = pow(2, curLodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;

    int stepSizeForConvolution = powerOfTwo;
    stepSizeForConvolution = 1;

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

                    //float curNoiseTop = (y + stepSizeForConvolution <= endY) ? noiseForCurrentChunk[x][y + stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y + 1][z]);
                    float curNoiseTop = noiseForCurrentChunk[x][y + stepSizeForConvolution][z];

                    if (curNoiseTop == 2) {
                        int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPositionTemp);
                        //megaVertPositions.AddUp(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.upFacesStartIndex + chunkFacesMetadata.numUpFaces] = curPositionTemp;
                        chunkFacesMetadata.numUpFaces++;
                        //std::cout << innerChunkIndex.y << std::endl;
                    }

                    //float curNoiseBottom = (y - stepSizeForConvolution >= startY - 1) ? noiseForCurrentChunk[x][y - stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y - 1][z]);
                    float curNoiseBottom = noiseForCurrentChunk[x][y - stepSizeForConvolution][z];

                    if (curNoiseBottom == 2) {
                        int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPositionTemp);
                        //megaVertPositions.AddDown(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.downFacesStartIndex + chunkFacesMetadata.numDownFaces] = curPositionTemp;
                        chunkFacesMetadata.numDownFaces++;
                    }

                    //float curNoiseFront = (z + stepSizeForConvolution <= endZ) ? noiseForCurrentChunk[x][y][z + stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z + 1]);
                    float curNoiseFront = noiseForCurrentChunk[x][y][z + stepSizeForConvolution];

                    if (curNoiseFront == 2) {
                        int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPositionTemp);
                        //megaVertPositions.AddFront(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.frontFacesStartIndex + chunkFacesMetadata.numFrontFaces] = curPositionTemp;
                        chunkFacesMetadata.numFrontFaces++;
                    }

                    //float curNoiseBack = (z - stepSizeForConvolution >= startZ - 1) ? noiseForCurrentChunk[x][y][z - stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z - 1]);
                    float curNoiseBack = noiseForCurrentChunk[x][y][z - stepSizeForConvolution];

                    if (curNoiseBack == 2) {
                        int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPositionTemp);
                        //megaVertPositions.AddBack(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.backFacesStartIndex + chunkFacesMetadata.numBackFaces] = curPositionTemp;
                        chunkFacesMetadata.numBackFaces++;
                    }

                    //float curNoiseRight = (x + stepSizeForConvolution <= endX) ? noiseForCurrentChunk[x + stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x + 1][y][z]);
                    float curNoiseRight = noiseForCurrentChunk[x + stepSizeForConvolution][y][z];

                    if (curNoiseRight == 2) {
                        int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPositionTemp);
                        //megaVertPositions.AddRight(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.rightFacesStartIndex + chunkFacesMetadata.numRightFaces] = curPositionTemp;
                        chunkFacesMetadata.numRightFaces++;
                    }

                    //float curNoiseLeft = (x - stepSizeForConvolution >= startX - 1) ? noiseForCurrentChunk[x - stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x - 1][y][z]);
                    float curNoiseLeft = noiseForCurrentChunk[x - stepSizeForConvolution][y][z];

                    if (curNoiseLeft == 2) {
                        int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_X);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Y);
                        //curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT_Z);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curPositionTemp);
                        //megaVertPositions.AddLeft(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.leftFacesStartIndex + chunkFacesMetadata.numLeftFaces] = curPositionTemp;
                        chunkFacesMetadata.numLeftFaces++;
                    }
                }
            }
        }
    }
}


class GreedyMeshCurShape {

public:
    Vector3 startPos = Vector3Zeros;
    Vector3 endPos = Vector3Zeros;
    Vector3 scale = Vector3Ones;
};

static void UpdateMeshFromGreedyShape(std::vector<GreedyMeshCurShape>& curGreedyMeshShapes
                                    , std::vector<int>& chunkMeshData
                                    , ChunkFacesMetadata& chunkFacesMetadata
                                    , int faceDir
                                    , int stepSizeForConvolution) {

    PROFILE_FUNCTION();

    //Optimize using data from large array, check if the number of faces is greater than 0 in that particular direction or something.

    for (int i = 0; i < curGreedyMeshShapes.size(); i++)
    {
        int curPosition = PackThreeNumbers(curGreedyMeshShapes[i].startPos.x - stepSizeForConvolution, curGreedyMeshShapes[i].startPos.y - stepSizeForConvolution, curGreedyMeshShapes[i].startPos.z - stepSizeForConvolution);

        int scaleX = curGreedyMeshShapes[i].scale.x;
        int scaleY = curGreedyMeshShapes[i].scale.y;
        int scaleZ = curGreedyMeshShapes[i].scale.z;

        //curPosition = curPosition | (scaleX << SCALE_POSITION_IN_PACKED_INT_X);
        //curPosition = curPosition | (scaleY << SCALE_POSITION_IN_PACKED_INT_Y);
        //curPosition = curPosition | (scaleZ << SCALE_POSITION_IN_PACKED_INT_Z);

        if (faceDir == 0 || faceDir == 1) {

            curPosition = curPosition | (scaleX << SCALE_POSITION_IN_PACKED_INT_A);
            curPosition = curPosition | (scaleZ << SCALE_POSITION_IN_PACKED_INT_B);
            //curVertex = vec3(curVertex.x * curScaleA, curVertex.y, curVertex.z * curScaleB);
        }
        else if (faceDir == 2 || faceDir == 3) {

            curPosition = curPosition | (scaleX << SCALE_POSITION_IN_PACKED_INT_A);
            curPosition = curPosition | (scaleY << SCALE_POSITION_IN_PACKED_INT_B);
            //curVertex = vec3(curVertex.x * curScaleA, curVertex.y * curScaleB, curVertex.z);
        }
        else if (faceDir == 4 || faceDir == 5) {
            
            curPosition = curPosition | (scaleZ << SCALE_POSITION_IN_PACKED_INT_A);
            curPosition = curPosition | (scaleY << SCALE_POSITION_IN_PACKED_INT_B);
            //curVertex = vec3(curVertex.x, curVertex.y * curScaleB, curVertex.z * curScaleA);
        }



        if (faceDir == FACE_UP_INDEX) {
            int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.upFacesStartIndex + chunkFacesMetadata.numUpFaces] = curPositionTemp;
            chunkFacesMetadata.numUpFaces++;
        }

        if (faceDir == FACE_DOWN_INDEX) {
            int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.downFacesStartIndex + chunkFacesMetadata.numDownFaces] = curPositionTemp;
            chunkFacesMetadata.numDownFaces++;
        }

        if (faceDir == FACE_FRONT_INDEX) {
            int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.frontFacesStartIndex + chunkFacesMetadata.numFrontFaces] = curPositionTemp;
            chunkFacesMetadata.numFrontFaces++;
        }

        if (faceDir == FACE_BACK_INDEX) {
            int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.backFacesStartIndex + chunkFacesMetadata.numBackFaces] = curPositionTemp;
            chunkFacesMetadata.numBackFaces++;
        }

        if (faceDir == FACE_RIGHT_INDEX) {
            int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.rightFacesStartIndex + chunkFacesMetadata.numRightFaces] = curPositionTemp;
            chunkFacesMetadata.numRightFaces++;
        }

        if (faceDir == FACE_LEFT_INDEX) {
            int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);

            chunkMeshData[chunkFacesMetadata.leftFacesStartIndex + chunkFacesMetadata.numLeftFaces] = curPositionTemp;
            chunkFacesMetadata.numLeftFaces++;
        }
    }
}

static void GreedyMeshCurFaceDir(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunkCurDirOnlyVisible, int curLodLevel, std::vector<GreedyMeshCurShape>& greedyMeshForCurFace, int faceDir) {

    PROFILE_FUNCTION();

    int scale = pow(2, curLodLevel);
    //int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    int powerOfTwo = pow(2, curLodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;

    int stepSizeForConvolution = powerOfTwo;

    bool doGreedyMeshing = true;

    if (doGreedyMeshing)
    {
        for (int y = startY; y <= endY; y += stepSizeForConvolution)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int x = startX; x <= endX; x += stepSizeForConvolution)
                {
                    float curNoise = noiseForCurrentChunkCurDirOnlyVisible[x][y][z];

                    if (curNoise == 1 || curNoise == 0)
                    {
                        GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };

                        if (faceDir == FACE_UP_INDEX) {

                            bool expandedX = false;
                            if (true) {

                                int length = 0;
                                int lastValidX = x;

                                for (int iterateXToExpand = x; iterateXToExpand <= endX; iterateXToExpand += stepSizeForConvolution)
                                {
                                    float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[iterateXToExpand][y][z];
                                    if (noiseForCheckingVoxel != 2) {
                                        length += scale;
                                        expandedX = true;
                                        lastValidX = iterateXToExpand;
                                    }
                                    else {
                                        x = lastValidX;
                                        break;
                                    }
                                }

                                if (expandedX) {
                                    curShape.endPos = Vector3{ (float)lastValidX, (float)y, (float)z };
                                    curShape.scale = Vector3{ (float)length, curShape.scale.y, curShape.scale.z };
                                }
                            }

                            if (true)
                            {
                                int breadth = 0;
                                int lastValidZ = z;
                                for (int iterateZToExpand = z; iterateZToExpand <= endZ; iterateZToExpand += stepSizeForConvolution)
                                {
                                    bool expandedShape = true;
                                    for (int i = curShape.startPos.x; i <= curShape.endPos.x; i += stepSizeForConvolution)
                                    {
                                        float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[i][y][iterateZToExpand];
                                        if (noiseForCheckingVoxel == 2) {
                                            expandedShape = false;
                                            break;
                                        }
                                    }

                                    if (expandedShape)
                                    {
                                        breadth += scale;
                                        lastValidZ = iterateZToExpand;

                                        for (int i = curShape.startPos.x; i <= curShape.endPos.x; i += stepSizeForConvolution)
                                        {
                                            noiseForCurrentChunkCurDirOnlyVisible[i][y][iterateZToExpand] = 2.0f;
                                        }

                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                curShape.endPos = Vector3{ (float)curShape.endPos.x, (float)y, (float)lastValidZ };
                                curShape.scale = Vector3{ (float)curShape.scale.x, curShape.scale.y, (float)breadth };
                            }
                        }

                        if (faceDir == FACE_RIGHT_INDEX) {

                            bool expandedZ = false;
                            if (true) {

                                int breadth = 0;
                                int lastValidZ = z;

                                for (int iterateZToExpand = z; iterateZToExpand <= endZ; iterateZToExpand += stepSizeForConvolution)
                                {
                                    float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[x][y][iterateZToExpand];
                                    if (noiseForCheckingVoxel != 2) {
                                        breadth += scale;
                                        expandedZ = true;
                                        lastValidZ = iterateZToExpand;
                                    }
                                    else {
                                        z = lastValidZ;
                                        break;
                                    }
                                }

                                if (expandedZ) {
                                    curShape.endPos = Vector3{ (float)x, (float)y, (float)lastValidZ };
                                    curShape.scale = Vector3{ (float)curShape.scale.x, curShape.scale.y, (float)breadth };
                                }
                            }

                            if (true)
                            {
                                int height = 0;
                                int lastValidY = y;
                                for (int iterateYToExpand = y; iterateYToExpand <= endZ; iterateYToExpand += stepSizeForConvolution)
                                {
                                    bool expandedShape = true;
                                    for (int i = curShape.startPos.z; i <= curShape.endPos.z; i += stepSizeForConvolution)
                                    {
                                        float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[x][iterateYToExpand][i];
                                        if (noiseForCheckingVoxel == 2) {
                                            expandedShape = false;
                                            break;
                                        }
                                    }

                                    if (expandedShape)
                                    {
                                        height += scale;
                                        lastValidY = iterateYToExpand;

                                        for (int i = curShape.startPos.z; i <= curShape.endPos.z; i += stepSizeForConvolution)
                                        {
                                            noiseForCurrentChunkCurDirOnlyVisible[x][iterateYToExpand][i] = 2.0f;
                                        }

                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                curShape.endPos = Vector3{ (float)x, (float)lastValidY, (float)curShape.endPos.z };
                                curShape.scale = Vector3{ (float)curShape.scale.x, (float)height, (float)curShape.scale.z };
                            }
                        }




                        greedyMeshForCurFace.push_back(curShape);
                    }
                }
            }
        }
    }

    if (false)
    {
        if (faceDir == FACE_UP_INDEX) {

            for (int y = startY; y <= endY; y += stepSizeForConvolution)
            {
                GreedyMeshCurShape curShapeForThisPlane = { Vector3{(float)0, (float)y, (float)0},  Vector3{(float)0, (float)y, (float)0}, Vector3{ (float)scale, (float)scale, (float)scale } };

                std::vector<GreedyMeshCurShape> greedyMeshMaskForCurPlane;

                for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
                {
                    for (int x = startX; x <= endX; x += stepSizeForConvolution)
                    {
                        float curNoise = noiseForCurrentChunkCurDirOnlyVisible[x][y][z];

                        if (curNoise != 2)
                        {
                            GreedyMeshCurShape curMask = { Vector3{(float)x, (float)y, (float)z}, Vector3{(float)x, (float)y, (float)z}, Vector3{(float)scale, (float)scale, (float)scale} };

                            int length = 0;
                            int lastValidX = x;

                            bool expandedX = false;

                            for (int iterateXToExpand = x; iterateXToExpand <= endX; iterateXToExpand += stepSizeForConvolution)
                            {
                                float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[iterateXToExpand][y][z];
                                if (noiseForCheckingVoxel != 2) {
                                    length += scale;
                                    expandedX = true;
                                    lastValidX = iterateXToExpand;
                                }
                                else {
                                    x = lastValidX;
                                    break;
                                }
                            }

                            if (expandedX) {
                                curMask.endPos = Vector3{ (float)lastValidX, (float)y, (float)z };
                                curMask.scale = Vector3{ (float)length, curMask.scale.y, curMask.scale.z };
                            }

                            greedyMeshMaskForCurPlane.push_back(curMask);
                        }

                    }

                    for (int i = 0; i < greedyMeshMaskForCurPlane.size(); i++)
                    {
                        for (int j = greedyMeshMaskForCurPlane[i].startPos.x; j < greedyMeshMaskForCurPlane[i].endPos.x; j++)
                        {

                        }
                    }
                }

            }
        }
    }

    if (!doGreedyMeshing) {
        for (int x = startX; x <= endX; x += stepSizeForConvolution)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int y = startY; y <= endY; y += stepSizeForConvolution)
                {
                    float curNoise = noiseForCurrentChunkCurDirOnlyVisible[x][y][z];

                    if (curNoise == 1 || curNoise == 0)
                    {

                        GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };
                        greedyMeshForCurFace.push_back(curShape);

                    }



                }
            }
        }
    }
 
    if (false) {

        for (int x = startX; x <= endX; x += stepSizeForConvolution)
        {
            int curX = x;
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int y = startY; y <= endY; y += stepSizeForConvolution)
                {
                    float curNoise = noiseForCurrentChunkCurDirOnlyVisible[x][y][z];

                    if (curNoise == 1 || curNoise == 0)
                    {

                        GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };

                        if (false)
                        {
                            if (faceDir == FACE_UP_INDEX || faceDir == FACE_DOWN_INDEX) {
                                for (int iterateXToExpand = x; iterateXToExpand <= endX; iterateXToExpand += stepSizeForConvolution)
                                {
                                    float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[iterateXToExpand][y][z];
                                    if (noiseForCheckingVoxel == 1 || noiseForCheckingVoxel == 0) {
                                        curShape.endPos = Vector3{ (float)iterateXToExpand, (float)y, (float)z };
                                        curShape.scale = Vector3{ (float)(abs(abs(iterateXToExpand) - abs(x)) + 1), curShape.scale.y, curShape.scale.z };
                                        if (curShape.scale.x == 0) {
                                            curShape.scale.x = scale;
                                        }
                                    }
                                    else {
                                        x = iterateXToExpand;
                                        break;
                                    }
                                }
                            }

                            if (false)
                            {
                                for (int iterateZToExpand = z; iterateZToExpand <= endZ; iterateZToExpand += stepSizeForConvolution)
                                {
                                    bool shapeCouldNotBeCompleted = false;
                                    for (int i = curShape.startPos.x; i < curShape.endPos.x; i += stepSizeForConvolution)
                                    {
                                        float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[i][y][iterateZToExpand];
                                        if (noiseForCheckingVoxel == 1 || noiseForCheckingVoxel == 0) {
                                        }
                                        else {
                                            shapeCouldNotBeCompleted = true;
                                            break;
                                        }

                                    }
                                    if (shapeCouldNotBeCompleted)
                                    {
                                        //z = iterateZToExpand;
                                        break;
                                    }
                                    else {
                                        curShape.endPos = Vector3{ (float)curShape.endPos.x, (float)y, (float)iterateZToExpand };
                                        curShape.scale = Vector3{ (float)curShape.scale.x, curShape.scale.y, (float)(iterateZToExpand - z + 1) };
                                    }
                                }
                            }

                            if (false)
                            {
                                for (int iterateYToExpand = y; iterateYToExpand <= endY; iterateYToExpand += stepSizeForConvolution)
                                {
                                    bool shapeCouldNotBeCompleted = false;
                                    for (int i = curShape.startPos.x; i < curShape.endPos.x; i += stepSizeForConvolution)
                                    {
                                        for (int j = curShape.startPos.z; j < curShape.endPos.z; j += stepSizeForConvolution)
                                        {
                                            float noiseForCheckingVoxel = noiseForCurrentChunkCurDirOnlyVisible[i][y][j];
                                            if (noiseForCheckingVoxel == 1 || noiseForCheckingVoxel == 0) {
                                            }
                                            else {
                                                shapeCouldNotBeCompleted = true;
                                                break;
                                            }
                                        }
                                        if (shapeCouldNotBeCompleted)
                                        {
                                            //z = iterateZToExpand;
                                            break;
                                        }
                                    }
                                    if (shapeCouldNotBeCompleted)
                                    {
                                        //z = iterateZToExpand;
                                        break;
                                    }
                                    else {
                                        curShape.endPos = Vector3{ (float)curShape.endPos.x, (float)iterateYToExpand, (float)curShape.endPos.z };
                                        curShape.scale = Vector3{ (float)curShape.scale.x, (float)(iterateYToExpand - y + 1), (float)curShape.scale.z };
                                    }

                                }
                            }
                        }

                        greedyMeshForCurFace.push_back(curShape);

                    }



                }
            }

            x = curX;
        }
    }

}

static void BinaryGreedyMeshCurFaceDir(std::vector<std::vector<uint64_t>>& bitNoiseForCurrentChunkCurDirOnlyVisible, int curLodLevel, std::vector<GreedyMeshCurShape>& greedyMeshForCurFace, int faceDir) {

    PROFILE_FUNCTION();

    int scale = pow(2, curLodLevel);
    //int curChunkIndexInBigArray = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

    int powerOfTwo = pow(2, curLodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;


    int stepSizeForConvolution = powerOfTwo;
    int innerLoopEnd = bitNoiseForCurrentChunkCurDirOnlyVisible.size();

    bool doGreedyMeshing = true;

    if (doGreedyMeshing)
    {
        if (faceDir == FACE_UP_INDEX || faceDir == FACE_DOWN_INDEX)
        {
            for (int y = startY; y <= endY; y += stepSizeForConvolution)
            {
                for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
                {
                    for (int x = startX; x <= endX; x += stepSizeForConvolution)
                    {
                        //uint64_t maskForCurFace = static_cast<uint64_t>(1) << x;
                        uint64_t maskForCurFace = static_cast<uint64_t>(1) << x;
                        uint64_t curNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] & maskForCurFace;

                        if (curNoise > 0)
                        {
                            GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };
                            //GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ 0.0f, 0.0f, 0.0f } };

                            if (true)
                            {
                                uint64_t curCheckingMaskX = 0;
                                int curXHead = std::countr_zero(bitNoiseForCurrentChunkCurDirOnlyVisible[y][z]);

                                int cumulativeScaleX = 0;

                                for (int i = curXHead; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t bitPosition = i;
                                    bool curXIsFilled = ((bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] >> bitPosition) & 1) > 0;
                                    if (curXIsFilled) {
                                        cumulativeScaleX += scale;
                                        curCheckingMaskX = curCheckingMaskX | static_cast<uint64_t>(1) << bitPosition; // Update Mask
                                        uint64_t maskToSetCurVoxelToEmpty = ~(static_cast<uint64_t>(1) << bitPosition);
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] = bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] & maskToSetCurVoxelToEmpty; // Make noise value 0 so that it is ignored during the next loop.
                                    }
                                    else {
                                        x = i;
                                        curShape.scale.x = cumulativeScaleX;
                                        break;
                                    }
                                }

                                int cumulativeScaleZ = scale;
                                for (int i = z + stepSizeForConvolution; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t noisesForCurZ = bitNoiseForCurrentChunkCurDirOnlyVisible[y][i];
                                    bool noisesIncludesMask = (noisesForCurZ & curCheckingMaskX) == curCheckingMaskX;

                                    if (noisesIncludesMask) {
                                        cumulativeScaleZ += scale;
                                        uint64_t modifiedNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[y][i] & ~(curCheckingMaskX);
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[y][i] = modifiedNoise; // remove noise from data since previous face was expanded here.
                                    }
                                    else {
                                        curShape.scale.z = cumulativeScaleZ;
                                        break;
                                    }
                                }
                            }
                            greedyMeshForCurFace.push_back(curShape);
                        }
                    }
                }
            }
        }
        else if(faceDir == FACE_FRONT_INDEX || faceDir == FACE_BACK_INDEX)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
               for (int y = startY; y <= endY; y += stepSizeForConvolution)
               {
                    for (int x = startX; x <= endX; x += stepSizeForConvolution)
                    {
                        uint64_t maskForCurFace = static_cast<uint64_t>(1) << x;
                        uint64_t curNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[z][y] & maskForCurFace;

                        if (curNoise > 0)
                        {
                            GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };

                            if (true)
                            {
                                uint64_t curCheckingMaskX = 0;
                                int curXHead = std::countr_zero(bitNoiseForCurrentChunkCurDirOnlyVisible[z][y]);

                                int cumulativeScaleX = 0;

                                for (int i = curXHead; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t bitPosition = i;
                                    bool curXIsFilled = ((bitNoiseForCurrentChunkCurDirOnlyVisible[z][y] >> bitPosition) & 1) > 0;
                                    if (curXIsFilled) {
                                        cumulativeScaleX += scale;
                                        curCheckingMaskX = curCheckingMaskX | static_cast<uint64_t>(1) << bitPosition; // Update Mask
                                        uint64_t maskToSetCurVoxelToEmpty = ~(static_cast<uint64_t>(1) << bitPosition);
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[z][y] = bitNoiseForCurrentChunkCurDirOnlyVisible[z][y] & maskToSetCurVoxelToEmpty; // Make noise value 0 so that it is ignored during the next loop.
                                    }
                                    else {
                                        x = i;
                                        curShape.scale.x = cumulativeScaleX;
                                        break;
                                    }
                                }

                                int cumulativeScaleY = scale;
                                for (int i = y + stepSizeForConvolution; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t noisesForCurY = bitNoiseForCurrentChunkCurDirOnlyVisible[z][i];
                                    bool noisesIncludesMask = (noisesForCurY & curCheckingMaskX) == curCheckingMaskX;

                                    if (noisesIncludesMask) {
                                        cumulativeScaleY += scale;
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[z][i] = bitNoiseForCurrentChunkCurDirOnlyVisible[z][i] & ~(curCheckingMaskX); // remove noise from data since previous face was expanded here.
                                    }
                                    else {
                                        curShape.scale.y = cumulativeScaleY;
                                        break;
                                    }
                                }
                            }

                            greedyMeshForCurFace.push_back(curShape);
                        }
                    }
                }
            }
        }
        else if (faceDir == FACE_RIGHT_INDEX || faceDir == FACE_LEFT_INDEX)
        {
            for (int x = startX; x <= endX; x += stepSizeForConvolution)
            {
                for (int y = startY; y <= endY; y += stepSizeForConvolution)
                {
                    for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
                    {
                        //uint64_t maskForCurFace = static_cast<uint64_t>(1) << x;
                        uint64_t maskForCurFace = static_cast<uint64_t>(1) << z;
                        uint64_t curNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[x][y] & maskForCurFace;

                        if (curNoise > 0)
                        {
                            GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };
                            //GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ 0.0f, 0.0f, 0.0f } };

                            if (true)
                            {
                                uint64_t curCheckingMaskZ = 0;
                                int curZHead = std::countr_zero(bitNoiseForCurrentChunkCurDirOnlyVisible[x][y]);

                                int cumulativeScaleZ = 0;

                                for (int i = curZHead; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t bitPosition = i;
                                    bool curXIsFilled = ((bitNoiseForCurrentChunkCurDirOnlyVisible[x][y] >> bitPosition) & 1) > 0;
                                    if (curXIsFilled) {
                                        cumulativeScaleZ += scale;
                                        curCheckingMaskZ = curCheckingMaskZ | static_cast<uint64_t>(1) << bitPosition; // Update Mask
                                        uint64_t maskToSetCurVoxelToEmpty = ~(static_cast<uint64_t>(1) << bitPosition);
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[x][y] = bitNoiseForCurrentChunkCurDirOnlyVisible[x][y] & maskToSetCurVoxelToEmpty; // Make noise value 0 so that it is ignored during the next loop.
                                    }
                                    else {
                                        z = i;
                                        curShape.scale.z = cumulativeScaleZ;
                                        break;
                                    }
                                }

                                int cumulativeScaleY = scale;
                                for (int i = y + stepSizeForConvolution; i < innerLoopEnd; i += stepSizeForConvolution)
                                {
                                    uint64_t noisesForCurY = bitNoiseForCurrentChunkCurDirOnlyVisible[x][i];
                                    bool noisesIncludesMask = (noisesForCurY & curCheckingMaskZ) == curCheckingMaskZ;

                                    if (noisesIncludesMask) {
                                        cumulativeScaleY += scale;
                                        bitNoiseForCurrentChunkCurDirOnlyVisible[x][i] = bitNoiseForCurrentChunkCurDirOnlyVisible[x][i] & ~(curCheckingMaskZ); // remove noise from data since previous face was expanded here.
                                    }
                                    else {
                                        curShape.scale.y = cumulativeScaleY;
                                        break;
                                    }
                                }
                            }
                            greedyMeshForCurFace.push_back(curShape);
                        }
                    }
                }
            }
        }

        else
        {
            for (int y = startY; y <= endY; y += stepSizeForConvolution)
            {
                for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
                {
                    for (int x = startX; x <= endX; x += stepSizeForConvolution)
                    {
                        uint64_t maskForCurFace = static_cast<uint64_t>(1) << x;
                        uint64_t curNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] & maskForCurFace;

                        if (curNoise > 0)
                        {
                            GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };
                            greedyMeshForCurFace.push_back(curShape);
                        }
                    }
                }
            }

        }

    }


    if (!doGreedyMeshing) {

        for (int y = startY; y <= endY; y += stepSizeForConvolution)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int x = startX; x <= endX; x += stepSizeForConvolution)
                {
                    unsigned int maskForCurFace = 1 << x;
                    unsigned int curNoise = bitNoiseForCurrentChunkCurDirOnlyVisible[y][z] & maskForCurFace;

                    if (curNoise > 0)
                    {
                        GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };
                        greedyMeshForCurFace.push_back(curShape);

                    }



                }
            }
        }
    }
}

//Can be multithreaded to increase performance by doing one thread per face direction.
static void GreedyMesh2D(std::vector<std::vector<std::vector<float>>>& noiseForCurrentChunk
    , std::vector<std::vector<std::vector<float>>>& scaleForEachVoxelInChunk
    , std::vector<int>& chunkMeshData
    , ChunkFacesMetadata& chunkFacesMetadata
    , VertexPositions& megaVertPositions
    , Vector3 innerChunkIndex
    , Vector3 chunkIndex
    , int curLodLevel)
{
    PROFILE_FUNCTION();

    chunkMeshData = std::vector<int>(totalNumVoxelsPerChunkWorstCase * NUM_FACES);

    chunkFacesMetadata.upFacesStartIndex = FACE_UP_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.downFacesStartIndex = FACE_DOWN_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.frontFacesStartIndex = FACE_FRONT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.backFacesStartIndex = FACE_BACK_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.rightFacesStartIndex = FACE_RIGHT_INDEX * totalNumVoxelsPerChunkWorstCase;
    chunkFacesMetadata.leftFacesStartIndex = FACE_LEFT_INDEX * totalNumVoxelsPerChunkWorstCase;

    int extraVoxelsToCompute = 2 * (pow(2, curLodLevel) + 1);

    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkUpFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkDownFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkFrontFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkBackFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkRightFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));
    std::vector<std::vector<std::vector<float>>> noiseForCurrentChunkLeftFace(chunkSize + extraVoxelsToCompute, std::vector<std::vector<float>>(chunkSize + extraVoxelsToCompute, std::vector<float>(chunkSize + extraVoxelsToCompute)));

    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkUpFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));
    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkDownFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));
    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkFrontFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));
    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkBackFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));
    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkRightFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));
    std::vector<std::vector<uint64_t>> bitNoiseForCurrentChunkLeftFace(chunkSize + extraVoxelsToCompute, std::vector<uint64_t>(chunkSize + extraVoxelsToCompute, 0));

    std::vector<GreedyMeshCurShape> greedyMeshForUpFace;
    std::vector<GreedyMeshCurShape> greedyMeshForDownFace;
    std::vector<GreedyMeshCurShape> greedyMeshForFrontFace;
    std::vector<GreedyMeshCurShape> greedyMeshForBackFace;
    std::vector<GreedyMeshCurShape> greedyMeshForRightFace;
    std::vector<GreedyMeshCurShape> greedyMeshForLeftFace;

    int scale = pow(2, curLodLevel);

    int powerOfTwo = pow(2, curLodLevel);
    int startX = powerOfTwo;
    int endX = chunkSize;

    int startY = powerOfTwo;
    int endY = chunkSize;

    int startZ = powerOfTwo;
    int endZ = chunkSize;

    int stepSizeForConvolution = powerOfTwo;

    bool binaryMeshing = true;

    if (!binaryMeshing) {
        for (int x = startX; x <= endX; x += stepSizeForConvolution)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int y = startY; y <= endY; y += stepSizeForConvolution)
                {
                    float curNoise = noiseForCurrentChunk[x][y][z];

                    if (curNoise == 1 || curNoise == 0) {

                        GreedyMeshCurShape curShape = { Vector3{(float)x, (float)y, (float)z},  Vector3{(float)x, (float)y, (float)z}, Vector3{ (float)scale, (float)scale, (float)scale } };

                        float curNoiseTop = noiseForCurrentChunk[x][y + stepSizeForConvolution][z];
                        if (curNoiseTop == 2) {
                            noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkUpFace[x][y][z] = 2;
                        }

                        float curNoiseBottom = noiseForCurrentChunk[x][y - stepSizeForConvolution][z];
                        if (curNoiseBottom == 2) {
                            noiseForCurrentChunkDownFace[x][y][z] = curNoise;
                            //greedyMeshForDownFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkDownFace[x][y][z] = 2;
                        }

                        float curNoiseFront = noiseForCurrentChunk[x][y][z + stepSizeForConvolution];
                        if (curNoiseFront == 2) {
                            noiseForCurrentChunkFrontFace[x][y][z] = curNoise;
                            //greedyMeshForFrontFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkFrontFace[x][y][z] = 2;
                        }

                        float curNoiseBack = noiseForCurrentChunk[x][y][z - stepSizeForConvolution];
                        if (curNoiseBack == 2) {
                            noiseForCurrentChunkBackFace[x][y][z] = curNoise;
                            //greedyMeshForBackFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkBackFace[x][y][z] = 2;
                        }

                        float curNoiseRight = noiseForCurrentChunk[x + stepSizeForConvolution][y][z];
                        if (curNoiseRight == 2) {
                            noiseForCurrentChunkRightFace[x][y][z] = curNoise;
                            //greedyMeshForRightFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkRightFace[x][y][z] = 2;
                        }

                        float curNoiseLeft = noiseForCurrentChunk[x - stepSizeForConvolution][y][z];
                        if (curNoiseLeft == 2) {
                            noiseForCurrentChunkLeftFace[x][y][z] = curNoise;
                            //greedyMeshForLeftFace.push_back(curShape);
                        }
                        else
                        {
                            noiseForCurrentChunkLeftFace[x][y][z] = 2;
                        }


                    }
                    else {
                        noiseForCurrentChunkUpFace[x][y][z] = 2;
                        noiseForCurrentChunkDownFace[x][y][z] = 2;
                        noiseForCurrentChunkFrontFace[x][y][z] = 2;
                        noiseForCurrentChunkBackFace[x][y][z] = 2;
                        noiseForCurrentChunkRightFace[x][y][z] = 2;
                        noiseForCurrentChunkLeftFace[x][y][z] = 2;
                    }

                }
            }
        }
    }

    if (binaryMeshing) {
        for (int y = startY; y <= endY; y += stepSizeForConvolution)
        {
            for (int z = startZ; z <= endZ; z += stepSizeForConvolution)
            {
                for (int x = startX; x <= endX; x += stepSizeForConvolution)
                {
                    float curNoise = noiseForCurrentChunk[x][y][z];

                    if (curNoise == 1 || curNoise == 0) {

                        float curNoiseTop = noiseForCurrentChunk[x][y + stepSizeForConvolution][z];
                        if (curNoiseTop == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkUpFace[y][z];
                            bitNoiseForCurrentChunkUpFace[y][z] = curPlaneFacesData | (static_cast<uint64_t>(1) << x);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkUpFace[y][z];
                        //    bitNoiseForCurrentChunkUpFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}

                        float curNoiseBottom = noiseForCurrentChunk[x][y - stepSizeForConvolution][z];
                        if (curNoiseBottom == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkDownFace[y][z];
                            bitNoiseForCurrentChunkDownFace[y][z] = curPlaneFacesData | (static_cast<uint64_t>(1) << x);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkDownFace[y][z];
                        //    bitNoiseForCurrentChunkDownFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}

                        float curNoiseFront = noiseForCurrentChunk[x][y][z + stepSizeForConvolution];
                        if (curNoiseFront == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkFrontFace[z][y];
                            bitNoiseForCurrentChunkFrontFace[z][y] = curPlaneFacesData | (static_cast<uint64_t>(1) << x);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkFrontFace[y][z];
                        //    bitNoiseForCurrentChunkFrontFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}

                        float curNoiseBack = noiseForCurrentChunk[x][y][z - stepSizeForConvolution];
                        if (curNoiseBack == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkBackFace[z][y];
                            bitNoiseForCurrentChunkBackFace[z][y] = curPlaneFacesData | (static_cast<uint64_t>(1) << x);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkBackFace[y][z];
                        //    bitNoiseForCurrentChunkBackFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}

                        float curNoiseRight = noiseForCurrentChunk[x + stepSizeForConvolution][y][z];
                        if (curNoiseRight == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkRightFace[x][y];
                            bitNoiseForCurrentChunkRightFace[x][y] = curPlaneFacesData | (static_cast<uint64_t>(1) << z);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkRightFace[y][z];
                        //    bitNoiseForCurrentChunkRightFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}

                        float curNoiseLeft = noiseForCurrentChunk[x - stepSizeForConvolution][y][z];
                        if (curNoiseLeft == 2) {
                            uint64_t curPlaneFacesData = bitNoiseForCurrentChunkLeftFace[x][y];
                            bitNoiseForCurrentChunkLeftFace[x][y] = curPlaneFacesData | (static_cast<uint64_t>(1) << z);
                            //noiseForCurrentChunkUpFace[x][y][z] = curNoise;
                            //greedyMeshForUpFace.push_back(curShape);
                        }
                        //else
                        //{
                        //    int curPlaneFacesData = bitNoiseForCurrentChunkLeftFace[y][z];
                        //    bitNoiseForCurrentChunkLeftFace[y][z] = curPlaneFacesData & ~(1 << x);
                        //}


                    }
                    //else {

                    //    int curPlaneFacesData = bitNoiseForCurrentChunkUpFace[y][z];
                    //    bitNoiseForCurrentChunkUpFace[y][z] = curPlaneFacesData & ~(1 << x);

                    //    curPlaneFacesData = bitNoiseForCurrentChunkDownFace[y][z];
                    //    bitNoiseForCurrentChunkDownFace[y][z] = curPlaneFacesData & ~(1 << x);

                    //    curPlaneFacesData = bitNoiseForCurrentChunkFrontFace[y][z];
                    //    bitNoiseForCurrentChunkFrontFace[y][z] = curPlaneFacesData & ~(1 << x);

                    //    curPlaneFacesData = bitNoiseForCurrentChunkBackFace[y][z];
                    //    bitNoiseForCurrentChunkBackFace[y][z] = curPlaneFacesData & ~(1 << x);

                    //    curPlaneFacesData = bitNoiseForCurrentChunkRightFace[y][z];
                    //    bitNoiseForCurrentChunkRightFace[y][z] = curPlaneFacesData & ~(1 << x);

                    //    curPlaneFacesData = bitNoiseForCurrentChunkLeftFace[y][z];
                    //    bitNoiseForCurrentChunkLeftFace[y][z] = curPlaneFacesData & ~(1 << x);
                    //}

                }
            }
        }
    }

    if (!binaryMeshing) {
        GreedyMeshCurFaceDir(noiseForCurrentChunkUpFace, curLodLevel, greedyMeshForUpFace, FACE_UP_INDEX);
        GreedyMeshCurFaceDir(noiseForCurrentChunkDownFace, curLodLevel, greedyMeshForDownFace, FACE_DOWN_INDEX);
        GreedyMeshCurFaceDir(noiseForCurrentChunkFrontFace, curLodLevel, greedyMeshForFrontFace, FACE_FRONT_INDEX);
        GreedyMeshCurFaceDir(noiseForCurrentChunkBackFace, curLodLevel, greedyMeshForBackFace, FACE_BACK_INDEX);
        GreedyMeshCurFaceDir(noiseForCurrentChunkRightFace, curLodLevel, greedyMeshForRightFace, FACE_RIGHT_INDEX);
        GreedyMeshCurFaceDir(noiseForCurrentChunkLeftFace, curLodLevel, greedyMeshForLeftFace, FACE_LEFT_INDEX);
    }
    else
    {
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkUpFace, curLodLevel, greedyMeshForUpFace, FACE_UP_INDEX);
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkDownFace, curLodLevel, greedyMeshForDownFace, FACE_DOWN_INDEX);
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkFrontFace, curLodLevel, greedyMeshForFrontFace, FACE_FRONT_INDEX);
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkBackFace, curLodLevel, greedyMeshForBackFace, FACE_BACK_INDEX);
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkRightFace, curLodLevel, greedyMeshForRightFace, FACE_RIGHT_INDEX);
        BinaryGreedyMeshCurFaceDir(bitNoiseForCurrentChunkLeftFace, curLodLevel, greedyMeshForLeftFace, FACE_LEFT_INDEX);

    }


    UpdateMeshFromGreedyShape(greedyMeshForUpFace, chunkMeshData, chunkFacesMetadata, FACE_UP_INDEX, stepSizeForConvolution);
    UpdateMeshFromGreedyShape(greedyMeshForDownFace, chunkMeshData, chunkFacesMetadata, FACE_DOWN_INDEX, stepSizeForConvolution);
    UpdateMeshFromGreedyShape(greedyMeshForFrontFace, chunkMeshData, chunkFacesMetadata, FACE_FRONT_INDEX, stepSizeForConvolution);
    UpdateMeshFromGreedyShape(greedyMeshForBackFace, chunkMeshData, chunkFacesMetadata, FACE_BACK_INDEX, stepSizeForConvolution);
    UpdateMeshFromGreedyShape(greedyMeshForRightFace, chunkMeshData, chunkFacesMetadata, FACE_RIGHT_INDEX, stepSizeForConvolution);
    UpdateMeshFromGreedyShape(greedyMeshForLeftFace, chunkMeshData, chunkFacesMetadata, FACE_LEFT_INDEX, stepSizeForConvolution);

}


void PlaneFacingDir(Vector3 dir, GenerativeMesh &curMesh) {

    PROFILE_FUNCTION();

    int numVertices = 4;
    curMesh.mesh.vertices = (float*)MemAlloc(numVertices * 3 * sizeof(float));
    curMesh.mesh.texcoords = (float*)MemAlloc(numVertices * 2 * sizeof(float));
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
    curMesh.mesh.vertexCount = numVertices;

    UploadMesh(&curMesh.mesh, false);
}

void PlaneFacingDirTriangle(Vector3 dir, GenerativeMesh &curMesh) {

    PROFILE_FUNCTION();

    int numVertices = 3;
    curMesh.mesh.vertices = (float*)MemAlloc(numVertices * 3 * sizeof(float));
    curMesh.mesh.texcoords = (float*)MemAlloc(numVertices * 2 * sizeof(float));

    FaceVerticesTopTriangle(curMesh.mesh.vertices, 0, 0, 0);

    TexCoordsTriangle(curMesh.mesh.texcoords);

    curMesh.mesh.triangleCount = 1;
    curMesh.mesh.vertexCount = numVertices;

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