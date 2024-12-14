#pragma once

#include <vector>

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
    {

    }

    void AddUp(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_UP_INDEX * totalNumVoxelsPerChunk + upEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        upEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    void AddDown(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_DOWN_INDEX * totalNumVoxelsPerChunk + downEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        downEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    void AddFront(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_FRONT_INDEX * totalNumVoxelsPerChunk + frontEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        frontEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    void AddBack(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_BACK_INDEX * totalNumVoxelsPerChunk + backEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        backEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    void AddRight(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_RIGHT_INDEX * totalNumVoxelsPerChunk + rightEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        rightEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    void AddLeft(int toAdd, int chunkFlatIndex, Vector3 chunkIndex) {
        megaArrayOfAllPositions[chunkFlatIndex + FACE_LEFT_INDEX * totalNumVoxelsPerChunk + leftEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]] = toAdd;
        leftEndVoxelPositions[ChunkDirFaceFlatIndex(chunkIndex)]++;
        totalFilled++;
    }

    int ChunkFlatIndex(Vector3 chunkIndex) {
        return chunkIndex.x * numChunks * numChunksY * NUM_FACES * chunkSize * chunkSize * chunkSize
            + chunkIndex.z * numChunksY * NUM_FACES * chunkSize * chunkSize * chunkSize
            + chunkIndex.y * NUM_FACES * chunkSize * chunkSize * chunkSize;
    }

    int ChunkDirFaceFlatIndex(Vector3 chunkIndex) {
        return chunkIndex.x * numChunks * numChunksY + chunkIndex.z * numChunksY + chunkIndex.y;
    }

};
