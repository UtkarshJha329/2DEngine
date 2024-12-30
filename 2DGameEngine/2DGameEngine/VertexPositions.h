#pragma once

#include <vector>
#include <unordered_map>
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

struct FreeListMetaData {

public:
    int freeBlockStartPosition;
    int freeBlockSize;
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

    std::vector<FreeListMetaData> freeListMetadataList;

    int freeListHead = 0;
    std::unordered_map<int, int> freeListSizeOfBlockAtIndex;

    int nextToFill = 0;

    VertexPositions() : megaArrayOfAllPositions(totalNumFacesToStore, 0)
        , upFacesMetadata(totalNumChunks, { 0, 0 })
        , downFacesMetadata(totalNumChunks, { 0, 0 })
        , frontFacesMetadata(totalNumChunks, { 0, 0 })
        , backFacesMetadata(totalNumChunks, { 0, 0 })
        , rightFacesMetadata(totalNumChunks, { 0, 0 })
        , leftFacesMetadata(totalNumChunks, { 0, 0 })
        , nextToFill(0)
        , freeListMetadataList(1, { 0, totalNumFacesToStore })
    {
        megaArrayOfAllPositions[freeListHead] = -1;
        freeListSizeOfBlockAtIndex[freeListHead] = totalNumFacesToStore;
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
            , nextToFill);
    }

    const int numToCheck = 16;

    void ClearChunkData(Vector3 innerChunkIndex) {

        int chunkFlatIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex);

        //VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV This should not be here, it will mess up the free list.
        //totalFilled -= upFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        //totalFilled -= downFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        //totalFilled -= frontFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        //totalFilled -= backFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        //totalFilled -= rightFacesMetadata[chunkFlatIndexWithoutVoxels].size;
        //totalFilled -= leftFacesMetadata[chunkFlatIndexWithoutVoxels].size;

        upFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        downFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        frontFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        backFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        rightFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;
        leftFacesMetadata[chunkFlatIndexWithoutVoxels].size = 0;

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

    void CopyDataToMegaArray(std::vector<int> & copyIntoArray
                            , std::vector<int> & copyFromArray, int offsetIntoCopyArray, int numCopyFromCopyArray
                            , int* mappedPositionThatNeedsToBeRemaped) {
        auto copyBeginFrom = copyFromArray.begin() + offsetIntoCopyArray;
        auto copyEndAt = copyBeginFrom + numCopyFromCopyArray;

        int copyIntoArrayCopyAtPosition = AllocateMemoryOfSize(numCopyFromCopyArray);

        std::copy(copyBeginFrom, copyEndAt, copyIntoArray.begin() + copyIntoArrayCopyAtPosition);
        //std::cout << totalFilled << std::endl;
        *mappedPositionThatNeedsToBeRemaped = copyIntoArrayCopyAtPosition;
        //nextToFill += numCopyFromCopyArray;
    }

    int AllocateMemoryOfSize(int sizeOfMemoryToAllocate) {
        //for (int i = 0; i < freeListMetadataList.size(); i++)
        //{
        //    if (freeListMetadataList[i].freeBlockSize >= sizeOfMemoryToAllocate) {

        //        int startPositionOfBlock = freeListMetadataList[i].freeBlockStartPosition;

        //        if (freeListMetadataList[i].freeBlockSize > sizeOfMemoryToAllocate) {
        //            freeListMetadataList[i].freeBlockStartPosition += sizeOfMemoryToAllocate;
        //            freeListMetadataList[i].freeBlockSize -= sizeOfMemoryToAllocate;
        //        }

        //        return startPositionOfBlock;
        //    }
        //}

        if (sizeOfMemoryToAllocate == 0) {
            return freeListHead;
        }

        int curFreeIndex = freeListHead;
        int lastFreeIndex = -1;
        int nextFreeIndex = megaArrayOfAllPositions[curFreeIndex];

        while (curFreeIndex != -1) {

            int sizeAtCurIndex = freeListSizeOfBlockAtIndex[curFreeIndex];
            if (sizeAtCurIndex >= sizeOfMemoryToAllocate) {

                int startPositionOfBlock = curFreeIndex;

                if (sizeAtCurIndex > sizeOfMemoryToAllocate) {
                    //rearrange block's index in free list.
                    int rearrangedPosition = startPositionOfBlock + sizeOfMemoryToAllocate;
                    if (lastFreeIndex != -1) {
                        megaArrayOfAllPositions[lastFreeIndex] = rearrangedPosition;
                    }
                    else {
                        freeListHead = rearrangedPosition;
                    }
                    megaArrayOfAllPositions[rearrangedPosition] = nextFreeIndex;
                    freeListSizeOfBlockAtIndex[rearrangedPosition] = sizeAtCurIndex - sizeOfMemoryToAllocate;
                    //std::cout << rearrangedPosition << std::endl;
                }
                else {
                    //skip this block in free list.
                    //std::cout << "Came here for whatever reason." << std::endl;
                    if (lastFreeIndex != -1) {
                        megaArrayOfAllPositions[lastFreeIndex] = nextFreeIndex;
                    }
                    else {
                        freeListHead = nextFreeIndex;
                    }
                }

                freeListSizeOfBlockAtIndex.erase(curFreeIndex);

                return startPositionOfBlock;
            }

            lastFreeIndex = curFreeIndex;
            curFreeIndex = megaArrayOfAllPositions[curFreeIndex];
            //std::cout << "Performed." << curFreeIndex << std::endl;
            nextFreeIndex = curFreeIndex == -1 ? -1 : megaArrayOfAllPositions[curFreeIndex];
        }

        return -1;
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
