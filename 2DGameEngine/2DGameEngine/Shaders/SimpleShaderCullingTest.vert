//#version 460 core
//
//#extension GL_ARB_shader_draw_parameters: enable
//
//layout (location = 0) in vec3 vertexPosition;
//layout (location = 1) in vec2 vertexTexCoord;
//
//layout (location = 3) in int instancePosition;
//
//// Input uniform values
//uniform mat4 mvp;
//
//struct Position{
//    float x, y, z;
//};
//
//layout(std430, binding = 3) buffer ChunkPositionBuffer
//{
//    Position chunkPosition[];
//};
//
//out vec2 texCoord;
//
//void main() {
//
//    //vec3 curPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z);
//    vec3 curPos = vec3((instancePosition >> 10) & 31, (instancePosition >> 5) & 31, (instancePosition) & 31);
//
//    mat4 translationMatrix = mat4(1.0);  // Identity matrix
//    translationMatrix[3] = vec4(curPos, 1.0);
//
//    mat4 tempMVP = mat4(1.0);
//
//    float scale = 1.0;
//    tempMVP[0][0] = scale;
//    tempMVP[1][1] = scale;
//    tempMVP[2][2] = scale;
//
//    //gl_Position = mvp * translationMatrix  * vec4(vertexPosition, 1.0);
//    gl_Position = tempMVP * vec4(vertexPosition, 1.0);
//    //gl_Position = vec4(vertexPosition, 1.0);
//    texCoord = vertexTexCoord;
//}

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
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out int faceDir;
out vec3 chunkPos;
out vec3 relChunkPos;
out vec3 innerVoxelPos;
flat out float zPos;
out vec2 texCoord;
flat out int flattenedChunkIndex;
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


int xPosInPackedInt = 10;
int yPosInPackedInt = 5;
int zPosInPackedInt = 0;

int faceDirPosInPackedInt = 19;
int curScalePosInPackedInt = 19;

int chunkSize = 32;
int numChunksHalfWidth = 64;
int totalNumChunksWidth = (2 * numChunksHalfWidth) + 1;

void main()
{
    vec3 curPos = vec3((instancePosition >> 12) & 63, (instancePosition >> 6) & 63, (instancePosition) & 63);
    
    flattenedChunkIndex = int(curPos.y * totalNumChunksWidth * totalNumChunksWidth + curPos.z * totalNumChunksWidth + curPos.x);
    
    curPos = curPos - vec3(numChunksHalfWidth, 0, numChunksHalfWidth);
    curPos *= chunkSize;

    zPos = length(vec2(abs(curPos.x), abs(curPos.z)));

    faceDir = (instancePosition >> faceDirPosInPackedInt) & 7;
    //float scale = (instancePosition >> 22) & 31;
    float scale = 32;

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
    curVertex *= scale;

    fragPosition = vec3(translationMatrix * vec4(curVertex, 1.0));
    fragTexCoord = vertexTexCoord;
    texCoord = vertexTexCoord;
    //fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(curVertex, 1.0)));

    gl_Position = mvp * translationMatrix  * vec4(curVertex, 1.0);

    //using current position X Y and Z coordinates scale appropriately?
}
