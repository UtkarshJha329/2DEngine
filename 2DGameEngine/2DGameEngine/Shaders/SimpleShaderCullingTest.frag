#version 460 core

in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D depthValueTexture;

void main()
{
    FragColor = vec4(vec3(texture(depthValueTexture, texCoord)), 1.0);
    //FragColor = vec4(vec2(texCoord), 1.0 ,1.0);
}
