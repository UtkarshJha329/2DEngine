#pragma once

#include "raylib/raymath.h"
#include "raylib/rlgl.h"

#define FACE_DIRECTION_POSITION 16
//#define SCALE_POSITION_IN_PACKED_INT_X 19
//#define SCALE_POSITION_IN_PACKED_INT_Y 24
//#define SCALE_POSITION_IN_PACKED_INT_Z 29

#define SCALE_POSITION_IN_PACKED_INT_A 19
#define SCALE_POSITION_IN_PACKED_INT_B 24

#define FACE_UP_INDEX 0
#define FACE_DOWN_INDEX 1
#define FACE_FRONT_INDEX 2
#define FACE_BACK_INDEX 3
#define FACE_RIGHT_INDEX 4
#define FACE_LEFT_INDEX 5

#define NUM_FACES 6

//#define CHUNK_SAVE_STRING(chunkIndex, faceDir) "Chunk" + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.x) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.y) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.z) + CHUNK_FILE_DELIMITER + "LOD.LEVEL" + CHUNK_FILE_DELIMITER + std::to_string(LODLevel) + CHUNK_FILE_DELIMITER + "FACEDIR" + CHUNK_FILE_DELIMITER + std::to_string(faceDir) + CHUNK_FILE_DELIMITER + "Data"
#define CHUNK_SAVE_STRING(chunkIndex) "Chunk" + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.x) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.y) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.z) + CHUNK_FILE_DELIMITER + "LOD.LEVEL" + CHUNK_FILE_DELIMITER + std::to_string(LODLevel) + CHUNK_FILE_DELIMITER + "Data"
#define CHUNK_METADATA_SAVE_STRING(chunkIndex) "Chunk" + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.x) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.y) + CHUNK_FILE_DELIMITER + std::to_string(chunkIndex.z) + CHUNK_FILE_DELIMITER + "LOD.LEVEL" + CHUNK_FILE_DELIMITER + std::to_string(LODLevel) + CHUNK_FILE_DELIMITER + "MetaData"

const std::string CHUNK_FILE_DELIMITER = ".";

const std::string worldDataDir = "WorldData/";

const int numChunksHalfWidth = 3;
const int numChunksHalfWidth_Y = 3;
const int chunkSize = 32;
const float scale = 0.01f;

constexpr int numChunksFullWidth = (2 * numChunksHalfWidth) + 1;
constexpr int numChunksFullWidth_Y = numChunksHalfWidth_Y;
constexpr int totalNumVoxelsPerChunk = chunkSize * chunkSize * chunkSize;
constexpr int totalNumVoxelsPerChunkWorstCase = totalNumVoxelsPerChunk / (2 * 2 * 2);

constexpr int totalNumChunks =  numChunksFullWidth * numChunksFullWidth * numChunksFullWidth_Y;
constexpr int totalNumFaces = totalNumChunks * NUM_FACES * totalNumVoxelsPerChunkWorstCase;

const int farPlaneDistance = RL_CULL_DISTANCE_FAR;

const int lodLevelOffset = 32;

constexpr Vector2 lodDistance1 = { 0, lodLevelOffset - 1 };
constexpr Vector2 lodDistance2 = { lodDistance1.y + 1, lodDistance1.y + lodLevelOffset };
constexpr Vector2 lodDistance3 = { lodDistance2.y + 1, lodDistance2.y + lodLevelOffset };
constexpr Vector2 lodDistance4 = { lodDistance3.y + 1, lodDistance3.y + lodLevelOffset };
constexpr Vector2 lodDistance5 = { lodDistance4.y + 1, lodDistance4.y + lodLevelOffset };

constexpr int totalNumFacesToStore = 100000000;
const int maxLODLevel = 5;

float LODLevel = 0.0f;
const bool saveChunkToFile = false;

bool shouldPerformOcclusionCulling = false;
float occlusionCullingStateValue = 0;

int maxRandValue = 4;

constexpr float diagonalDist = 3 * chunkSize * 1.732f;

const int localSizeOfComputeXZ = 32;

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };
