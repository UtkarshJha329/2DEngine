#version 430

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
flat in int faceDir;
in vec3 chunkPos;
in vec3 relChunkPos;
flat in vec3 innerVoxelPos;
flat in float _halfNumChunksWidth;
flat in int _chunkSize;
flat in vec2 curTexCoord;
flat in float isThridOrFourthCorner;
flat in vec3 scaledXYZ;
flat in vec3 curScale;
flat in float curLodLevel;
//in vec3 meshVertexPos;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
layout (location = 0) out vec4 finalColor;
layout (location = 1) out vec4 renderTarget2;
layout (location = 2) out vec4 depthTarget;

// NOTE: Add here your custom variables

uniform vec4 ambient;
//uniform vec3 viewPos;
uniform float switchColours;

int numChunksY = 3;

uniform float numChunksPerLOD;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float RemappedTextureCoord(float uninterpolatedTexCoordOfVertex, float interpolatedTexCoord, float curScale, float curLodLevel){

    float remappedTexCoord = 0.0;

    float numberOfTimesTextureRepeats = curScale / pow(2, curLodLevel);
    float numTexturesOnAxis = 6.0;
    float widthOfEachTextureForFace = 1.0 / numTexturesOnAxis;

    float remappedInterpolatedTexCoord = interpolatedTexCoord * numberOfTimesTextureRepeats;
    float compartmentalizedTexCoord = mod(remappedInterpolatedTexCoord, widthOfEachTextureForFace);

    //float curTexCoordMin = uninterpolatedTexCoordOfVertex  - (numberOfTimesTextureRepeats / (numTexturesOnAxis));

    remappedTexCoord = compartmentalizedTexCoord;

    return remappedTexCoord;

}

