#pragma once

#include <vector>
#include <fstream>
#include <span>

#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"

struct ChunkFacePositionMetaData {

public:
    int startPositionInBigArray;
    int size;

    int EndPos() {
        return startPositionInBigArray + size;
    }
};


struct VertexPositions {

public:

    std::vector<int> megaArrayOfAllPositions;

    std::vector<ChunkFacePositionMetaData> upEndVoxelPositions;
    std::vector<ChunkFacePositionMetaData> downEndVoxelPositions;
    std::vector<ChunkFacePositionMetaData> frontEndVoxelPositions;
    std::vector<ChunkFacePositionMetaData> backEndVoxelPositions;
    std::vector<ChunkFacePositionMetaData> rightEndVoxelPositions;
    std::vector<ChunkFacePositionMetaData> leftEndVoxelPositions;

    int totalFilled = 0;

    VertexPositions() : megaArrayOfAllPositions(totalNumFaces, 0)
        , upEndVoxelPositions(totalNumChunks, { 0, 0 })
        , downEndVoxelPositions(totalNumChunks, { 0, 0 })
        , frontEndVoxelPositions(totalNumChunks, { 0, 0 })
        , backEndVoxelPositions(totalNumChunks, { 0, 0 })
        , rightEndVoxelPositions(totalNumChunks, { 0, 0 })
        , leftEndVoxelPositions(totalNumChunks, { 0, 0 })
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

        //VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV This should not be here, it will mess up the free list.
        totalFilled -= upEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= downEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= frontEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= backEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= rightEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= leftEndVoxelPositions[chunkFlatIndexWithoutVoxels].size;

        upEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;
        downEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;
        frontEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;
        backEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;
        rightEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;
        leftEndVoxelPositions[chunkFlatIndexWithoutVoxels].size = 0;

        //int start = ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
        //int end = start + totalNumVoxelsPerChunk * NUM_FACES;
        //for (int i = start; i < end; i++)
        //{
        //    megaArrayOfAllPositions[i] = 0;
        //}
    }

    int GetCurFaceDirChunkDataEndPos(Vector3 chunkIndex, int curDir) {
        switch (curDir) {
        case FACE_UP_INDEX:
            return upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_DOWN_INDEX:
            return downEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_FRONT_INDEX:
            return frontEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_BACK_INDEX:
            return backEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_RIGHT_INDEX:
            return rightEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_LEFT_INDEX:
            return leftEndVoxelPositions[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        }
    }

    std::span<int> GetCurChunkCurDirVoxelData(Vector3 curChunkIndex, int curDir) {

        int curChunkFlatIndexWithVoxels = ChunkTotalFlatIndexWithVoxels(curChunkIndex);
        int curChunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(curChunkIndex);

        auto curIteratorHead = megaArrayOfAllPositions.begin() + curChunkFlatIndexWithVoxels + (curDir * totalNumVoxelsPerChunkWorstCase);
        std::span<int> curSpan;

        switch (curDir) {
        case FACE_UP_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + upEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_DOWN_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + downEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_FRONT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + frontEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_BACK_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + backEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_RIGHT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + rightEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_LEFT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + leftEndVoxelPositions[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        }
    }

    void AddUp(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_UP_INDEX * totalNumVoxelsPerChunkWorstCase + upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;

        //std::cout << upEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)] << std::endl;
        totalFilled++;
    }

    void AddDown(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_DOWN_INDEX * totalNumVoxelsPerChunkWorstCase + downEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        downEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
        totalFilled++;
    }

    void AddFront(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_FRONT_INDEX * totalNumVoxelsPerChunkWorstCase + frontEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        frontEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
        totalFilled++;
    }

    void AddBack(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_BACK_INDEX * totalNumVoxelsPerChunkWorstCase + backEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        backEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
        totalFilled++;
    }

    void AddRight(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_RIGHT_INDEX * totalNumVoxelsPerChunkWorstCase + rightEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        rightEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
        totalFilled++;
    }

    void AddLeft(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_LEFT_INDEX * totalNumVoxelsPerChunkWorstCase + leftEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        leftEndVoxelPositions[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
        totalFilled++;
    }

    int ChunkTotalFlatIndexWithVoxels(Vector3 innerChunkIndex) {

        innerChunkIndex = innerChunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };

        return innerChunkIndex.y * numChunksFullWidth * numChunksFullWidth * NUM_FACES * totalNumVoxelsPerChunkWorstCase
            + innerChunkIndex.z * numChunksFullWidth * NUM_FACES * totalNumVoxelsPerChunkWorstCase
            + innerChunkIndex.x * NUM_FACES * totalNumVoxelsPerChunkWorstCase;
    }

    int ChunkFlatIndexWithoutVoxels(Vector3 innerChunkIndex) {

        innerChunkIndex = innerChunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };
        return innerChunkIndex.y * numChunksFullWidth * numChunksFullWidth + innerChunkIndex.z * numChunksFullWidth + innerChunkIndex.x;
    }

    int ImaginaryChunkFlatIndexWithoutVoxels(Vector3 chunkIndex) {

        //chunkIndex = chunkIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };
        return chunkIndex.y * numChunksFullWidth * numChunksFullWidth + chunkIndex.z * numChunksFullWidth + chunkIndex.x;
    }

    int InnerIndexFlattened(Vector3 innerIndex) {

        innerIndex = innerIndex + Vector3{ (float)numChunksHalfWidth, (float)0, (float)numChunksHalfWidth };
        return innerIndex.y * numChunksFullWidth * numChunksFullWidth + innerIndex.z * numChunksFullWidth + innerIndex.x;

    }
};
