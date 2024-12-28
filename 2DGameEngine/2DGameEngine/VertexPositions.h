#pragma once

#include <vector>
#include <fstream>
#include <span>

#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"

struct ChunkFacesMetadata {

    int upFacesStartIndex = 0;
    int downFacesStartIndex = 0;
    int frontFacesStartIndex = 0;
    int backFacesStartIndex = 0;
    int rightFacesStartIndex = 0;
    int leftFacesStartIndex = 0;

    int numUpFaces = 0;
    int numDownFaces = 0;
    int numFrontFaces = 0;
    int numBackFaces = 0;
    int numRightFaces = 0;
    int numLeftFaces = 0;

    int GetSizeOfFaceDirPositions(int faceDir) {
        switch (faceDir) {
        case FACE_UP_INDEX:
            return numUpFaces;
        case FACE_DOWN_INDEX:
            return numDownFaces;
        case FACE_FRONT_INDEX:
            return numFrontFaces;
        case FACE_BACK_INDEX:
            return numBackFaces;
        case FACE_RIGHT_INDEX:
            return numRightFaces;
        case FACE_LEFT_INDEX:
            return numLeftFaces;
        }
    }

    int* GetAppropriateStartIndexBasedOnFaceDir(int faceDir) {
        switch (faceDir) {
        case FACE_UP_INDEX:
            return &upFacesStartIndex;
        case FACE_DOWN_INDEX:
            return &downFacesStartIndex;
        case FACE_FRONT_INDEX:
            return &frontFacesStartIndex;
        case FACE_BACK_INDEX:
            return &backFacesStartIndex;
        case FACE_RIGHT_INDEX:
            return &rightFacesStartIndex;
        case FACE_LEFT_INDEX:
            return &leftFacesStartIndex;
        }
    }
};

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

    std::vector<ChunkFacePositionMetaData> upFacesMetadata;
    std::vector<ChunkFacePositionMetaData> downFacesMetadata;
    std::vector<ChunkFacePositionMetaData> frontFacesMetadata;
    std::vector<ChunkFacePositionMetaData> backFacesMetadata;
    std::vector<ChunkFacePositionMetaData> rightFacesMetadata;
    std::vector<ChunkFacePositionMetaData> leftFacesMetadata;

    int totalFilled = 0;

    VertexPositions() : megaArrayOfAllPositions(/*totalNumFaces*/1250000 * 2, 0)
        , upFacesMetadata(totalNumChunks, { 0, 0 })
        , downFacesMetadata(totalNumChunks, { 0, 0 })
        , frontFacesMetadata(totalNumChunks, { 0, 0 })
        , backFacesMetadata(totalNumChunks, { 0, 0 })
        , rightFacesMetadata(totalNumChunks, { 0, 0 })
        , leftFacesMetadata(totalNumChunks, { 0, 0 })
        , totalFilled(0)
    {

    }

    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(megaArrayOfAllPositions
            , upFacesMetadata
            , downFacesMetadata
            , frontFacesMetadata
            , backFacesMetadata
            , rightFacesMetadata
            , leftFacesMetadata
            , totalFilled);
    }

    const int numToCheck = 16;

    void ClearChunkData(Vector3 innerChunkIndex) {

        int chunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        //VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV This should not be here, it will mess up the free list.
        totalFilled -= upFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= downFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= frontFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= backFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= rightFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        totalFilled -= leftFacesMetadata[chunkFlatIndexWithoutVoxels].size;

        upFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        downFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        frontFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        backFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        rightFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        leftFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;

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
            return upFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_DOWN_INDEX:
            return downFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_FRONT_INDEX:
            return frontFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_BACK_INDEX:
            return backFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_RIGHT_INDEX:
            return rightFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        case FACE_LEFT_INDEX:
            return leftFacesMetadata[ChunkFlatIndexWithoutVoxels(chunkIndex)].EndPos();
        }
    }

    std::span<int> GetCurChunkCurDirVoxelData(Vector3 innerChunkIndex, int curDir) {

        int curChunkFlatIndexWithVoxels = ChunkTotalFlatIndexWithVoxels(innerChunkIndex);
        int curChunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        auto curIteratorHead = megaArrayOfAllPositions.begin() + curChunkFlatIndexWithVoxels + (curDir * totalNumVoxelsPerChunkWorstCase);
        std::span<int> curSpan;

        switch (curDir) {
        case FACE_UP_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + upFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_DOWN_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + downFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_FRONT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + frontFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_BACK_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + backFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_RIGHT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + rightFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        case FACE_LEFT_INDEX:
            curSpan = std::span<int>(curIteratorHead, curIteratorHead + leftFacesMetadata[curChunkFlatIndexWithoutVoxels].size);
            return curSpan;
        }
    }

    void MapChunkMemoryToBigArray(Vector3 innerChunkIndex, ChunkFacesMetadata chunkFacesMetadata) {

        int chunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        //int startPos = ChunkTotalFlatIndexWithVoxels(innerChunkIndex);

        upFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.upFacesStartIndex;
        upFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numUpFaces;
        //startPos += chunkFacesMetadata.numUpFaces;

        downFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.downFacesStartIndex;
        downFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numDownFaces;
        //startPos += chunkFacesMetadata.numDownFaces;

        frontFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.frontFacesStartIndex;
        frontFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numFrontFaces;
        //startPos += chunkFacesMetadata.numFrontFaces;
        
        backFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.backFacesStartIndex;
        backFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numBackFaces;
        //startPos += chunkFacesMetadata.numBackFaces;
        
        rightFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.rightFacesStartIndex;
        rightFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numRightFaces;
        //startPos += chunkFacesMetadata.numRightFaces;
        
        leftFacesMetadata[chunkFlatIndexWithoutVoxels].startPositionInBigArray = chunkFacesMetadata.leftFacesStartIndex;
        leftFacesMetadata[chunkFlatIndexWithoutVoxels].size = chunkFacesMetadata.numLeftFaces;
        //startPos += chunkFacesMetadata.numLeftFaces;
    }

    void CopyDataToMegaArray(std::vector<int> & copyIntoArray, int copyIntoArrayOffsetToCopyAt
                            , std::vector<int> & copyFromArray, int offsetIntoCopyArray, int numCopyFromCopyArray
                            , int* mappedPositionThatNeedsToBeRemaped) {
        auto copyBeginFrom = copyFromArray.begin() + offsetIntoCopyArray;
        auto copyEndAt = copyBeginFrom + numCopyFromCopyArray;
        std::copy(copyBeginFrom, copyEndAt, copyIntoArray.begin() + copyIntoArrayOffsetToCopyAt);
        //std::cout << totalFilled << std::endl;
        *mappedPositionThatNeedsToBeRemaped = copyIntoArrayOffsetToCopyAt;
        totalFilled += numCopyFromCopyArray;
    }

    void AddUp(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_UP_INDEX * totalNumVoxelsPerChunkWorstCase + upFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        upFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
    }

    void AddDown(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_DOWN_INDEX * totalNumVoxelsPerChunkWorstCase + downFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        downFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
    }

    void AddFront(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_FRONT_INDEX * totalNumVoxelsPerChunkWorstCase + frontFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        frontFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
    }

    void AddBack(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_BACK_INDEX * totalNumVoxelsPerChunkWorstCase + backFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        backFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
    }

    void AddRight(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_RIGHT_INDEX * totalNumVoxelsPerChunkWorstCase + rightFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        rightFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
    }

    void AddLeft(int toAdd, Vector3 innerChunkIndex) {
        megaArrayOfAllPositions[ChunkTotalFlatIndexWithVoxels(innerChunkIndex) + FACE_LEFT_INDEX * totalNumVoxelsPerChunkWorstCase + leftFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size] = toAdd;
        leftFacesMetadata[ChunkFlatIndexWithoutVoxels(innerChunkIndex)].size++;
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
