#version 330 core
layout(location = 0) in vec2 TexCoords;

out vec3 vs_position;
out vec3 vs_color;
out float vs_scale;
out int vs_mask;

uniform float width;
uniform float height;
uniform mat4 model;
uniform vec2 focal;
uniform vec2 pp;

uniform float diameter;

uniform sampler2D colorTex;
uniform sampler2D depthTex;
uniform sampler2D maskTex;

void main() {
	
	float depth = texture(depthTex, TexCoords).x;
	
	// perspective unprojection
	float x = (TexCoords.x * width - pp.x) / focal.x * depth;
	float y = (TexCoords.y * height - (pp.y + 2.0f * (height * 0.5f - pp.y))) / focal.y * depth;
	
	vec4 localPosition = vec4(x,y,depth,1.0f);
	vec4 worldPosition = model * localPosition;
	worldPosition = worldPosition / worldPosition.w;
	
    vs_position = worldPosition.xyz;
	vs_color = texture(colorTex, TexCoords).rgb;
	vs_scale = length(localPosition.xyz) * diameter;
	
	vs_mask = texture(maskTex, TexCoords).x > 0.5f? 1 : 0;
}
