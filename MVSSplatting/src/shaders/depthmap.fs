#version 330 core
layout(location = 0) out vec4 FragColor;

in fs_in
{
    vec2 TexCoord;
	float viewDepth;
}frag;

uniform int useMask;
uniform int showDepth;
uniform sampler2D colorTex;
uniform sampler2D depthTex;
uniform sampler2D maskTex;

void main()
{
	float mask = texture(maskTex, frag.TexCoord).x;
	if(useMask > 0.5f && mask < 0.5f){
		discard;
	}
	
	FragColor = vec4(texture(colorTex, frag.TexCoord).xyz, 1);
	

	if(showDepth > 0){
		float d = texture(depthTex, frag.TexCoord).x / 30;
		FragColor = vec4(d, d, d, 1);
	}
}