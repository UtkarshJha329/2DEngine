//#version 430 
//
//struct DrawArraysIndirectCommand {
//    uint  count;
//    uint  instanceCount;
//    uint  first;
//    uint  baseInstance;
//};
//
//struct Plane {
//	vec3 normal;
//	vec3 pointOnPlane;
//};
//
//layout(std430, binding = 4) buffer ChunkVisibilityBuffer
//{
//    int chunksVisibility[];
//};
//
//layout(std430, binding = 5) buffer IndirectCommandsBuffer
//{
//    DrawArraysIndirectCommand indirectCommands[];
//};
//
//// Local compute unit size
//layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
//
////layout(rgba32f, binding = 0) uniform image2D imgOutput;
//
////void main() {
////    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
////    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
////	
////    value.x = float(texelCoord.x)/(gl_NumWorkGroups.x);
////    value.y = float(texelCoord.y)/(gl_NumWorkGroups.y);
////	
////    imageStore(imgOutput, texelCoord, value);
////}
//
//bool InFrustum(vec3 curChunkPos
//            , Plane nearPlane
//            , Plane farPlane
//            , Plane rightPlane
//            , Plane leftPlane
//            , Plane topPlane
//            , Plane bottomPlane)
//{
//    //return false;
//
//    if (dot(nearPlane.normal, normalize(curChunkPos - nearPlane.pointOnPlane)) < 0) {
//        return false;
//    }
//
//    if (dot(farPlane.normal, normalize(curChunkPos - farPlane.pointOnPlane)) < 0) {
//        return false;
//    }
//
//    if (dot(rightPlane.normal, normalize(curChunkPos - rightPlane.pointOnPlane)) < 0) {
//        return false;
//    }
//
//    if (dot(leftPlane.normal, normalize(curChunkPos - leftPlane.pointOnPlane)) < 0) {
//        return false;
//    }
//
//    return true;
//}
//
//layout (location = 0) uniform vec3 nearPlaneNormal;
//layout (location = 1) uniform vec3 farPlaneNormal;
//layout (location = 2) uniform vec3 rightPlaneNormal;
//layout (location = 3) uniform vec3 leftPlaneNormal;
//layout (location = 4) uniform vec3 topPlaneNormal;
//layout (location = 5) uniform vec3 bottomPlaneNormal;
//
//layout (location = 6) uniform vec3 positionOnFrustum;
//
//layout (location = 7) uniform vec3 cameraPos;
//layout (location = 8) uniform vec3 cameraDir;
//layout (location = 9) uniform float diagonalDist;
//
//layout (location = 10) uniform float time;
//
//int chunkSize = 32;
//
//float rand(vec2 co){
//    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
//}
//
//void main(){
//
//    uint numChunksHalfWidth = 80;
//    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;
//
//    uvec3 curId = uvec3(gl_GlobalInvocationID);
//    uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;
//
////    vec3 position = vec3(0.0) - cameraDir * diagonalDist * 2 * 1.414f;
////    
////    float farPlaneDistance = 100000000.0f;
////
////    Plane nearPlane = { nearPlaneNormal, vec3(0.0)};
////    Plane farPlane = { farPlaneNormal, vec3(0.0) + (cameraDir * (farPlaneDistance + diagonalDist * 1.414f))};
////    Plane rightPlane = { rightPlaneNormal, vec3(0.0)};
////    Plane leftPlane = { leftPlaneNormal, vec3(0.0)};
////    Plane topPlane = { topPlaneNormal, vec3(0.0)};
////    Plane bottomPlane = { bottomPlaneNormal, vec3(0.0)};
////
////    vec3 curChunkPos = (vec3(curId) - vec3(numChunksHalfWidth, 0, numChunksHalfWidth)) * chunkSize;
////
////    bool visible = InFrustum(curChunkPos, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);
//    
//    vec3 curChunkPos = (vec3(curId) - vec3(numChunksHalfWidth, 0, numChunksHalfWidth)) * chunkSize;
//    bool inFrontOfCamera = dot(vec3(0, 1, 0), normalize(curChunkPos)) > 0.0;
//
//    //visible = rand(curChunkPos.xz * time) > 0.5;
//    //bool visible = false;
//
//    if(inFrontOfCamera)
//    {
//        chunksVisibility[flattenedID] = 1;
//    }
//    else
//    {
//        chunksVisibility[flattenedID] = 0;
//    }
//}

#version 430 

struct Plane {
	vec3 normal;
	vec3 pointOnPlane;
};

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

// Local compute unit size
layout (local_size_x = 32, local_size_y = 1, local_size_z = 32) in;

layout (location = 0) uniform vec3 nearPlaneNormal;
layout (location = 1) uniform vec3 farPlaneNormal;
layout (location = 2) uniform vec3 rightPlaneNormal;
layout (location = 3) uniform vec3 leftPlaneNormal;
layout (location = 4) uniform vec3 topPlaneNormal;
layout (location = 5) uniform vec3 bottomPlaneNormal;

layout (location = 6) uniform vec3 cameraDir;
layout (location = 7) uniform float diagonalDist;

bool InFrustum(vec3 curChunkPos
            , Plane nearPlane
            , Plane farPlane
            , Plane rightPlane
            , Plane leftPlane
            , Plane topPlane
            , Plane bottomPlane)
{

    if (dot(nearPlane.normal, normalize(curChunkPos - nearPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(farPlane.normal, normalize(curChunkPos - farPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(rightPlane.normal, normalize(curChunkPos - rightPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(leftPlane.normal, normalize(curChunkPos - leftPlane.pointOnPlane)) < 0) {
        return false;
    }

    return true;
}

void main(){

    int chunkSize = 32;
    uint numChunksHalfWidth = 80;
    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;

    uvec3 curId = uvec3(gl_GlobalInvocationID);
    uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;

    //flattenedID < numChunksFullWidth * numChunksFullWidth * 3
    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){
        vec3 curChunkPos = (vec3(curId) - vec3(numChunksHalfWidth, 0, numChunksHalfWidth));
        curChunkPos *= chunkSize;

        float farPlaneDistance = 100000000.0f;

        //vec3 cameraModifiedPosition = vec3(0.0) - (cameraDir * diagonalDist * 2 * 1.414f);
        vec3 cameraModifiedPosition = vec3(0.0) -  (cameraDir * diagonalDist * 2 * 1.414f);
        //vec3 cameraModifiedPosition = vec3(0.0);

        Plane nearPlane = { nearPlaneNormal, cameraModifiedPosition};
        Plane farPlane = { farPlaneNormal, cameraModifiedPosition + (cameraDir * (farPlaneDistance + diagonalDist * 1.414f))};
        Plane rightPlane = { rightPlaneNormal, cameraModifiedPosition};
        Plane leftPlane = { leftPlaneNormal, cameraModifiedPosition};
        Plane topPlane = { topPlaneNormal, cameraModifiedPosition};
        Plane bottomPlane = { bottomPlaneNormal, cameraModifiedPosition};

        bool isVisible = InFrustum(curChunkPos, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);

        //bool inFrontOfCamera = dot(vec3(0, 0, 1), normalize(curChunkPos)) > 0.0;
        //bool inFrontOfCamera = dot(cameraDir, normalize(curChunkPos)) > 0.0;
        //if(inFrontOfCamera)
        if(isVisible)
        {
            chunksVisibility[flattenedID] = 1;
        }
        else
        {
            chunksVisibility[flattenedID] = 0;
        }
    }
}