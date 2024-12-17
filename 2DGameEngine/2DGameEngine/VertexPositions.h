#pragma once

#include <vector>
#include <fstream>

#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"


struct VertexPositions {

public:

    std::vector<int> megaArrayOfAllPositions;

    std::vector<int> upEndVoxelPositions;
    std::vector<int> downEndVoxelPositions;
    std::vector<int> frontEndVoxelPositions;
    std::vector<int> backEndVoxelPositions;
    std::vector<int> rightEndVoxelPositions;
    std::vector<int> leftEndVoxelPositions;

    int totalFilled = 0;

    VertexPositions() : megaArrayOfAllPositions(totalNumFaces, 0)
        , upEndVoxelPositions(totalNumChunks, 0)
        , downEndVoxelPositions(totalNumChunks, 0)
        , frontEndVoxelPositions(totalNumChunks, 0)
        , backEndVoxelPositions(totalNumChunks, 0)
        , rightEndVoxelPositions(totalNumChunks, 0)
        , leftEndVoxelPositions(totalNumChunks, 0)
        , totalFilled(0)
    {

    }

    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(megaArrayOfAllPositions
            , upEndVoxelPositions
            , downEndVoxelPositions
            , frontEndVoxelPositions
            , backEndVoxelPositions
            , rightEndVoxelPositions
            , leftEndVoxelPositions
            , totalFilled);
    }

    const int numToCheck = 16;

    void ClearChunkData(Vector3 innerChunkIndex) {

        int chunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        totalFilled -= upEndVoxelPositions[chunkFlatIndexWithoutVoxels];
        totalFilled -= downEndVoxelPositions[chunkFlatIndexWithoutVoxels];
        totalFilled -= frontEndVoxelPositions[chunkFlatIndexWithoutVoxels];
        totalFilled -= backEndVoxelPositions[chunkFlatIndexWithoutVoxels];
        totalFilled -= rightEndVoxelPositions[chunkFlatIndexWithoutVoxels];
        totalFilled -= leftEndVoxelPositions[chunkFlatIndexWithoutVoxels];

        upEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;
        downEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;
        frontEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;
        backEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;
        rightEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;
        leftEndVoxelPositions[chunkFlatIndexWithoutVoxels] = 0;

        int start = ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
        int end = start + totalNumVoxelsPerChunk * NUM_FACES;
        for (int i = start; i < end; i++)
        {
            megaArrayOfAllPositions[i] = 0;
        }
    }

    void AddUp(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_UP_INDEX * totalNumVoxelsPerChunk + upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;

        //std::cout << upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)] << std::endl;
        totalFilled++;
    }

    void AddDown(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_DOWN_INDEX * totalNumVoxelsPerChunk + downEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        downEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;
        totalFilled++;
    }

    void AddFront(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_FRONT_INDEX * totalNumVoxelsPerChunk + frontEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        frontEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;
        totalFilled++;
    }

    void AddBack(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_BACK_INDEX * totalNumVoxelsPerChunk + backEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        backEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;
        totalFilled++;
    }

    void AddRight(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_RIGHT_INDEX * totalNumVoxelsPerChunk + rightEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        rightEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;
        totalFilled++;
    }

    void AddLeft(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_LEFT_INDEX * totalNumVoxelsPerChunk + leftEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]] = toAdd;
        leftEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)]++;
        totalFilled++;
    }

    int ChunkTotalFlatIndexWithVoxels(Vector3 innerChunkIndex) {

        innerChunkIndex = innerChunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };

        return innerChunkIndex.x * numChunksFullWidth * numChunksYFullWidth * NUM_FACES * chunkSize * chunkSize * chunkSize
            + innerChunkIndex.z * numChunksYFullWidth * NUM_FACES * chunkSize * chunkSize * chunkSize
            + innerChunkIndex.y * NUM_FACES * chunkSize * chunkSize * chunkSize;
    }

    int ChunkFlatIndexWithoutVoxels(Vector3 innerChunkIndex) {

        innerChunkIndex = innerChunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };
        return innerChunkIndex.x * numChunksFullWidth * numChunksYFullWidth + innerChunkIndex.z * numChunksYFullWidth + innerChunkIndex.y;
    }

    int ImaginaryChunkFlatIndexWithoutVoxels(Vector3 chunkIndex) {

        chunkIndex = chunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };
        return chunkIndex.x * numChunksFullWidth * numChunksYFullWidth + chunkIndex.z * numChunksYFullWidth + chunkIndex.y;
    }
};
