#version 430

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
flat in int faceDir;
in vec3 chunkPos;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

uniform vec4 ambient;
//uniform vec3 viewPos;
uniform float switchColours;

int numChunks = 64;
int numChunksY = 3;

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

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

    if(switchColours != 0){
        //vec3 mappedChunkPos = vec3(mod(chunkPos.x, numChunks), mod(chunkPos.y, numChunksY), mod(chunkPos.z, numChunks));
        vec3 colour = vec3(0.0, 0.0, 0.0);

        int maxLODLevel = 5;
        int chunkSize = 32;
        int numChunksPerLOD = 10;

        vec3 moddedChunkPos = vec3(abs(chunkPos.x) / chunkSize, 0.0, abs(chunkPos.z) / chunkSize);

        if(moddedChunkPos.x < ((maxLODLevel - 4) * numChunksPerLOD) && moddedChunkPos.z < ((maxLODLevel - 4) * numChunksPerLOD)){
            colour = vec3(0.0, 0.0, 0.0);
        }
        else if(moddedChunkPos.x < ((maxLODLevel - 3) * numChunksPerLOD) && moddedChunkPos.z < ((maxLODLevel - 3) * numChunksPerLOD)){
            colour = vec3(0.0, 1.0, 0.0);
        }
        else if(moddedChunkPos.x < ((maxLODLevel - 2) * numChunksPerLOD) && moddedChunkPos.z < ((maxLODLevel - 2) * numChunksPerLOD)){
            colour = vec3(1.0, 0.0, 1.0);
        }
        else if(moddedChunkPos.x < ((maxLODLevel - 1) * numChunksPerLOD) && moddedChunkPos.z < ((maxLODLevel - 1) * numChunksPerLOD)){
            colour = vec3(0.0, 1.0, 1.0);
        }
        else if(moddedChunkPos.x < (maxLODLevel * numChunksPerLOD) && moddedChunkPos.z < (maxLODLevel * numChunksPerLOD)){
            colour = vec3(1.0);
        }

        //finalColor = vec4(mappedChunkPos * 1 / numChunks, 1.0);
        finalColor = vec4(colour, 1.0);
        //finalColor = vec4(moddedChunkPos * 1 / numChunks, 1.0);
    }
}
