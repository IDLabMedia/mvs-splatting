#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D inputTex;

void main()
{
	FragColor = vec4(texture(inputTex, vec2(TexCoords.x, 1-TexCoords.y)).rgb, 1);
}