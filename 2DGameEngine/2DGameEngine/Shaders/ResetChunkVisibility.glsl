//#version 430 
//
//layout(std430, binding = 4) buffer ChunkVisibilityBuffer
//{
//    int chunksVisibility[];
//};
//
//layout (local_size_x = 32, local_size_y = 1, local_size_z = 32) in;
//
//uniform int chunkSize;
//uniform int numChunksHalfWidth;
//
//void main(){
//
//    int numChunksFullWidth = (2 * numChunksHalfWidth) + 1;
//
//    ivec3 curId = ivec3(gl_GlobalInvocationID);
//
//    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){
//
//        int flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;
//        chunksVisibility[flattenedID] = 0;
//
//    }
//}
//

#version 430

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

layout(std430, binding = 5) buffer ChunkVisibilityValueBuffer
{
    int chunkVisibilityValue;
};

layout (local_size_x = 32, local_size_y = 1, local_size_z = 32) in;

uniform int chunkSize;
uniform int numChunksHalfWidth;

void main(){

    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;

    uvec3 curId = uvec3(gl_GlobalInvocationID);

    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){
        uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;
        chunksVisibility[flattenedID] = chunkVisibilityValue;
    }
}