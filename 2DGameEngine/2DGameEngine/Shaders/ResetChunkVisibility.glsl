#version 430 

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

layout (local_size_x = 32, local_size_y = 1, local_size_z = 32) in;

uniform int chunkSize;
uniform int numChunksHalfWidth;

void main(){

    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;

    uvec3 curId = uvec3(gl_GlobalInvocationID);
    uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;

    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){

        chunksVisibility[flattenedID] = 0;

    }
}