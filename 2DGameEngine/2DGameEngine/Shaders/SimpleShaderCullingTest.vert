#version 460

#extension GL_ARB_shader_draw_parameters: enable

// Input vertex attributes
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
//in vec4 vertexColor;      // Not required

layout (location = 3) in int instancePosition;

struct Position{
    float x, y, z;
};

layout(std430, binding = 3) buffer ChunkPositionBuffer
{
    Position chunkPosition[];
};

//layout (location = 4) in vec3 chunkPositionInstanced;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
flat out float zPos;
flat out int flattenedChunkIndex;
flat out int _chunkSize;
flat out int _halfNumChunksWidth;
//out vec3 meshVertexPos;

// NOTE: Add here your custom variables

uniform vec3 curChunkPos;
uniform vec3 cameraPos;
uniform int chunkSize;
uniform int numChunksHalfWidth;

vec3 verticesUP[4] = vec3[4]( vec3(-0.5, 0.5, -0.5),  
                              vec3(-0.5, 0.5, 0.5),   
                              vec3(0.5, 0.5, -0.5),   
                              vec3(0.5, 0.5, 0.5));   

vec3 verticesDOWN[4] = vec3[4]( vec3(0.5, -0.5, -0.5),
                                vec3(0.5, -0.5, 0.5), 
                                vec3(-0.5, -0.5, -0.5),
                                vec3(-0.5, -0.5, 0.5));

vec3 verticesFRONT[4] = vec3[4]( vec3(0.5, -0.5, 0.5),
                                vec3(0.5, 0.5, 0.5),  
                                vec3(-0.5, -0.5, 0.5),
                                vec3(-0.5, 0.5, 0.5));

vec3 verticesBACK[4] = vec3[4]( vec3(-0.5, -0.5, -0.5),
                                vec3(-0.5, 0.5, -0.5),
                                vec3(0.5, -0.5, -0.5),
                                vec3(0.5, 0.5, -0.5));

vec3 verticesRIGHT[4] = vec3[4]( vec3(0.5, 0.5, -0.5),
                                vec3(0.5, 0.5, 0.5),  
                                vec3(0.5, -0.5, -0.5),
                                vec3(0.5, -0.5, 0.5));

vec3 verticesLEFT[4] = vec3[4]( vec3(-0.5, -0.5, -0.5),
                                vec3(-0.5, -0.5, 0.5),
                                vec3(-0.5, 0.5, -0.5),
                                vec3(-0.5, 0.5, 0.5));


int xPosInPackedInt = 16;
int yPosInPackedInt = 8;
int zPosInPackedInt = 0;

int unpackMask = 255;

int faceDirPosInPackedInt = 24;
int curScalePosInPackedInt = 27;

int scale = 1;

int faceDir = 0;

void main()
{
    vec3 cameraChunkPos = cameraPos / chunkSize;
    _chunkSize = chunkSize;
    _halfNumChunksWidth = numChunksHalfWidth;
    int totalNumChunksWidth = (2 * numChunksHalfWidth) + 1;
    
    vec3 curPos = vec3((instancePosition >> xPosInPackedInt) & unpackMask, (instancePosition >> yPosInPackedInt) & unpackMask, (instancePosition >> zPosInPackedInt) & unpackMask);
    
    flattenedChunkIndex = int(curPos.y * totalNumChunksWidth * totalNumChunksWidth + curPos.z * totalNumChunksWidth + curPos.x);
    
    //curPos = curPos - vec3(numChunksHalfWidth, 0, numChunksHalfWidth);
    curPos = curPos - vec3(numChunksHalfWidth, 0, numChunksHalfWidth);
    vec3 drawingCurPos = vec3(curPos.x + cameraChunkPos.x, curPos.y, curPos.z + cameraChunkPos.z);
    //curPos = vec3(curPos.x + cameraChunkPos.x, curPos.y, curPos.z + cameraChunkPos.z);
    curPos *= chunkSize;
    drawingCurPos *= chunkSize;

    //zPos = length(vec2(curPos.x, curPos.z));

    faceDir = (instancePosition >> faceDirPosInPackedInt) & 7;

    mat4 translationMatrix = mat4(1.0);  // Identity matrix
    translationMatrix[3] = vec4(drawingCurPos, 1.0);
    
    vec3 curVertex = vec3(0.0);
    
    float multiplyFac = 1.0;
    if(faceDir == 0){
        curVertex = verticesUP[gl_VertexID];
    }
    else if(faceDir == 1){
        curVertex = verticesDOWN[gl_VertexID];
    }
    else if(faceDir == 2){
        curVertex = verticesFRONT[gl_VertexID];
    }
    else if(faceDir == 3){
        curVertex = verticesBACK[gl_VertexID];
    }
    else if(faceDir == 4){
        curVertex = verticesRIGHT[gl_VertexID];
    }
    else if(faceDir == 5){
        curVertex = verticesLEFT[gl_VertexID];
    }

    curVertex += 0.5;
    curVertex *= scale;

    gl_Position = mvp * translationMatrix  * vec4(curVertex, 1.0);
}
