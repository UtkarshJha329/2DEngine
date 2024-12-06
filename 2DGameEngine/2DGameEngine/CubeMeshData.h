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

void TexCoords(std::vector<float>& texCoords) {

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

    texCoords.push_back(xTopLeft);
    texCoords.push_back(yTopLeft);

    texCoords.push_back(xTopLeft + (1.0f / (float)numTexturesX));
    texCoords.push_back(yTopLeft + (1.0f / (float)numTexturesY));

    texCoords.push_back(xTopLeft + (1.0f / (float)numTexturesX));
    texCoords.push_back(yTopLeft);

    texCoords.push_back(xTopLeft);
    texCoords.push_back(yTopLeft);

    texCoords.push_back(xTopLeft);
    texCoords.push_back(yTopLeft + (1.0f / (float)numTexturesY));

    texCoords.push_back(xTopLeft + (1.0f / (float)numTexturesX));
    texCoords.push_back(yTopLeft + (1.0f / (float)numTexturesY));
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

void FaceVerticesTop(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);
}

void FaceIndicesTop(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);

}

void FaceVerticesBottom(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

}

void FaceIndicesBottom(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(2 + offset);
    //indices.push_back(1 + offset);
    //indices.push_back(3 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);
}

void FaceVerticesFront(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);
}

void FaceIndicesFront(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);
}

void FaceVerticesBack(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);
}

void FaceIndicesBack(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);

}

void FaceVerticesRight(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);
}

void FaceIndicesRight(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);

}

void FaceVerticesLeft(std::vector<float>& vertices, float offsetX, float offsetY, float offsetZ) {

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(-0.5 + offsetZ);

    vertices.push_back(-0.5 + offsetX);
    vertices.push_back(-0.5 + offsetY);
    vertices.push_back(0.5 + offsetZ);
}

void FaceIndicesLeft(std::vector<unsigned short>& indices, unsigned short offset) {
    indices.push_back(0 + offset);
    indices.push_back(1 + offset);
    indices.push_back(2 + offset);

    //indices.push_back(0 + offset);
    //indices.push_back(3 + offset);
    //indices.push_back(1 + offset);

    indices.push_back(3 + offset);
    indices.push_back(4 + offset);
    indices.push_back(5 + offset);

}