void main()
{
//    float clipValue = 0.5;
//    if(meshVertexPos.x > clipValue || meshVertexPos.x < -clipValue
//    || meshVertexPos.y > clipValue || meshVertexPos.y < -clipValue
//    || meshVertexPos.z > clipValue || meshVertexPos.z < -clipValue)
//    {
//        discard;
//    }
    // Texel color fetching from texture sampler
    
    vec2 texCoord = fragTexCoord;
    if(isThridOrFourthCorner == 1.0){

        if(scaledXYZ.x == 1.0 && scaledXYZ.z == 1.0){
            texCoord.x = RemappedTextureCoord(curTexCoord.x, fragTexCoord.x, curScale.x, curLodLevel);
            texCoord.y = RemappedTextureCoord(curTexCoord.y, fragTexCoord.y, curScale.z, curLodLevel);
        }
        else if(scaledXYZ.x == 1.0 && scaledXYZ.y == 1.0){
            texCoord.x = RemappedTextureCoord(curTexCoord.x, fragTexCoord.x, curScale.x, curLodLevel);
            texCoord.y = RemappedTextureCoord(curTexCoord.y, fragTexCoord.y, curScale.y, curLodLevel);
        }
        else if(scaledXYZ.y == 1.0 && scaledXYZ.z == 1.0){
            texCoord.y = RemappedTextureCoord(curTexCoord.y, fragTexCoord.y, curScale.y, curLodLevel);
            texCoord.x = RemappedTextureCoord(curTexCoord.x, fragTexCoord.x, curScale.z, curLodLevel);
        }
    }


    vec4 texelColor = texture(texture0, texCoord);

    if(faceDir == 0){
            finalColor = pow(texelColor, vec4(1.0/2.2));
            //finalColor = pow(vec4(1.0, 1.0, 0.0, 1.0), vec4(1.0/2.2));
    }
    else if(faceDir == 1){
            finalColor = pow(texelColor * vec4(1.0, 0.5, 0.5, 1.0), vec4(1.0/2.2));
            //finalColor = pow(vec4(1.0, 0.5, 0.5, 1.0), vec4(1.0/2.2));
    }
    else if(faceDir == 2){
            finalColor = pow(texelColor * vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0/2.2));
            //finalColor = pow(vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0/2.2));
    }
    else if(faceDir == 3){
            finalColor = pow(texelColor * vec4(0.0, 0.0, 1.0, 1.0), vec4(1.0/2.2));
            //finalColor = pow(vec4(0.0, 0.0, 1.0, 1.0), vec4(1.0/2.2));
    }
    else if(faceDir == 4){
            finalColor = pow(texelColor * vec4(1.0, 0.0, 1.0, 1.0), vec4(1.0/2.2));
            //finalColor = pow(vec4(1.0, 0.0, 1.0, 1.0), vec4(1.0/2.2));
    }
    else if(faceDir == 5){
            finalColor = pow(texelColor * vec4(0.0, 1.0, 1.0, 1.0), vec4(1.0/2.2));
            //finalColor = pow(vec4(0.0, 1.0, 1.0, 1.0), vec4(1.0/2.2));
    }

    float totalNumChunksWidth = (2 * _halfNumChunksWidth) + 1;
    float totalNumChunksWidth_Y = 3;
    vec3 relChunkCoordsAbs = vec3(abs(relChunkPos.x) / _chunkSize, relChunkPos.y / _chunkSize, abs(relChunkPos.z) / _chunkSize);
    vec3 remappedRelChunkCoords = vec3((relChunkPos.x / _chunkSize) + _halfNumChunksWidth, relChunkPos.y / _chunkSize, (relChunkPos.z / _chunkSize) + _halfNumChunksWidth);
    //float totalNumChunksPerLOD = numChunksPerLOD * 2;

    bool drawAll = true;

    if(switchColours == 1){
        //vec3 mappedChunkPos = vec3(mod(chunkPos.x, numChunks), mod(chunkPos.y, numChunksY), mod(chunkPos.z, numChunks));
        vec3 colour = vec3(0.0, 0.0, 0.0);

        int maxLODLevel = 5;
        float dist = length(relChunkCoordsAbs);

        if(dist < ((maxLODLevel - 4) * numChunksPerLOD)){
            colour = vec3(0.0, 0.0, 0.0);
        }
        else if(dist < ((maxLODLevel - 3) * numChunksPerLOD)){
            colour = vec3(0.0, 1.0, 0.0);
        }
        else if(dist < ((maxLODLevel - 2) * numChunksPerLOD)){
            colour = vec3(1.0, 0.0, 1.0);
        }
        else if(dist < ((maxLODLevel - 1) * numChunksPerLOD)){
            colour = vec3(0.0, 1.0, 1.0);
        }
        else{
            //if(dist < (maxLODLevel * numChunksPerLOD))
            colour = vec3(1.0);
        }

        //finalColor = vec4(mappedChunkPos * 1 / numChunks, 1.0);
        
        finalColor = vec4(colour, 1.0);
        
        //finalColor = vec4(relChunkCoords * (1 / halfNumChunksWidth), 1.0);
        //renderTarget2 = vec4(vec3(1 - (length(relChunkCoords) * (1 / numChunks))), 1.0);
        //renderTarget2 = vec4(1 - (vec3(relChunkCoords.x / totalNumChunksWidth, relChunkCoords.y / totalNumChunksWidth_Y, relChunkCoords.z / totalNumChunksWidth)), 1.0);
        //finalColor = vec4(moddedChunkPos * 1 / numChunks, 1.0);
    }
    else if(switchColours == 2)
    {
        //renderTarget2 = vec4(1 - (vec3(remappedRelChunkCoords.x / totalNumChunksWidth, remappedRelChunkCoords.y / totalNumChunksWidth_Y, remappedRelChunkCoords.z / totalNumChunksWidth)), 1.0);
        //renderTarget2 = vec4(vec3(abs(curVoxelPos.x / (halfNumChunksWidth * chunkSize)), curVoxelPos.y, abs(curVoxelPos.z / (halfNumChunksWidth * chunkSize))), 1.0);
        //renderTarget2 = vec4(1 - (vec3(length(relChunkCoords) * (1 / numChunks))), 1.0);
        renderTarget2 += vec4(0.25, 0.25, 0.25, 0.25);
    }


    if(switchColours == 3 || switchColours == 4 || drawAll)
    {
        //float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
        //depthTarget = vec4(vec3(depth), 1.0);
        vec3 remappedChunkCoordsPos = remappedRelChunkCoords * _chunkSize;
        vec3 mappedCoords = vec3((remappedChunkCoordsPos.x + innerVoxelPos.x) / (totalNumChunksWidth * _chunkSize)
                                , (remappedChunkCoordsPos.y + innerVoxelPos.y) / (totalNumChunksWidth_Y * _chunkSize)
                                , (remappedChunkCoordsPos.z + innerVoxelPos.z) / (totalNumChunksWidth * _chunkSize));
//        vec3 mappedCoords = vec3((remappedChunkCoordsPos.x) / (totalNumChunksWidth * _chunkSize)
//                                , (remappedChunkCoordsPos.y) / (totalNumChunksWidth_Y * _chunkSize)
//                                , (remappedChunkCoordsPos.z) / (totalNumChunksWidth * _chunkSize));

        if(false){

            if((innerVoxelPos.x == 0 || innerVoxelPos.x == (_chunkSize - 1))
               || (innerVoxelPos.y == 0 || innerVoxelPos.y == (_chunkSize - 1))
               || (innerVoxelPos.z == 0 || innerVoxelPos.z == (_chunkSize - 1)))
            {
                mappedCoords = vec3(0.0);
            }
        }

        if(false)
        {
            if((innerVoxelPos.x == 0)
               || (innerVoxelPos.y == 0)
               || (innerVoxelPos.z == 0))
            {
                mappedCoords = vec3(0.0);
            }
        }

        if(true)
        {
            if((innerVoxelPos.x ==  (_chunkSize - 1))
               || (innerVoxelPos.y ==  (_chunkSize - 1))
               || (innerVoxelPos.z ==  (_chunkSize - 1)))
            {
                mappedCoords = vec3(0.0);
            }
        }

        depthTarget = vec4(mappedCoords, 1.0);
    }
}
