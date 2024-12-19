#pragma once

#include "raylib/raymath.h"

#define FACE_DIRECTION_POSITION 16

#define FACE_UP_INDEX 0
#define FACE_DOWN_INDEX 1
#define FACE_FRONT_INDEX 2
#define FACE_BACK_INDEX 3
#define FACE_RIGHT_INDEX 4
#define FACE_LEFT_INDEX 5

#define NUM_FACES 6

const int numChunksHalfWidth = 4;
const int numChunksHalfWidth_Y = 2;
const int chunkSize = 8;
const float scale = 0.1f;

constexpr int numChunksFullWidth = (2 * numChunksHalfWidth) + 1; //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< THIS DOESN'T MATCH WITH THE RENDER TRAVERSAL ORDER COORDINATES!!!!
constexpr int numChunksFullWidth_Y = numChunksHalfWidth_Y;
constexpr int totalNumVoxelsPerChunk = chunkSize * chunkSize * chunkSize;

constexpr int totalNumChunks =  numChunksFullWidth * numChunksFullWidth * numChunksFullWidth_Y;
constexpr int totalNumFaces = totalNumChunks * NUM_FACES * totalNumVoxelsPerChunk;

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };