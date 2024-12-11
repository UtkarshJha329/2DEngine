#pragma once

#include <vector>

#include "BlockType.h"

enum class BlockFaceDirection {
    UP,
    DOWN,
    FRONT,
    BACK,
    RIGHT,
    LEFT
};

void TexCoords(float* texCoords) {

    //texCoords.push_back(0.0f);
    //texCoords.push_back(0.0f);

    //texCoords.push_back(1.0f);
    //texCoords.push_back(1.0f);

    //texCoords.push_back(1.0f);
    //texCoords.push_back(0.0f);

    //texCoords.push_back(0.0f);
    //texCoords.push_back(0.0f);

    //texCoords.push_back(0.0f);
    //texCoords.push_back(1.0f);

    //texCoords.push_back(1.0f);
    //texCoords.push_back(1.0f);
//************************************

    //const int numTexturesX = 6;
    //const int numTexturesY = 6;

    //const float xBottomLeft = 4.0f / (float)numTexturesX;
    //const float yBottomLeft = 0.0f / (float)numTexturesY;

    //texCoords.push_back(xBottomLeft);
    //texCoords.push_back(yBottomLeft);

    //texCoords.push_back(xBottomLeft + (1.0f / (float)numTexturesX));
    //texCoords.push_back(yBottomLeft);

    //texCoords.push_back(xBottomLeft + (1.0f / (float)numTexturesX));
    //texCoords.push_back(yBottomLeft + (1.0f / (float)numTexturesY));

    //texCoords.push_back(xBottomLeft);
    //texCoords.push_back(yBottomLeft);

    //texCoords.push_back(xBottomLeft);
    //texCoords.push_back(yBottomLeft - (1.0f / (float)numTexturesY));

    //texCoords.push_back(xBottomLeft + (1.0f / (float)numTexturesX));
    //texCoords.push_back(yBottomLeft - (1.0f / (float)numTexturesY));

//************************************
    const int numTexturesX = 6;
    const int numTexturesY = 6;

    const float xTopLeft = 4.0f / (float)numTexturesX;
    const float yTopLeft = 0.0f / (float)numTexturesY;

    texCoords[0] = (xTopLeft);
    texCoords[1] = (yTopLeft + (1.0f / (float)numTexturesY));

    texCoords[2] = (xTopLeft);
    texCoords[3] = (yTopLeft);

    texCoords[4] = (xTopLeft + (1.0f / (float)numTexturesX));
    texCoords[5] = (yTopLeft + (1.0f / (float)numTexturesY));

    //texCoords[6] = (xTopLeft);
    //texCoords[7] = (yTopLeft);

    texCoords[6] = (xTopLeft + (1.0f / (float)numTexturesY));
    texCoords[7] = (yTopLeft);

    //texCoords[10] = (xTopLeft + (1.0f / (float)numTexturesX));
    //texCoords[11] = (yTopLeft + (1.0f / (float)numTexturesY));
//************************************
}

void FaceVerticesTop(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (-0.5 + offsetX);
    vertices[4] =  (0.5 + offsetY);
    vertices[5] =  (0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] = (0.5 + offsetX);
    vertices[10] =(0.5 + offsetY);
    vertices[11] =(0.5 + offsetZ);
}

void FaceVerticesBottom(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (-0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (-0.5 + offsetX);
    vertices[7] = (-0.5 + offsetY);
    vertices[8] = (0.5 + offsetZ);

    //vertices[9] =  (-0.5 + offsetX);
    //vertices[10] = (-0.5 + offsetY);
    //vertices[11] = (-0.5 + offsetZ);

    vertices[9] = (0.5 + offsetX);
    vertices[10] = (-0.5 + offsetY);
    vertices[11] = (-0.5 + offsetZ);

    //vertices[15] = (0.5 + offsetX);
    //vertices[16] = (-0.5 + offsetY);
    //vertices[17] = (0.5 + offsetZ);

}

void FaceVerticesFront(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (0.5 + offsetX);
    vertices[1] = (-0.5 + offsetY);
    vertices[2] = (0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (-0.5 + offsetX);
    vertices[7] =  (-0.5 + offsetY);
    vertices[8] =  (0.5 + offsetZ);

    vertices[9] = (-0.5 + offsetX);
    vertices[10] =(0.5 + offsetY);
    vertices[11] =(0.5 + offsetZ);
}


void FaceVerticesBack(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (-0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (-0.5 + offsetX);
    vertices[4] = (0.5 + offsetY);
    vertices[5] = (-0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] =  (-0.5 + offsetY);
    vertices[8] =  (-0.5 + offsetZ);

    vertices[9] = (0.5 + offsetX);
    vertices[10] =(0.5 + offsetY);
    vertices[11] =(-0.5 + offsetZ);
}

void FaceVerticesRight(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] = (-0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] = (0.5 + offsetX);
    vertices[10] =(-0.5 + offsetY);
    vertices[11] = (0.5 + offsetZ);
}

void FaceVerticesLeft(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (-0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (-0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (-0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] = (-0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (0.5 + offsetZ);

}