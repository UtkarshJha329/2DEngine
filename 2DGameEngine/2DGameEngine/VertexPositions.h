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

    inline static int totalSize = totalNumFaces * sizeof(int) + NUM_FACES * totalNumChunks * sizeof(int) + sizeof(int);

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

    void ClearChunkData(Vector3 chunkIndex) {

        totalFilled -= upEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];
        totalFilled -= downEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];
        totalFilled -= frontEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];
        totalFilled -= backEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];
        totalFilled -= rightEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];
        totalFilled -= leftEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)];

        upEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;
        downEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;
        frontEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;
        backEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;
        rightEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;
        leftEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)] = 0;

        int start = RescopedChunkTotalFlatIndexWithVoxels(RescopeChunkIndex(chunkIndex));
        int end = start + totalNumVoxelsPerChunk * NUM_FACES;
        for (int i = start; i < end; i++)
        {
            megaArrayOfAllPositions[i] = 0;
        }
    }

    void AddUp(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_UP_INDEX * totalNumVoxelsPerChunk + upEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        upEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    void AddDown(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_DOWN_INDEX * totalNumVoxelsPerChunk + downEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        downEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    void AddFront(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_FRONT_INDEX * totalNumVoxelsPerChunk + frontEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        frontEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    void AddBack(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_BACK_INDEX * totalNumVoxelsPerChunk + backEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        backEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    void AddRight(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_RIGHT_INDEX * totalNumVoxelsPerChunk + rightEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        rightEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    void AddLeft(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_LEFT_INDEX * totalNumVoxelsPerChunk + leftEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]] = toAdd;
        leftEndVoxelPositions[RescopedChunkFlatIndexWithoutVoxels(chunkIndex)]++;
        totalFilled++;
    }

    int RescopedChunkTotalFlatIndexWithVoxels(Vector3 chunkIndex) {

        chunkIndex = RescopeChunkIndex(chunkIndex);

        return chunkIndex.x * numChunks * numChunksY * NUM_FACES * chunkSize * chunkSize * chunkSize
            + chunkIndex.z * numChunksY * NUM_FACES * chunkSize * chunkSize * chunkSize
            + chunkIndex.y * NUM_FACES * chunkSize * chunkSize * chunkSize;
    }

    int RescopedChunkFlatIndexWithoutVoxels(Vector3 chunkIndex) {

        chunkIndex = RescopeChunkIndex(chunkIndex);
        return chunkIndex.x * numChunks * numChunksY + chunkIndex.z * numChunksY + chunkIndex.y;
    }

    int NonRescopedChunkFlatIndexWithoutVoxels(Vector3 chunkIndex) {

        return chunkIndex.x * numChunks * numChunksY + chunkIndex.z * numChunksY + chunkIndex.y;
    }

    Vector3 RescopeChunkIndex(Vector3 chunkIndex) {

        if (chunkIndex.x >= numChunks) {
            chunkIndex.x = (int)chunkIndex.x % numChunks;
        }

        if (chunkIndex.y >= numChunksY) {
            chunkIndex.y = (int)chunkIndex.y % numChunksY;
        }

        if (chunkIndex.z >= numChunks) {
            chunkIndex.z = (int)chunkIndex.z % numChunks;
        }
        return chunkIndex;
    }

};
