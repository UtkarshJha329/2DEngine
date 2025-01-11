#version 460 core

in vec2 texCoord;
flat in float zPos;
flat in int flattenedChunkIndex;
flat in int _chunkSize;
flat in int _halfNumChunksWidth;
in vec3 relChunkPos;

layout (location = 0) out vec4 FragColor;

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

uniform sampler2D depthValueTexture;
uniform float cutOffDepth;

void main()
{
    int totalNumChunksWidth = (2 * _halfNumChunksWidth) + 1;
    int totalNumChunksWidth_Y = 3;
    
    vec2 viewport_wh = vec2(1280, 720);
    vec2 ndc = (gl_FragCoord.xy / viewport_wh);

    vec2 screenCoord = ndc;
    vec4 depth = texture(depthValueTexture, vec2(screenCoord.x, screenCoord.y));

    vec3 remappedDepth = vec3(depth.x * totalNumChunksWidth, depth.y * totalNumChunksWidth_Y, depth.z * totalNumChunksWidth);
    remappedDepth = vec3(remappedDepth.x - _halfNumChunksWidth, remappedDepth.y, remappedDepth.z - _halfNumChunksWidth);
    remappedDepth = remappedDepth * _chunkSize;

    float testDepth = 0.0;

    testDepth = zPos;
    float depthTestOffset = 32.0 * 1.732;
    float testDepthAgainst = 0.0;
    //testDepthAgainst = length(vec2(remappedDepth.x, remappedDepth.z));
    testDepthAgainst = 32 * 3;

    if(testDepth < testDepthAgainst + depthTestOffset){
        FragColor = vec4(0.0, 0.0, 1.0, 0.25);
        chunksVisibility[flattenedChunkIndex] = 1;
    }
    else if(testDepth == testDepthAgainst + depthTestOffset){
        FragColor = vec4(0.0, 1.0, 0.0, 0.25);
        chunksVisibility[flattenedChunkIndex] = 1;
    }
    else if(testDepth > testDepthAgainst + depthTestOffset){
        //FragColor = vec4(vec3(depth), 0.25);
        FragColor = vec4(1.0, 0.0, 0.0, 0.25);
        // VVVVVVVVVVVVVVVVVVVVVVVVVVV Doesn't work.
        //chunksVisibility[flattenedChunkIndex] = 0;
        //FragColor = vec4(1.0, 0.0, 0.0, 0.25);
    }
}
