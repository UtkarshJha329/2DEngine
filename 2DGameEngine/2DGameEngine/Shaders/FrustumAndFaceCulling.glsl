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

layout (std430, binding = 5) buffer IndirectDrawCommandIndex {
    int currentIndirectdrawCommandIndex;
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
    float x, y, z, w;
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

layout (location = 9) uniform sampler2D previousDepthInformationTex;
layout (location = 10) uniform float shouldPerformOcclusionCulling;

layout (location = 11) uniform mat4 mvp;

layout (location = 12) uniform float LODLevel;

vec3 up = { 0, 1, 0 };
vec3 down = { 0, -1, 0 };
vec3 front = { 0, 0, 1 };
vec3 back = { 0, 0, -1 };
vec3 right = { 1, 0, 0 };
vec3 left = { -1, 0, 0 };

bool InFrustum(vec3 innerChunkIndex
            , int chunkSize
            , Plane nearPlane
            , Plane farPlane
            , Plane rightPlane
            , Plane leftPlane
            , Plane topPlane
            , Plane bottomPlane)
{
    //return true;
    vec3 innerChunkPos = innerChunkIndex * chunkSize;
    if (dot(nearPlane.normal, normalize(innerChunkPos - nearPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(farPlane.normal, normalize(innerChunkPos - farPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(rightPlane.normal, normalize(innerChunkPos - rightPlane.pointOnPlane)) < 0) {
        return false;
    }

    if (dot(leftPlane.normal, normalize(innerChunkPos - leftPlane.pointOnPlane)) < 0) {
        return false;
    }

    return true;
}

int ChunkFlatIndexWithoutVoxels(vec3 innerChunkIndex, int numChunksWidthFull){

    int numChunksHalfWidth = (numChunksWidthFull - 1) / 2;
    vec3 mappedInnerChunkIndex = innerChunkIndex + vec3(numChunksHalfWidth, 0, numChunksHalfWidth);
    return int(mappedInnerChunkIndex.y * numChunksWidthFull * numChunksWidthFull + mappedInnerChunkIndex.z * numChunksWidthFull + mappedInnerChunkIndex.x);

}

void CreateIndirectDrawOrderBasedOnFaceVisibility(vec3 innerChunkIndex, vec3 cameraPos, int numChunksWidthFull, int chunkSize)
{
        int numChunksHalfWidth = (numChunksWidthFull - 1) / 2;

        ivec3 cameraChunkIndex = ivec3(int(cameraPosition.x) / chunkSize, int(cameraPosition.y) / chunkSize, int(cameraPosition.z) / chunkSize);
        vec3 curChunkIndex = { ((innerChunkIndex.x + cameraChunkIndex.x)), (innerChunkIndex.y), (innerChunkIndex.z + cameraChunkIndex.z) };

        vec3 offsetInnerIndex = curChunkIndex;
        if (offsetInnerIndex.x > numChunksHalfWidth) {
            float diff = offsetInnerIndex.x - numChunksHalfWidth;
            if(diff > numChunksWidthFull){
                diff = mod(diff, numChunksWidthFull);
            }
            offsetInnerIndex.x = -numChunksHalfWidth + diff - 1;
        }

        if (offsetInnerIndex.x < -numChunksHalfWidth) {
            float diff = abs(offsetInnerIndex.x) - numChunksHalfWidth;
            if(diff > numChunksWidthFull){
                diff = mod(diff, numChunksWidthFull);
            }
            offsetInnerIndex.x = numChunksHalfWidth - diff + 1;
        }

        if (offsetInnerIndex.z > numChunksHalfWidth) {
            float diff = offsetInnerIndex.z - numChunksHalfWidth;
            if(diff > numChunksWidthFull){
                diff = mod(diff, numChunksWidthFull);
            }
            offsetInnerIndex.z = -numChunksHalfWidth + diff - 1;
        }

        if (offsetInnerIndex.z < -numChunksHalfWidth) {
            float diff = abs(offsetInnerIndex.z) - numChunksHalfWidth;
            if(diff > numChunksWidthFull){
                diff = mod(diff, numChunksWidthFull);
            }
            offsetInnerIndex.z = numChunksHalfWidth - diff + 1;
        }


        Position drawChunkPos = { (curChunkIndex.x * chunkSize), (curChunkIndex.y * chunkSize), (curChunkIndex.z * chunkSize), 1.0};

        vec3 dirToChunkFromCamera = vec3(drawChunkPos.x, drawChunkPos.y, drawChunkPos.z) - (cameraPos);
        //vec3 dirToChunkFromCamera = vec3(innerChunkIndex.x, innerChunkIndex.y, innerChunkIndex.z) * chunkSize - vec3(0.0, cameraPos.y, 0.0);

        vec3 curChunkRelPosFromCentre = vec3(dirToChunkFromCamera.x, 0.0, dirToChunkFromCamera.z);
        float distToChunk = abs(length(curChunkRelPosFromCentre)) / chunkSize;

        float lodLevelOffset = 32.0;

        vec2 lodDistance1 = { 0, lodLevelOffset - 1 };
        vec2 lodDistance2 = { lodDistance1.y + 1, lodDistance1.y + lodLevelOffset };
        vec2 lodDistance3 = { lodDistance2.y + 1, lodDistance2.y + lodLevelOffset };
        vec2 lodDistance4 = { lodDistance3.y + 1, lodDistance3.y + lodLevelOffset };
        vec2 lodDistance5 = { lodDistance4.y + 1, lodDistance4.y + lodLevelOffset };

        float curLodLevel = 0.0;
        if (distToChunk >= lodDistance1.x && distToChunk <= lodDistance1.y) {
            curLodLevel = LODLevel + 0;
        }
        else if (distToChunk >= lodDistance2.x && distToChunk <= lodDistance2.y) {
            curLodLevel = LODLevel + 1;
        }
        else if (distToChunk >= lodDistance3.x && distToChunk <= lodDistance3.y) {
            curLodLevel = LODLevel + 2;
        }
        else if (distToChunk >= lodDistance4.x && distToChunk <= lodDistance4.y) {
            curLodLevel = LODLevel + 3;
        }
        else if (distToChunk >= lodDistance5.x) {
            curLodLevel = LODLevel + 4;
        }

        if (curLodLevel > 5) {
            curLodLevel = 5;
        }

        drawChunkPos.w = curLodLevel;
        

        //vec2 screenResolution = vec2(1280, 720);

        vec4 clipPositionOfChunk = mvp * vec4(vec3(drawChunkPos.x, drawChunkPos.y, drawChunkPos.z), 1.0);
        vec3 ndcPosition = clipPositionOfChunk.xyz / clipPositionOfChunk.w;
        //vec2 screenSpacePosition = (ndcPosition.xy * 0.5 + 0.5) * screenResolution;
        vec2 uvPosition = (ndcPosition.xy * 0.5 + 0.5);

        vec4 previousDepthAtPixelPosition = texture(previousDepthInformationTex, uvPosition);

        vec4 depth = previousDepthAtPixelPosition;
        float numChunksWidthFull_Y = 3;
        vec3 remappedDepth = vec3(depth.x * numChunksWidthFull, depth.y * numChunksWidthFull_Y, depth.z * numChunksWidthFull);
        remappedDepth = vec3(remappedDepth.x - numChunksHalfWidth, remappedDepth.y, remappedDepth.z - numChunksHalfWidth);
        remappedDepth = remappedDepth * chunkSize;

        float depthTestOffset = 32.0 * 1.732;
        float testDepthAgainst = length(vec2(remappedDepth.x, remappedDepth.z)) + depthTestOffset;

        if ((distToChunk <= testDepthAgainst) || shouldPerformOcclusionCulling == 0.0)
        {
            float dotUp = dot(dirToChunkFromCamera, up);
            float dotDown = dot(dirToChunkFromCamera, down);
            float dotFront = dot(dirToChunkFromCamera, front);
            float dotBack = dot(dirToChunkFromCamera, back);
            float dotRight = dot(dirToChunkFromCamera, right);
            float dotLeft = dot(dirToChunkFromCamera, left);

            int curChunkIndexWithoutVoxels = ChunkFlatIndexWithoutVoxels(offsetInnerIndex, numChunksWidthFull);
            //Position drawChunkPos = { ((innerChunkIndex.x) * chunkSize), (innerChunkIndex.y * chunkSize), ((innerChunkIndex.z)* chunkSize)};

            //bool cameraInThisChunkWidthAndBreadth = ((curChunkIndex.x) == (cameraChunkIndex.x) || (curChunkIndex.z) == (cameraChunkIndex.z));
            //bool cameraInThisChunkWidthAndBreadth = (abs(innerChunkIndex.x) == (0) || abs(innerChunkIndex.z) == (0));
            //bool cameraInThisChunkWidthAndBreadth = ((innerChunkIndex.x) == (0) || (innerChunkIndex.z) == (0));
            //bool cameraInThisChunkWidthAndBreadth = (abs(innerChunkIndex.x) <= (1) || abs(innerChunkIndex.z) <= (1));
            bool cameraInThisChunkWidthAndBreadth = (((innerChunkIndex.x) >= (-1) && innerChunkIndex.x <= 0) || ((innerChunkIndex.z) >= (-1) && innerChunkIndex.z <= 0));
            //bool cameraInThisChunkWidthAndBreadth = (abs(innerChunkIndex.x) <= (0) || abs(innerChunkIndex.z) <= (0));
            bool drawAll = false;

            if (dotUp < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = upFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = upFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

            if (dotDown < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = downFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = downFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

            if (dotFront < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = frontFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = frontFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

            if (dotBack < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = backFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = backFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

            if (dotRight < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = rightFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = rightFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

            if (dotLeft < 0 || cameraInThisChunkWidthAndBreadth || drawAll) {
                int start = leftFacesMetadata[curChunkIndexWithoutVoxels].startPositionInBigArray;
                int numInstances = leftFacesMetadata[curChunkIndexWithoutVoxels].size;
                if (numInstances > 0) {
                    DrawArraysIndirectCommand curCommand = { 4, numInstances, 0, start };
                    int index = atomicAdd(currentIndirectdrawCommandIndex, 1);
                    indirectDrawCommands[index] = curCommand;
                    chunkDrawCommandPosiitons[index] = drawChunkPos;
                }
            }

        }
}

uniform int chunkSize;
uniform int numChunksHalfWidth;

void main(){

    uint numChunksFullWidth = (2 * numChunksHalfWidth) + 1;

    uvec3 curId = uvec3(gl_GlobalInvocationID);
    uint flattenedID = curId.y * numChunksFullWidth * numChunksFullWidth + curId.z * numChunksFullWidth + curId.x;

    if(curId.x < numChunksFullWidth && curId.z < numChunksFullWidth && curId.y < 3){

        if(chunksVisibility[flattenedID] == 1){

            vec3 innerChunkIndex = (vec3(curId) - vec3(numChunksHalfWidth, 0, numChunksHalfWidth));
            ivec3 cameraChunkIndex = ivec3(int(cameraPosition.x) / chunkSize, int(cameraPosition.y) / chunkSize, int(cameraPosition.z) / chunkSize);

            vec3 curChunkIndex = vec3(innerChunkIndex.x + cameraChunkIndex.x, innerChunkIndex.y + 0, innerChunkIndex.z + cameraChunkIndex.z);
            vec3 curChunkPos = curChunkIndex * chunkSize;

            float farPlaneDistance = 100000000.0f;

            vec3 cameraModifiedPosition = vec3(0.0) - (cameraDir * diagonalDist * 2 * 1.414f);

            Plane nearPlane = { nearPlaneNormal, cameraModifiedPosition};
            Plane farPlane = { farPlaneNormal, cameraModifiedPosition + (cameraDir * (farPlaneDistance + diagonalDist * 1.414f))};
            Plane rightPlane = { rightPlaneNormal, cameraModifiedPosition};
            Plane leftPlane = { leftPlaneNormal, cameraModifiedPosition};
            Plane topPlane = { topPlaneNormal, cameraModifiedPosition};
            Plane bottomPlane = { bottomPlaneNormal, cameraModifiedPosition};

            bool isVisible = InFrustum(innerChunkIndex, chunkSize, nearPlane, farPlane, rightPlane, leftPlane, topPlane, bottomPlane);

            if(isVisible)
            //if(true)
            {
                chunksVisibility[flattenedID] = 1;

                CreateIndirectDrawOrderBasedOnFaceVisibility(innerChunkIndex, cameraPosition, int(numChunksFullWidth), chunkSize);
            }
            else
            {
                chunksVisibility[flattenedID] = 0;
            }
        }
        else
        {
            chunksVisibility[flattenedID] = 0;
        }
    }
}