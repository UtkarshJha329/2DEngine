
#include <iostream>
#include <chrono>
#include <future>
#include <filesystem>

#include <queue>

#include "flecs/flecs.h"

#define GLSL_VERSION            430
#define GRAPHICS_API_OPENGL_43
#define RLGL_RENDER_TEXTURES_HINT
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

#define FORCE_DEDICATED_GPU 1
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
    , std::vector<int>& chunkMeshData
    , ChunkFacesMetadata& chunkFacesMetadata
    , VertexPositions &megaVertPositions
    , Vector3 rescopedChunkIndex
    , Vector3 chunkIndex
    , int curLodLevel);

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

    std::vector<std::vector<std::vector<float>>> _noiseForCurChunk(chunkSize + 3, std::vector<std::vector<float>>(chunkSize + 3, std::vector<float>(chunkSize + 3)));
    MakeNoiseForChunk(_noiseForCurChunk, chunkIndex.x, chunkIndex.y, chunkIndex.z, numChunksFullWidth, numChunksFullWidth_Y, chunkSize, scale);

    ConvoluteNoise(_noiseForCurChunk, curLodLevel);

    std::vector<int> chunkMeshData;
    ChunkFacesMetadata chunkFacesMetadata;
    GenMeshCustom2D(_noiseForCurChunk, chunkMeshData, chunkFacesMetadata, megaVertPositions, innerChunkIndex, chunkIndex, curLodLevel);

    {
        {
            std::lock_guard<std::mutex> lockChunkMappingAndAllocatingMutex(chunkMappingAllocatingMutex);

            if (reclaimPreviousMemory) {
                //int chunkFlatPos = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);

                //std::cout << "Reclaim Memory." << std::endl;
                megaVertPositions.ClearChunkData(innerChunkIndex);

                //megaVertPositions.ReclaimMemory(megaVertPositions.upFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.upFacesMetadata[chunkFlatPos].size);
                //megaVertPositions.ReclaimMemory(megaVertPositions.downFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.downFacesMetadata[chunkFlatPos].size);
                //megaVertPositions.ReclaimMemory(megaVertPositions.frontFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.frontFacesMetadata[chunkFlatPos].size);
                //megaVertPositions.ReclaimMemory(megaVertPositions.backFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.backFacesMetadata[chunkFlatPos].size);
                //megaVertPositions.ReclaimMemory(megaVertPositions.rightFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.rightFacesMetadata[chunkFlatPos].size);
                //megaVertPositions.ReclaimMemory(megaVertPositions.leftFacesMetadata[chunkFlatPos].startPositionInBigArray, megaVertPositions.leftFacesMetadata[chunkFlatPos].size);
            }

            for (int i = 0; i < NUM_FACES; i++)
            {
                megaVertPositions.CopyDataToMegaArray(megaVertPositions.megaArrayOfAllPositions/*, megaVertPositions.nextToFill*//*megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + (i * totalNumVoxelsPerChunkWorstCase)*/
                                                    , chunkMeshData, i * totalNumVoxelsPerChunkWorstCase, chunkFacesMetadata.GetSizeOfFaceDirPositions(i)
                                                    , chunkFacesMetadata.GetAppropriateStartIndexBasedOnFaceDir(i));

                //if (reclaimPreviousMemory) {
                //    std::cout << chunkFacesMetadata.GetAppropriateStartIndexBasedOnFaceDir(i) << " : " << chunkFacesMetadata.GetSizeOfFaceDirPositions(i) << std::endl;
                //}
                //std::cout << *chunkFacesMetadata.GetAppropriateStartIndexBasedOnFaceDir(i) << " : " << chunkFacesMetadata.GetSizeOfFaceDirPositions(i) << std::endl;
            }

            //std::cout << *chunkFacesMetadata.GetAppropriateStartIndexBasedOnFaceDir(0) << std::endl;

            //chunkFacesMetadata.upFacesStartIndex += chunkIndexFlattenedWithVoxels;
            //chunkFacesMetadata.downFacesStartIndex += chunkIndexFlattenedWithVoxels;
            //chunkFacesMetadata.frontFacesStartIndex += chunkIndexFlattenedWithVoxels;
            //chunkFacesMetadata.backFacesStartIndex += chunkIndexFlattenedWithVoxels;
            //chunkFacesMetadata.rightFacesStartIndex += chunkIndexFlattenedWithVoxels;
            //chunkFacesMetadata.leftFacesStartIndex += chunkIndexFlattenedWithVoxels;

            megaVertPositions.MapChunkMemoryToBigArray(innerChunkIndex, chunkFacesMetadata);

            //megaVertPositions.totalFilled += chunkFacesMetadata.numUpFaces
            //                                + chunkFacesMetadata.numDownFaces
            //                                + chunkFacesMetadata.numFrontFaces
            //                                + chunkFacesMetadata.numBackFaces
            //                                + chunkFacesMetadata.numRightFaces
            //                                + chunkFacesMetadata.numLeftFaces;
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


    //for (int i = 0; i < NUM_FACES; i++)
    //{
    //    std::string curChunkSaveFileName = CHUNK_SAVE_STRING(curChunkIndex, i);

    //    std::ifstream is(worldDataDir + curChunkSaveFileName, std::ios::binary);
    //    cereal::BinaryInputArchive iarchive(is);

    //    int start = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + (i * totalNumVoxelsPerChunk);
    //    std::span<int> curChunkSlice(megaVertPositions.megaArrayOfAllPositions.begin() + start, megaVertPositions.GetCurFaceDirChunkDataEndPos(curChunkIndex, i));

    //    iarchive(cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size()));

    //}

    //std::ifstream is(curChunkFileName, std::ios::binary);
    //cereal::BinaryInputArchive iarchive(is);

    //int chunkFlatIndexWithoutVoxels = megaVertPositions.ChunkFlatIndexWithoutVoxels(innerChunkIndex);
    //int start = megaVertPositions.ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
    //std::span<int> curChunkSlice(megaVertPositions.megaArrayOfAllPositions.begin() + start, totalNumVoxelsPerChunk * NUM_FACES);

    //iarchive(cereal::binary_data(curChunkSlice.data(), sizeof(int) * curChunkSlice.size())
    //    , megaVertPositions.upEndVoxelPositions[chunkFlatIndexWithoutVoxels]
    //    , megaVertPositions.downEndVoxelPositions[chunkFlatIndexWithoutVoxels]
    //    , megaVertPositions.frontEndVoxelPositions[chunkFlatIndexWithoutVoxels]
    //    , megaVertPositions.backEndVoxelPositions[chunkFlatIndexWithoutVoxels]
    //    , megaVertPositions.rightEndVoxelPositions[chunkFlatIndexWithoutVoxels]
    //    , megaVertPositions.leftEndVoxelPositions[chunkFlatIndexWithoutVoxels]);

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
std::unordered_map<int, bool> innerIndexWhereNewMeshNeedsToBeCalculated;

const int screenWidth = 1280;
const int screenHeight = 720;

int main()
{
    Instrumentor::Instance().BeginSession("Profile");

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

    for (int i = 0; i < chunkVisibility.size(); i++)
    {
        chunkVisibility[i] = 0;
    }

    Vector3 cameraChunkIndex = { (int)camera.position.x / chunkSize, (int)camera.position.y / chunkSize, (int)camera.position.z / chunkSize };
    Vector3 oldCameraChunkPosition = cameraChunkIndex;
    Vector3 oldCameraPos = camera.position;

    int chunkBeingGeneratedCount = 0;
    bool chunksChanged = false;

    GenerativeMesh renderQuad = { 0 };
    PlaneFacingDir(up, renderQuad);
    renderQuad.instanceVBOID = 0;

    GenerativeMesh cullingRenderQuad = { 0 };
    PlaneFacingDir(up, cullingRenderQuad);
    cullingRenderQuad.instanceVBOID = 0;

    int indirectBufferVBO = 0;

    std::vector<Vector3> renderTraversalOrder;

    for (int y = 0; y < numChunksFullWidth_Y; y++)
    {
        Vector3 chunkIndex = Vector3{ (float)(0), (float)(y), (float)0 };
        renderTraversalOrder.push_back(chunkIndex);
        //std::cout << chunkIndex.x << ", " << chunkIndex.y << ", " << chunkIndex.z << std::endl;
        mappedInnerIndexMap[megaVertPositions.InnerIndexFlattened(chunkIndex)] = chunkIndex;
        innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(chunkIndex)] = true;
        chunksGridCoordinates.push_back(float3{ 0, (float)y, 0 });
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
                chunksGridCoordinates.push_back(float3{ (float)curLayerNum, (float)y, (float)z });
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
                chunksGridCoordinates.push_back(float3{ (float)-curLayerNum, (float)y, (float)z });
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
                chunksGridCoordinates.push_back(float3{ (float)x, (float)y, (float)curLayerNum });
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
                chunksGridCoordinates.push_back(float3{ (float)x, (float)y, (float)-curLayerNum });

            }
        }
        curLayerNum++;
    }

    Shader instanceShader = LoadShader(TextFormat("Shaders/lighting_instancing.vert", GLSL_VERSION),
        TextFormat("Shaders/lighting.frag", GLSL_VERSION));
    // Get instanceShader locations
    instanceShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instanceShader, "mvp");
    instanceShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(instanceShader, "viewPos");
    instanceShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instanceShader, "instanceTransform");

    Shader zPrePassShader = LoadShader(TextFormat("Shaders/ZPrePass.vert", GLSL_VERSION),
        TextFormat("Shaders/ZPrePass.frag", GLSL_VERSION));
    // Get instanceShader locations
    zPrePassShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(zPrePassShader, "mvp");
    zPrePassShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(zPrePassShader, "viewPos");
    zPrePassShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(zPrePassShader, "instanceTransform");

    // Set instanceShader value: ambient light level
    //int ambientLoc = GetShaderLocation(instanceShader, "ambient");
    //float ambientValue[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    //SetShaderValue(instanceShader, ambientLoc, ambientValue, SHADER_UNIFORM_VEC4);

    Material instancedMaterial = LoadMaterialDefault();
    instancedMaterial.shader = instanceShader;
    instancedMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = textureLoad;

    Material zPrePassMaterial = LoadMaterialDefault();
    zPrePassMaterial.shader = zPrePassShader;
    
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
    float numChunks = numChunksHalfWidth;
    SetShaderValue(instanceShader, numChunksLoc, &numChunks, SHADER_UNIFORM_FLOAT);

    int cameraPosLoc = GetShaderLocation(instanceShader, "cameraPos");
    float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(instanceShader, cameraPosLoc, cameraPos, SHADER_UNIFORM_VEC3);

    Image chunksIDImage = LoadImageFromTexture(target.secondColourTexture);
    Image chunksDepthImage = LoadImageFromTexture(target.depthColourTexture);

    Shader cullingShader = LoadShader(TextFormat("Shaders/SimpleShaderCullingTest.vert", GLSL_VERSION),
        TextFormat("Shaders/SimpleShaderCullingTest.frag", GLSL_VERSION));
    //cullingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(cullingShader, "viewPosition");

    SecondRenderTexture rd2D = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    Material screenRenderMaterial = LoadMaterialDefault();
    screenRenderMaterial.shader = cullingShader;

    const int bindDepthTextureAtPosition = 0;
    rlEnableShader(cullingShader.id);
        int textureLocation = rlGetLocationUniform(cullingShader.id, "depthValueTexture");
        rlSetUniform(textureLocation, &bindDepthTextureAtPosition, RL_SHADER_UNIFORM_SAMPLER2D, 1);
    rlDisableShader();

    int cutOffDepthLoc = GetShaderLocation(cullingShader, "cutOffDepth");
    float cutOffDepthValue = 0.25f;
    SetShaderValue(cullingShader, cutOffDepthLoc, &cutOffDepthValue, SHADER_UNIFORM_FLOAT);

    unsigned int chunksGridPosSSBO = rlLoadShaderBuffer(chunksGridCoordinates.size() * sizeof(float3), chunksGridCoordinates.data(), RL_DYNAMIC_DRAW);

    std::vector<int> megaArrayOfAllPositions2;
    std::vector<int> startPositions2;
    std::vector<int> sizes2;
    std::vector<DrawArraysIndirectCommand> drawArraysIndirectCommands2;

    for (int i = 0; i < 6; i++)
    {
        startPositions2.push_back(megaArrayOfAllPositions2.size());
        for (int y = 0; y < 3; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                for (int z = 0; z < 64; z++)
                {
                    int largeChunkCentrePos = x << 12;
                    largeChunkCentrePos = largeChunkCentrePos | y << 6;
                    largeChunkCentrePos = largeChunkCentrePos | z;
                    largeChunkCentrePos = largeChunkCentrePos | (i << 19);
                    largeChunkCentrePos = largeChunkCentrePos | (1 << 22);

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

    while (!WindowShouldClose())
    {

        PROFILE_SCOPE("Game Loop");

        //std::cout << GetMousePosition().x << ", " << GetMousePosition().y << std::endl;

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
            randValue = randValue > 4 ? 0 : randValue;
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
                if (LODBorderMesh(renderTraversalOrder[i]))
                {
                    //if ((offset.x != 0 && renderTraversalOrder[i].x == offset.x * numChunksHalfWidth) || (offset.z != 0 && renderTraversalOrder[i].z == offset.z * numChunksHalfWidth)) {
                    innerIndexWhereNewMeshNeedsToBeCalculated[megaVertPositions.InnerIndexFlattened(renderTraversalOrder[i])] = true;
                }
            }
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

        BeginTextureMode(target);
        {
            //BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
            {
                PROFILE_SCOPE("Drawing Chunks");

                {
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

                                int curLodLevel = 0;
                                int curDistFromCamera = (int)(Vector3Length(Vector3{ renderTraversalOrder[i].x, 0, renderTraversalOrder[i].z }));
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

                            int startUp = megaVertPositions.upFacesMetadata[it].startPositionInBigArray;
                            int sizeUp = megaVertPositions.upFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startUp, sizeUp * sizeof(int), startUp * sizeof(int));

                            int startDown = megaVertPositions.downFacesMetadata[it].startPositionInBigArray;
                            int sizeDown = megaVertPositions.downFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startDown, sizeDown * sizeof(int), startDown * sizeof(int));

                            int startFront = megaVertPositions.frontFacesMetadata[it].startPositionInBigArray;
                            int sizeFront = megaVertPositions.frontFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startFront, sizeFront * sizeof(int), startFront * sizeof(int));

                            int startBack = megaVertPositions.backFacesMetadata[it].startPositionInBigArray;
                            int sizeBack = megaVertPositions.backFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startBack, sizeBack * sizeof(int), startBack * sizeof(int));

                            int startRight = megaVertPositions.rightFacesMetadata[it].startPositionInBigArray;
                            int sizeRight = megaVertPositions.rightFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startRight, sizeRight * sizeof(int), startRight * sizeof(int));

                            int startLeft = megaVertPositions.leftFacesMetadata[it].startPositionInBigArray;
                            int sizeLeft = megaVertPositions.leftFacesMetadata[it].size;
                            rlUpdateVertexBuffer(renderQuad.instanceVBOID, megaVertPositions.megaArrayOfAllPositions.data() + startLeft, sizeLeft * sizeof(int), startLeft * sizeof(int));
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

                    rlSetDepthFuncToLess();
                    rlColorMask(false, false, false, false);

                    rlEnableShader(zPrePassShader.id);

                    DrawMeshMultiInstancedDrawIndirect(renderQuad, zPrePassMaterial
                        , megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size()
                        , drawArraysIndirectCommands, drawArraysIndirectCommands.size());

                    rlSetDepthFuncToEqual();
                    rlColorMask(true, true, true, true);

                    DrawMeshMultiInstancedDrawIndirect(renderQuad, instancedMaterial
                        , megaVertPositions.megaArrayOfAllPositions.data(), megaVertPositions.megaArrayOfAllPositions.size()
                        , drawArraysIndirectCommands, drawArraysIndirectCommands.size());
                    rlUnloadShaderBuffer(chunkPosSSBO);

                    rlSetDepthFuncToLess();
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

            //EndDrawing();
            oldCameraChunkPosition = cameraChunkIndex;
            oldCameraPos = camera.position;
        }
        EndTextureMode();

        if(false)
        {
            BeginTextureMode(rd2D);
            ClearBackground(RAYWHITE);

            rlEnableShader(screenRenderMaterial.shader.id);

            BeginMode3D(camera);

            rlActiveTextureSlot(bindDepthTextureAtPosition);
            rlEnableTexture(target.depthColourTexture.id);

            rlBindShaderBuffer(chunksGridPosSSBO, 3);

            if (shouldPerformOcclusionCulling) {
                for (int i = 0; i < chunkVisibility.size(); i++)
                {
                    chunkVisibility[i] = 0;
                }

                unsigned int chunkVisibilitySSBO = rlLoadShaderBuffer(chunkVisibility.size() * sizeof(int), chunkVisibility.data(), RL_DYNAMIC_DRAW);
                rlBindShaderBuffer(chunkVisibilitySSBO, 4);

                rlEnableWireMode();
                DrawMeshMultiInstancedDrawIndirect(cullingRenderQuad, screenRenderMaterial
                    , megaArrayOfAllPositions2.data(), megaArrayOfAllPositions2.size()
                    , drawArraysIndirectCommands2, drawArraysIndirectCommands2.size()
                    , false);
                rlDisableWireMode();

                rlReadShaderBuffer(chunkVisibilitySSBO, chunkVisibility.data(), chunkVisibility.size() * sizeof(int), 0);
                rlUnloadShaderBuffer(chunkVisibilitySSBO);

                //for (int i = 0; i < renderTraversalOrder.size(); i++)
                //{
                //    int flattenedRenderTraversalIndex = megaVertPositions.ChunkFlatIndexWithoutVoxels(renderTraversalOrder[i]);
                //    if (chunkVisibility[flattenedRenderTraversalIndex] == 1) {
                //        int wireCubeSize = 32;
                //        DrawCubeWires(renderTraversalOrder[i] * chunkSize, wireCubeSize, wireCubeSize, wireCubeSize, GREEN);
                //    }
                //}
            }

            //for (int i = 0; i < chunksGridCoordinates.size(); i++)
            //{
            //    Vector3 chunkPos = Vector3{ chunksGridCoordinates[i].v[0], chunksGridCoordinates[i].v[1], chunksGridCoordinates[i].v[2] };
            //    chunkPos *= chunkSize;
            //    DrawCubeWires(chunkPos, 32, 32, 32, BLUE);
            //}

            EndMode3D();

            rlDisableShader();
            EndTextureMode();
        }

        //BeginTextureMode(rd2D);
        //    ClearBackground(RAYWHITE);
        //    BeginShaderMode(cullingShader);
        //        SetShaderValueTexture(cullingShader, GetShaderLocation(cullingShader, "depthValueTexture"), target.depthColourTexture);
        //        DrawTexture(target.depthColourTexture, 100, 100, WHITE);
        //    EndShaderMode();
        //EndTextureMode();

        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (randValue == 0) {
                DrawTextureRec(target.texture, Rectangle { 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2 { 0, 0 }, WHITE);
            }
            else if(randValue == 1) {
                DrawTextureRec(target.texture, Rectangle { 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2 { 0, 0 }, WHITE);
            }
            else if(randValue == 2) {
                DrawTextureRec(target.secondColourTexture, Rectangle { 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2 { 0, 0 }, WHITE);
            }
            else if(randValue == 3) {
                DrawTextureRec(target.depthColourTexture, Rectangle { 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2 { 0, 0 }, WHITE);
            }
            else if(randValue == 4) {
                DrawTextureRec(rd2D.texture, Rectangle { 0, 0, (float)screenWidth, (float)-screenHeight }, Vector2 { 0, 0 }, WHITE);
            }
            DrawCircle(screenWidth / 2, screenHeight / 2, 1.0f, RED);
            DrawFPS(40, 40);
        EndDrawing();
    }

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

    constexpr bool drawAll = false;

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
                    float curNoiseTop = (y + stepSizeForConvolution <= endY) ? noiseForCurrentChunk[x][y + stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y + 1][z]);

                    if (curNoiseTop == 2) {
                        int curPositionTemp = curPosition + (FACE_UP_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curPositionTemp);
                        //megaVertPositions.AddUp(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.upFacesStartIndex + chunkFacesMetadata.numUpFaces] = curPositionTemp;
                        chunkFacesMetadata.numUpFaces++;
                        //std::cout << innerChunkIndex.y << std::endl;
                    }

                    float curNoiseBottom = (y - stepSizeForConvolution >= startY - 1) ? noiseForCurrentChunk[x][y - stepSizeForConvolution][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y - 1][z]);

                    if (curNoiseBottom == 2) {
                        int curPositionTemp = curPosition + (FACE_DOWN_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curPositionTemp);
                        //megaVertPositions.AddDown(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.downFacesStartIndex + chunkFacesMetadata.numDownFaces] = curPositionTemp;
                        chunkFacesMetadata.numDownFaces++;
                    }

                    float curNoiseFront = (z + stepSizeForConvolution <= endZ) ? noiseForCurrentChunk[x][y][z + stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z + 1]);

                    if (curNoiseFront == 2) {
                        int curPositionTemp = curPosition + (FACE_FRONT_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curPositionTemp);
                        //megaVertPositions.AddFront(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.frontFacesStartIndex + chunkFacesMetadata.numFrontFaces] = curPositionTemp;
                        chunkFacesMetadata.numFrontFaces++;
                    }

                    float curNoiseBack = (z - stepSizeForConvolution >= startZ - 1) ? noiseForCurrentChunk[x][y][z - stepSizeForConvolution] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x][y][z - 1]);

                    if (curNoiseBack == 2) {
                        int curPositionTemp = curPosition + (FACE_BACK_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curPositionTemp);
                        //megaVertPositions.AddBack(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.backFacesStartIndex + chunkFacesMetadata.numBackFaces] = curPositionTemp;
                        chunkFacesMetadata.numBackFaces++;
                    }

                    float curNoiseRight = (x + stepSizeForConvolution <= endX) ? noiseForCurrentChunk[x + stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x + 1][y][z]);

                    if (curNoiseRight == 2) {
                        int curPositionTemp = curPosition + (FACE_RIGHT_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
                        //transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curPositionTemp);
                        //megaVertPositions.AddRight(curPositionTemp, innerChunkIndex);

                        chunkMeshData[chunkFacesMetadata.rightFacesStartIndex + chunkFacesMetadata.numRightFaces] = curPositionTemp;
                        chunkFacesMetadata.numRightFaces++;
                    }

                    float curNoiseLeft = (x - stepSizeForConvolution >= startX - 1) ? noiseForCurrentChunk[x - stepSizeForConvolution][y][z] : ((stepSizeForConvolution > 1) ? 2 : noiseForCurrentChunk[x - 1][y][z]);

                    if (curNoiseLeft == 2) {
                        int curPositionTemp = curPosition + (FACE_LEFT_INDEX << FACE_DIRECTION_POSITION);
                        curPositionTemp = curPositionTemp | (scale << SCALE_POSITION_IN_PACKED_INT);
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