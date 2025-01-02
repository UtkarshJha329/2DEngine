#version 460 core

in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D depthValueTexture;

uniform float cutOffDepth;

int halfNumChunksWidth = 32;
int totalNumChunksWidth = (2 * halfNumChunksWidth) + 1;
int totalNumChunksWidth_Y = 3;

int chunkSize = 32;

void main()
{
//    vec4 depth = texture(depthValueTexture, texCoord);
//
//    vec3 remappedDepth = vec3(depth.x * totalNumChunksWidth * chunkSize, depth.y * totalNumChunksWidth_Y * chunkSize, depth.z * totalNumChunksWidth * chunkSize);
//
//    if(length(vec2(remappedDepth.x, remappedDepth.z)) < cutOffDepth * totalNumChunksWidth * chunkSize){
//        discard;
//    }
//
//    FragColor = vec4(vec3(depth), 1.0);

    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //FragColor = vec4(vec3(0.0), 1.0);
    //FragColor = vec4(vec2(texCoord), 1.0 ,1.0);
}
