#version 430 core

uniform samplerCube skybox;

in vec3 texCoord;

out vec4 outColor;

void main()
{
	outColor = texture(skybox,texCoord);
}