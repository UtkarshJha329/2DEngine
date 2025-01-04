#version 460 core

in vec2 texCoord;
flat in float zPos;
flat in int flattenedChunkIndex;

layout (location = 0) out vec4 FragColor;

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};


uniform sampler2D depthValueTexture;

uniform float cutOffDepth;

int halfNumChunksWidth = 32;
int totalNumChunksWidth = (2 * halfNumChunksWidth) + 1;
int totalNumChunksWidth_Y = 3;

int chunkSize = 32;

void main()
{

    vec2 viewport_wh = vec2(1280, 720);
    //vec2 ndc = (2.0 * gl_FragCoord.xy / viewport_wh) - 1.0;
    vec2 ndc = (gl_FragCoord.xy / viewport_wh);

    vec2 screenCoord = ndc;
    //vec3 ndc = gl_FragCoord.xyz / gl_FragCoord.w;

    // Step 2: Convert NDC to screen space coordinates
    //vec2 screenCoord = (ndc.xy * 0.5 + 0.5) / vec2(1280, 720);

    vec4 depth = texture(depthValueTexture, vec2(screenCoord.x, screenCoord.y));

    vec3 remappedDepth = vec3(depth.x * totalNumChunksWidth, depth.y * totalNumChunksWidth_Y, depth.z * totalNumChunksWidth);
    remappedDepth = vec3(remappedDepth.x - halfNumChunksWidth, remappedDepth.y, remappedDepth.z - halfNumChunksWidth);
    remappedDepth = remappedDepth * chunkSize;

//    if(length(vec2(remappedDepth.x, remappedDepth.z)) < cutOffDepth * totalNumChunksWidth * chunkSize){
//        discard;
//    }

    //length(vec2(remappedDepth.x, remappedDepth.z))
    if(zPos <= length(vec2(remappedDepth.x, remappedDepth.z))){
        //FragColor = vec4(screenCoord, 0.0, 1.0);
        //FragColor = vec4(vec3(depth), 1.0);
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        chunksVisibility[flattenedChunkIndex] = 1;
    }
    else{
        //FragColor = vec4(screenCoord, 0.0, 1.0);
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        //FragColor = vec4(vec3(depth), 1.0);
    }

    //depth = texture(depthValueTexture, texCoord);
    //FragColor = vec4(vec3(depth), 1.0);

    //FragColor = vec4(vec3(0.0), 1.0);
    //FragColor = vec4(vec2(texCoord), 1.0 ,1.0);
}
