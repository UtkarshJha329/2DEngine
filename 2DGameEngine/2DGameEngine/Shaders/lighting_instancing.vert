#version 460

#extension GL_ARB_shader_draw_parameters: enable

// Input vertex attributes
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
//in vec4 vertexColor;      // Not required

layout (location = 3) in int instancePosition;

struct Position{
    float x, y, z, w;
};

layout(std430, binding = 13) buffer ChunkPositionBuffer
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
out vec2 fragTexCoord;
out int faceDir;
out vec3 chunkPos;
out vec3 relChunkPos;
flat out vec3 innerVoxelPos;
flat out float _halfNumChunksWidth;
flat out int _chunkSize;
flat out float _lodLevel;
flat out vec2 curTexCoord;
flat out float isThridOrFourthCorner;
flat out vec3 scaledXYZ;
flat out vec3 curScale;
flat out float curLodLevel;

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

//Triangle Points.
//vec3 verticesUP[4] = vec3[4]( vec3(-0.5, 0.5, -0.5), 
//                              vec3(-0.5, 0.5, 2.0),
//                              vec3(2.0, 0.5, -0.5),
//                              vec3(-0.5, 0.5, -0.5));
//
////vec3 verticesDOWN[4] = vec3[4]( vec3(0.5, -0.5, -0.5),
////                                vec3(0.5, -0.5, 0.5), 
////                                vec3(-0.5, -0.5, -0.5),
////                                vec3(-0.5, -0.5, 0.5));
////
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

uniform int chunkSize;
uniform int halfNumChunksWidth;

float curScaleX;
float curScaleY;
float curScaleZ;

int xPosInPackedInt = 10;
int yPosInPackedInt = 5;
int zPosInPackedInt = 0;

int faceDirPosInPackedInt = 16;

//int curScalePosInPackedIntX = 19;
//int curScalePosInPackedIntY = 24;
//int curScalePosInPackedIntZ = 29;

int curScalePosInPackedIntA = 19;
int curScalePosInPackedIntB = 24;
//int curScalePosInPackedIntGen = 29;

float curScaleA;
float curScaleB;
//float curScaleGen;


void main()
{
    _halfNumChunksWidth = float(halfNumChunksWidth);
    _chunkSize = chunkSize;

    int totalNumChunksWidth = (2 * halfNumChunksWidth) + 1;
    chunkPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z);

    ivec3 cameraChunkIndex = ivec3(cameraPos) / chunkSize;
    ivec3 cameraChunkIndexPos = cameraChunkIndex * chunkSize;

    relChunkPos = vec3(chunkPos.x - cameraChunkIndexPos.x, chunkPos.y, chunkPos.z - cameraChunkIndexPos.z);

    vec3 relChunkCoords = vec3((relChunkPos.x / chunkSize) + halfNumChunksWidth, relChunkPos.y / chunkSize, (relChunkPos.z / chunkSize) + halfNumChunksWidth);
    int flattenedChunkCoords = int(relChunkCoords.y * totalNumChunksWidth * totalNumChunksWidth + relChunkCoords.z * totalNumChunksWidth + relChunkCoords.x);

    if(gl_VertexID == 2 || gl_VertexID == 3){
        isThridOrFourthCorner = 1.0;
    }
    else{
        isThridOrFourthCorner = 0.0;
    }

//  chunksVisibility[flattenedChunkCoords] != 2 && chunksVisibility[flattenedChunkCoords] != 0
//  chunksVisibility[flattenedChunkCoords] == 1
    if(true){
//    if(renderAll == 1 || chunksVisibility[flattenedChunkCoords] == 1){
        
        vec3 curVoxelPosUncompressed = vec3((instancePosition >> xPosInPackedInt) & 31, (instancePosition >> yPosInPackedInt) & 31, (instancePosition >> zPosInPackedInt) & 31);
        faceDir = (instancePosition >> faceDirPosInPackedInt) & 7;

//        curScaleX = (instancePosition >> curScalePosInPackedIntX) & 31;
//        curScaleZ = (instancePosition >> curScalePosInPackedIntZ) & 31;
//        curScaleY = (instancePosition >> curScalePosInPackedIntY) & 31;

        curScaleA = (instancePosition >> curScalePosInPackedIntA) & 31;
        curScaleB = (instancePosition >> curScalePosInPackedIntB) & 31;
//        curScaleGen = (instancePosition >> curScalePosInPackedIntGen) & 31;

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

        //meshVertexPos = curVertex;

        curVertex += 0.5;
        //curVertex = vec3(curVertex.x * curScaleX, curVertex.y * curScaleY, curVertex.z * curScaleZ);
        
        _lodLevel = chunkPosition[gl_DrawIDARB].w;
        curLodLevel = _lodLevel;
        float lodScaleValue = pow(2, _lodLevel);
        //float oneByLodScaleValue = 1 / lodScaleValue;

        //fragTexCoord = vertexTexCoord;
        curTexCoord = vertexTexCoord;
        fragTexCoord = vertexTexCoord;
 
        if(faceDir == 0 || faceDir == 1){
            //lodScaleValue = faceDir == 1 ? 0.0 : lodScaleValue;
            //curVertex = vec3(curVertex.x * curScaleA, curVertex.y + lodScaleValue, curVertex.z * curScaleB);
            curVertex = vec3(curVertex.x * curScaleA, curVertex.y, curVertex.z * curScaleB);
            scaledXYZ = vec3(1.0, 0.0, 1.0);
            curScale = vec3(curScaleA, lodScaleValue, curScaleB);
        }
        else if(faceDir == 2 || faceDir == 3){
            //lodScaleValue = faceDir == 3 ? 0.0 : lodScaleValue;
            //curVertex = vec3(curVertex.x * curScaleA, curVertex.y * curScaleB, curVertex.z + lodScaleValue);
            curVertex = vec3(curVertex.x * curScaleA, curVertex.y * curScaleB, curVertex.z);
            scaledXYZ = vec3(1.0, 1.0, 0.0);
            curScale = vec3(curScaleA, curScaleB, lodScaleValue);
        }
        else if(faceDir == 4 || faceDir == 5){
            //lodScaleValue = faceDir == 5 ? 0.0 : lodScaleValue;
            //curVertex = vec3(curVertex.x + lodScaleValue, curVertex.y * curScaleB, curVertex.z * curScaleA);
            curVertex = vec3(curVertex.x, curVertex.y * curScaleB, curVertex.z * curScaleA);
            scaledXYZ = vec3(0.0, 1.0, 1.0);
            curScale = vec3(lodScaleValue, curScaleB, curScaleA);
        }

        gl_Position = mvp * translationMatrix  * vec4(curVertex, 1.0);

        //using current position X Y and Z coordinates scale appropriately?
    }
    else
    {
        gl_Position = vec4(vec3(0.0), 1.0);
    }
}

