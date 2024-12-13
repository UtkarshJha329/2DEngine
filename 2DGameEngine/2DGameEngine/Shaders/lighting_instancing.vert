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

// NOTE: Add here your custom variables

uniform vec3 curChunkPos;

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

void main()
{
    vec3 curVoxelPos = vec3((instancePosition >> 10) & 31, (instancePosition >> 5) & 31, instancePosition & 31);
    faceDir = (instancePosition >> 16) & 7;



    vec3 curPos = vec3(chunkPosition[gl_DrawIDARB].x, chunkPosition[gl_DrawIDARB].y, chunkPosition[gl_DrawIDARB].z) + curVoxelPos;
    //vec3 curPos = chunkPosition + curVoxelPos;
    mat4 translationMatrix = mat4(1.0);  // Identity matrix
    translationMatrix[3] = vec4(curPos, 1.0);
    
    vec3 curVertex = vec3(0.0);
    
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


    fragPosition = vec3(translationMatrix * vec4(curVertex, 1.0));
    fragTexCoord = vertexTexCoord;
    //fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(curVertex, 1.0)));

    // Calculate final vertex position, note that we multiply mvp by instanceTransform
    gl_Position = mvp * translationMatrix  * vec4(curVertex, 1.0);
}
