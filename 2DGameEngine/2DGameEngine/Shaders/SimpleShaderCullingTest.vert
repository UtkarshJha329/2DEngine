#version 460 core

#extension GL_ARB_shader_draw_parameters: enable

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;

layout (location = 3) in int instancePosition;

// Input uniform values
uniform mat4 mvp;

struct Position{
    float x, y, z;
};

layout(std430, binding = 3) buffer ChunkPositionBuffer
{
    Position chunkPosition[];
};

out vec2 texCoord;

void main() {

    vec3 curPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z);

    mat4 translationMatrix = mat4(1.0);  // Identity matrix
    translationMatrix[3] = vec4(curPos, 1.0);

    mat4 tempMVP = mat4(1.0);

    float scale = 1.0;
    tempMVP[0][0] = scale;
    tempMVP[1][1] = scale;
    tempMVP[2][2] = scale;

    //gl_Position = mvp * translationMatrix  * vec4(vertexPosition, 1.0);
    gl_Position = tempMVP * vec4(vertexPosition, 1.0);
    //gl_Position = vec4(vertexPosition, 1.0);
    texCoord = vertexTexCoord;
}