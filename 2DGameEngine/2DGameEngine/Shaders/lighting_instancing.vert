#version 330

// Input vertex attributes
layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
//in vec4 vertexColor;      // Not required

layout (location = 3) in int instancePosition;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// NOTE: Add here your custom variables

uniform vec3 curChunkPos;

void main()
{
    vec3 curVoxelPos = vec3((instancePosition >> 10) & 31, (instancePosition >> 5) & 31, instancePosition & 31);  
    vec3 curPos = curChunkPos + curVoxelPos;
    mat4 translationMatrix = mat4(1.0);  // Identity matrix
    translationMatrix[3] = vec4(curPos, 1.0);

    //// Construct the rotation matrix (using rotationAxis and rotationAngle)
    //mat4 rotationMatrix = mat4(1.0); // Identity matrix
    //float cosA = 1;
    //float sinA = 0;
    //float oneMinusCos = 1.0 - cosA;
    //
    //vec3 rotationAxis = {0.0, 1.0, 0.0};
    //// Rodrigues' rotation formula for rotation matrix
    //rotationMatrix[0] = vec4(cosA + rotationAxis.x * rotationAxis.x * oneMinusCos, rotationAxis.x * rotationAxis.y * oneMinusCos - rotationAxis.z * sinA, rotationAxis.x * rotationAxis.z * oneMinusCos + rotationAxis.y * sinA, 0.0);
    //rotationMatrix[1] = vec4(rotationAxis.y * rotationAxis.x * oneMinusCos + rotationAxis.z * sinA, cosA + rotationAxis.y * rotationAxis.y * oneMinusCos, rotationAxis.y * rotationAxis.z * oneMinusCos - rotationAxis.x * sinA, 0.0);
    //rotationMatrix[2] = vec4(rotationAxis.z * rotationAxis.x * oneMinusCos - rotationAxis.y * sinA, rotationAxis.z * rotationAxis.y * oneMinusCos + rotationAxis.x * sinA, cosA + rotationAxis.z * rotationAxis.z * oneMinusCos, 0.0);
    //rotationMatrix[3] = vec4(0.0, 0.0, 0.0, 1.0);
    //
    //// Construct the scaling matrix
    //mat4 scaleMatrix = mat4(1.0);  // Identity matrix
    //scaleMatrix[0] = vec4(1, 0.0, 0.0, 0.0);
    //scaleMatrix[1] = vec4(0.0, 1, 0.0, 0.0);
    //scaleMatrix[2] = vec4(0.0, 0.0, 1, 0.0);
    //scaleMatrix[3] = vec4(0.0, 0.0, 0.0, 1.0);
    //
    //// Combine the matrices: model = translation * rotation * scale
    //mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

    // Send vertex attributes to fragment shader
    //fragPosition = vec3(translationMatrix * vec4(vertexPosition, 1.0));
    
    fragPosition = vec3(translationMatrix * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    //fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    // Calculate final vertex position, note that we multiply mvp by instanceTransform
    gl_Position = mvp * translationMatrix  * vec4(vertexPosition, 1.0);
}
