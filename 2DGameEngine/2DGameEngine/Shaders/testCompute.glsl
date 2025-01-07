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

struct DrawArraysIndirectCommand {
    uint  count;
    uint  instanceCount;
    uint  first;
    uint  baseInstance;
};

struct Plane {
	vec3 normal;
	vec3 pointOnPlane;
};

struct ChunkFacePositionMetaData {
    int startPositionInBigArray;
    int size;
};

layout(std430, binding = 4) buffer ChunkVisibilityBuffer
{
    int chunksVisibility[];
};

layout (std430, binding = 5) buffer AtomicCounter {
    int nextIndirectdrawCommandIndex;
};

layout(std430, binding = 6) buffer IndirectDrawCommandsBuffer
{
    DrawArraysIndirectCommand indirectDrawCommands[];
};

layout(std430, binding = 7) buffer UpFacesMetadataBuffer
{
    ChunkFacePositionMetaData upFacesMetadata[];
};

layout(std430, binding = 8) buffer DownFacesMetadataBuffer
{
    ChunkFacePositionMetaData downFacesMetadata[];
};

layout(std430, binding = 9) buffer FrontFacesMetadataBuffer
{
    ChunkFacePositionMetaData frontFacesMetadata[];
};

layout(std430, binding = 10) buffer BackFacesMetadataBuffer
{
    ChunkFacePositionMetaData backFacesMetadata[];
};

layout(std430, binding = 11) buffer RightFacesMetadataBuffer
{
    ChunkFacePositionMetaData rightFacesMetadata[];
};

layout(std430, binding = 12) buffer LeftFacesMetadataBuffer
{
    ChunkFacePositionMetaData leftFacesMetadata[];
};

struct Position{
    float x, y, z;
};

layout(std430, binding = 13) buffer ChunkDrawCommandPositions
{
    Position chunkDrawCommandPosiitons[];
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
layout (location = 7) uniform vec3 cameraPosition;
layout (location = 8) uniform float diagonalDist;

vec3 up = { 0, 1, 0 };
vec3 down = { 0, -1, 0 };
vec3 front = { 0, 0, 1 };
vec3 back = { 0, 0, -1 };
vec3 right = { 1, 0, 0 };
vec3 left = { -1, 0, 0 };

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

int ChunkFlatIndexWithoutVoxels(vec3 innerChunkIndex, int numChunksWidthFull){

    int numChunksHalfWidth = (numChunksWidthFull - 1) / 2;
    innerChunkIndex = innerChunkIndex + vec3(numChunksHalfWidth, 0, numChunksHalfWidth);
    return int(innerChunkIndex.y * numChunksWidthFull * numChunksWidthFull + innerChunkIndex.z * numChunksWidthFull + innerChunkIndex.x);

}

void CreateIndirectDrawOrderBasedOnFaceVisibility(vec3 innerChunkIndex, vec3 cameraPos, int numChunksWidthFull, int chunkSize)
{
        vec3 dirToChunkFromCamera = (innerChunkIndex * chunkSize) - cameraPos;
        //dirToChunkFromCamera *= -1;

        float dotUp = dot(dirToChunkFromCamera, up);
        float dotDown = dot(dirToChunkFromCamera, down);
        float dotFront = dot(dirToChunkFromCamera, front);
        float dotBack = dot(dirToChunkFromCamera, back);
        float dotRight = dot(dirToChunkFromCamera, right);
        float dotLeft = dot(dirToChunkFromCamera, left);

        int curChunkIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(innerChunkIndex, numChunksWidthFull);
        bool cameraInThisChunkWidthAndBreadth = (innerChunkIndex.x == 0 || innerChunkIndex.z == 0);
        Position drawChunkPos = { innerChunkIndex.x * chunkSize, innerChunkIndex.y * chunkSize, innerChunkIndex.z * chunkSize };

        if (dotUp < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = upFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = upFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }

        if (dotDown < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = downFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = downFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }

        if (dotFront < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = frontFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = frontFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }

        if (dotBack < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = backFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = backFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }

        if (dotRight < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = rightFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = rightFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }

        if (dotLeft < 0 || cameraInThisChunkWidthAndBreadth) {
            int start = leftFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
            int numInstances = leftFacesMetadata[curChunkIndexWithoutVoxels].size;
            if (numInstances > 0) {
                DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                int index = atomicAdd(nextIndirectdrawCommandIndex, 1);
                indirectDrawCommands[index] = curCommand;
                chunkDrawCommandPosiitons[index] = drawChunkPos;
            }
        }
}

void main(){

    int chunkSize = 32;
    uint numChunksHalfWidth = 80;
    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;

    uvec3 curId = uvec3(gl_GlobalInvocationID);
    uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;

    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){
        vec3 curChunkIndex = (vec3(curId) - vec3(numChunksHalfWidth, 0, numChunksHalfWidth));
        vec3 curChunkPos = curChunkIndex * chunkSize;

        float farPlaneDistance = 100000000.0f;

        vec3 cameraModifiedPosition = vec3(0.0) -  (cameraDir * diagonalDist * 2 * 1.414f);

        Plane nearPlane = { nearPlaneNormal, cameraModifiedPosition};
        Plane farPlane = { farPlaneNormal, cameraModifiedPosition + (cameraDir * (farPlaneDistance + diagonalDist * 1.414f))};
        Plane rightPlane = { rightPlaneNormal, cameraModifiedPosition};
        Plane leftPlane = { leftPlaneNormal, cameraModifiedPosition};
        Plane topPlane = { topPlaneNormal, cameraModifiedPosition};
        Plane bottomPlane = { bottomPlaneNormal, cameraModifiedPosition};

        bool isVisible = InFrustum(curChunkPos, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);

        if(isVisible)
        {
            chunksVisibility[flattenedID] = 1;
            //atomicAdd(nextIndirectdrawCommandIndex, 1);
            //ivec3 cameraChunkIndex = ivec3(cameraPosition.x / chunkSize, cameraPosition.y / chunkSize, cameraPosition.z / chunkSize);
            //vec3 curChunkOffsetIndex = curChunkIndex + cameraChunkIndex;
            //CreateIndirectDrawOrderBasedOnFaceVisibility(curChunkOffsetIndex, cameraPosition, int(numChunksFullWidth), chunkSize);
            CreateIndirectDrawOrderBasedOnFaceVisibility(curChunkIndex, cameraPosition, int(numChunksFullWidth), chunkSize);
        }
        else
        {
            chunksVisibility[flattenedID] = 0;
        }
    }
}