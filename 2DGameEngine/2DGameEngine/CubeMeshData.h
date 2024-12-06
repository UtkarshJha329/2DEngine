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
    texCoords[1] = (yTopLeft);

    texCoords[2] = (xTopLeft + (1.0f / (float)numTexturesX));
    texCoords[3] = (yTopLeft + (1.0f / (float)numTexturesY));

    texCoords[4] = (xTopLeft + (1.0f / (float)numTexturesX));
    texCoords[5] = (yTopLeft);

    texCoords[6] = (xTopLeft);
    texCoords[7] = (yTopLeft);

    texCoords[8] = (xTopLeft);
    texCoords[9] = (yTopLeft + (1.0f / (float)numTexturesY));

    texCoords[10] = (xTopLeft + (1.0f / (float)numTexturesX));
    texCoords[11] = (yTopLeft + (1.0f / (float)numTexturesY));
//************************************
}

void VertColour(std::vector<unsigned char>& vertColour) {

    vertColour.push_back(255);
    vertColour.push_back(0);
    vertColour.push_back(0);
    vertColour.push_back(255);

    vertColour.push_back(0);
    vertColour.push_back(255);
    vertColour.push_back(0);
    vertColour.push_back(255);

    vertColour.push_back(0);
    vertColour.push_back(0);
    vertColour.push_back(255);
    vertColour.push_back(255);

    vertColour.push_back(255);
    vertColour.push_back(0);
    vertColour.push_back(0);
    vertColour.push_back(255);

    vertColour.push_back(0);
    vertColour.push_back(255);
    vertColour.push_back(255);
    vertColour.push_back(255);

    vertColour.push_back(0);
    vertColour.push_back(255);
    vertColour.push_back(0);
    vertColour.push_back(255);


}

void FaceVerticesTop(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] = (-0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (-0.5 + offsetZ);

    vertices[12] = (-0.5 + offsetX);
    vertices[13] = (0.5 + offsetY);
    vertices[14] = (0.5 + offsetZ);

    vertices[15] = (0.5 + offsetX);
    vertices[16] = (0.5 + offsetY);
    vertices[17] = (0.5 + offsetZ);
}

void FaceIndicesTop(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);

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

    vertices[9] =  (-0.5 + offsetX);
    vertices[10] = (-0.5 + offsetY);
    vertices[11] = (-0.5 + offsetZ);

    vertices[12] = (0.5 + offsetX);
    vertices[13] = (-0.5 + offsetY);
    vertices[14] = (-0.5 + offsetZ);

    vertices[15] = (0.5 + offsetX);
    vertices[16] = (-0.5 + offsetY);
    vertices[17] = (0.5 + offsetZ);

}

void FaceIndicesBottom(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(2 + offset);
    //indices.push_back(1 + offset);
    //indices.push_back(3 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);
}

void FaceVerticesFront(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (0.5 + offsetZ);

    vertices[9] = (-0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (0.5 + offsetZ);

    vertices[12] = (-0.5 + offsetX);
    vertices[13] = (-0.5 + offsetY);
    vertices[14] = (0.5 + offsetZ);

    vertices[15] = (0.5 + offsetX);
    vertices[16] = (-0.5 + offsetY);
    vertices[17] = (0.5 + offsetZ);
}

void FaceIndicesFront(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);
}

void FaceVerticesBack(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (-0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (-0.5 + offsetZ);

    vertices[6] = (-0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] =  (0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (-0.5 + offsetZ);

    vertices[12] = (0.5 + offsetX);
    vertices[13] = (-0.5 + offsetY);
    vertices[14] = (-0.5 + offsetZ);

    vertices[15] = (-0.5 + offsetX);
    vertices[16] = (-0.5 + offsetY);
    vertices[17] = (-0.5 + offsetZ);
}

void FaceIndicesBack(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);

}

void FaceVerticesRight(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (0.5 + offsetZ);

    vertices[3] = (0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (-0.5 + offsetZ);

    vertices[6] = (0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (-0.5 + offsetZ);

    vertices[9] =  (0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (0.5 + offsetZ);

    vertices[12] = (0.5 + offsetX);
    vertices[13] = (-0.5 + offsetY);
    vertices[14] = (0.5 + offsetZ);

    vertices[15] = (0.5 + offsetX);
    vertices[16] = (-0.5 + offsetY);
    vertices[17] = (-0.5 + offsetZ);
}

void FaceIndicesRight(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);

}

void FaceVerticesLeft(float* vertices, float offsetX, float offsetY, float offsetZ) {

    vertices[0] = (-0.5 + offsetX);
    vertices[1] = (0.5 + offsetY);
    vertices[2] = (-0.5 + offsetZ);

    vertices[3] = (-0.5 + offsetX);
    vertices[4] = (-0.5 + offsetY);
    vertices[5] = (0.5 + offsetZ);

    vertices[6] = (-0.5 + offsetX);
    vertices[7] = (0.5 + offsetY);
    vertices[8] = (0.5 + offsetZ);

    vertices[9] =  (-0.5 + offsetX);
    vertices[10] = (0.5 + offsetY);
    vertices[11] = (-0.5 + offsetZ);

    vertices[12] = (-0.5 + offsetX);
    vertices[13] = (-0.5 + offsetY);
    vertices[14] = (-0.5 + offsetZ);

    vertices[15] = (-0.5 + offsetX);
    vertices[16] = (-0.5 + offsetY);
    vertices[17] = (0.5 + offsetZ);
}

void FaceIndicesLeft(unsigned short* indices, unsigned short offset) {
    indices[0] = (0 + offset);
    indices[1] = (1 + offset);
    indices[2] = (2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices[3] = (3 + offset);
    indices[4] = (4 + offset);
    indices[5] = (5 + offset);

}