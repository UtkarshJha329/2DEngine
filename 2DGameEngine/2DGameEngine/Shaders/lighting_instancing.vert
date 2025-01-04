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

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

//layout (location = 4) in vec3 chunkPositionInstanced;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out int faceDir;
out vec3 chunkPos;
out vec3 relChunkPos;
out vec3 innerVoxelPos;
//out vec3 meshVertexPos;

// NOTE: Add here your custom variables

uniform vec3 curChunkPos;
uniform vec3 cameraPos;

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

//vec3 verticesUP[4] = vec3[4]( vec3(-0.5, 0.5, -0.5), 
//                              vec3(-0.5, 0.5, 2.0),
//                              vec3(2.0, 0.5, -0.5),
//                              vec3(-0.5, 0.5, -0.5));
//
//vec3 verticesDOWN[4] = vec3[4]( vec3(0.5, -0.5, -0.5),
//                                vec3(0.5, -0.5, 0.5), 
//                                vec3(-0.5, -0.5, -0.5),
//                                vec3(-0.5, -0.5, 0.5));
//
//
//vec3 verticesFRONT[4] = vec3[4]( vec3(0.5, -0.5, 0.5),
//                                vec3(0.5, 2.0, 0.5),  
//                                vec3(-2.0, -0.5, 0.5),
//                                vec3(0.5, -0.5, 0.5));
//
//vec3 verticesBACK[4] = vec3[4]( vec3(-0.5, -0.5, -0.5),
//                                vec3(-0.5, 2.0, -0.5),
//                                vec3(2.0, -0.5, -0.5),
//                                vec3(-0.5, -0.5, -0.5));
//
//vec3 verticesRIGHT[4] = vec3[4]( vec3(0.5, 0.5, -0.5),
//                                vec3(0.5, 0.5, 2.0),  
//                                vec3(0.5, -2.0, -0.5),
//                                vec3(0.5, 0.5, -0.5));
//
//vec3 verticesLEFT[4] = vec3[4]( vec3(-0.5, -0.5, -0.5),
//                                vec3(-0.5, -0.5, 2.0),
//                                vec3(-0.5, 2.0, -0.5),
//                                vec3(-0.5, -0.5, -0.5));

uniform float lodScale;
uniform float renderAll;

float curScale;

int xPosInPackedInt = 10;
int yPosInPackedInt = 5;
int zPosInPackedInt = 0;

int faceDirPosInPackedInt = 16;
int curScalePosInPackedInt = 19;

void main()
{
    int chunkSize = 32;
    int halfNumChunksWidth = 80;
    int totalNumChunksWidth = (2 * halfNumChunksWidth) + 1;
    chunkPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z);
    relChunkPos = vec3(chunkPos.x - cameraPos.x, chunkPos.y, chunkPos.z - cameraPos.z);

    vec3 relChunkCoords = vec3((relChunkPos.x / chunkSize) + halfNumChunksWidth, relChunkPos.y / chunkSize, (relChunkPos.z / chunkSize) + halfNumChunksWidth);
    int flattenedChunkCoords = int(relChunkCoords.y * totalNumChunksWidth * totalNumChunksWidth + relChunkCoords.z * totalNumChunksWidth + relChunkCoords.x);

//  chunksVisibility[flattenedChunkCoords] != 2 && chunksVisibility[flattenedChunkCoords] != 0
//  chunksVisibility[flattenedChunkCoords] == 1
    if(true){
//    if(renderAll == 1 || chunksVisibility[flattenedChunkCoords] == 1){
        
        vec3 curVoxelPosUncompressed = vec3((instancePosition >> xPosInPackedInt) & 31, (instancePosition >> yPosInPackedInt) & 31, (instancePosition >> zPosInPackedInt) & 31);
        faceDir = (instancePosition >> faceDirPosInPackedInt) & 7;

        curScale = (instancePosition >> curScalePosInPackedInt) & 31;


        vec3 curPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z) + curVoxelPosUncompressed;
        innerVoxelPos = curVoxelPosUncompressed;

        //vec3 curPos = chunkPosition + curVoxelPos;
        mat4 translationMatrix = mat4(1.0);  // Identity matrix
        translationMatrix[3] = vec4(curPos, 1.0);
    
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
        curVertex *= curScale;

        fragPosition = vec3(translationMatrix * vec4(curVertex, 1.0));
        fragTexCoord = vertexTexCoord;
        //fragColor = vertexColor;
        fragNormal = normalize(vec3(matNormal * vec4(curVertex, 1.0)));

        gl_Position = mvp * translationMatrix  * vec4(curVertex, 1.0);

        //using current position X Y and Z coordinates scale appropriately?
    }
    else
    {
        gl_Position = vec4(vec3(0.0), 1.0);
    }
}

