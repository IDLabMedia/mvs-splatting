#version 330 core
layout(location = 0) out float FragError;

in vec2 TexCoords;

// input camera parameters
uniform float width;
uniform float height;
uniform mat4 model;
uniform vec2 focal;   // for perspective unprojection
uniform vec2 pp;      // for perspective unprojection

// output camera parameters
uniform mat4 view;

uniform float depth;

uniform sampler2D mainColorTex;
uniform sampler2D neighborColorTex;


void main()
{
	// unproject to find the worldPosition of the current pixel
	vec4 worldPosition;
	// perspective unprojection
	float x = (TexCoords.x * width - pp.x) / focal.x * depth;
	float y = (TexCoords.y * height - (pp.y + 2.0f * (height * 0.5f - pp.y))) / focal.y * depth;
	
	vec4 localPosition = vec4(x,y,depth,1.0f);
	worldPosition = model * localPosition;
	worldPosition = worldPosition / worldPosition.w;
	
	// project onto the neighbor
	vec4 viewPosition = view * worldPosition;
	viewPosition = viewPosition / viewPosition.w;
	if(viewPosition.z > 0){
		float u = viewPosition.x / viewPosition.z * focal.x + pp.x;
		float v = viewPosition.y / viewPosition.z * focal.y + (pp.y + 2.0f * (height * 0.5f - pp.y));
		vec2 screenTexNeighbor = vec2(u / width, v / height);
		
		vec4 color_neighbor = texture(neighborColorTex, screenTexNeighbor);
		vec4 color_main = texture(mainColorTex, TexCoords);
		//FragColor = color_neighbor;
		FragError = length(color_main.xyz - color_neighbor.xyz);
		
		// apply a penalty if screenTexNeighbor is outside of image bounds
		if(screenTexNeighbor.x < 0 || screenTexNeighbor.x > 1 || screenTexNeighbor.y < 0 || screenTexNeighbor.y > 1 ){
			FragError += 0.01f;
		}
	}
	else {
		FragError = 1;
	}
	
	
}
