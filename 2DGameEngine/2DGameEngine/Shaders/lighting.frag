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

int numChunks = 5;
int numChunksY = 1;

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
        vec3 mappedChunkPos = vec3(mod(chunkPos.x, numChunks), mod(chunkPos.y, numChunksY), mod(chunkPos.z, numChunks)); 
        finalColor = vec4(mappedChunkPos * 1 / numChunks, 1.0);
    }
}
