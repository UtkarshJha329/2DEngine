#version 430

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
flat in int faceDir;
in vec3 chunkPos;
in vec3 relChunkPos;
in vec3 innerVoxelPos;
//in vec3 meshVertexPos;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
layout (location = 0) out vec4 finalColor;
layout (location = 1) out vec4 renderTarget2;
layout (location = 2) out vec4 depthTarget;

// NOTE: Add here your custom variables

void main(){}
