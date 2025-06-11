#version 330 core
layout(location = 0) out float FragMask;

in vec2 TexCoords;

// input camera parameters
uniform float width;
uniform float height;
uniform mat4 model;
uniform vec2 focal;   // for perspective unprojection
uniform vec2 pp;      // for perspective unprojection

// output camera parameters
uniform mat4 view;


uniform sampler2D mainColorTex;
uniform sampler2D mainDepthTex;
uniform sampler2D neighborColorTex;
uniform sampler2D neighborDepthTex;


void main()
{
	// Project neighbor to main camera
	// -------------------------------
	
	float depth = texture(neighborDepthTex, TexCoords).x;
	
	// unproject to find the worldPosition of the current pixel
	vec4 worldPosition;
	// perspective unprojection
	float x = (TexCoords.x * width - pp.x) / focal.x * depth;
	float y = (TexCoords.y * height - (pp.y + 2.0f * (height * 0.5f - pp.y))) / focal.y * depth;
	
	vec4 localPosition = vec4(x,y,depth,1.0f);
	worldPosition = model * localPosition;
	worldPosition = worldPosition / worldPosition.w;
	
	// project onto main camera
	vec4 viewPosition = view * worldPosition;
	viewPosition = viewPosition / viewPosition.w;
	if(viewPosition.z <= 0){
		discard;
	}
	
	float u = viewPosition.x / viewPosition.z * focal.x + pp.x;
	float v = viewPosition.y / viewPosition.z * focal.y + (pp.y + 2.0f * (height * 0.5f - pp.y));
	vec2 screenTexMain = vec2(u / width, v / height);
	
	// discard if outside of image bounds
	if(screenTexMain.x < 0 || screenTexMain.x > 1 || screenTexMain.y < 0 || screenTexMain.y > 1 ){
		discard;
	}
	
	// Set FragMask = 0 if the point from the neighbor camera does not match well 
	// with the corresponding point of the main camera.
	// --------------------------------------------------------------------------
	vec4 color_main = texture(mainColorTex, screenTexMain);
	vec4 color_neighbor = texture(neighborColorTex, TexCoords);
	float view_depth_main = texture(mainDepthTex, screenTexMain).x;
	float view_depth_neighbor = viewPosition.z;
	
	// depth test
	if (view_depth_neighbor > view_depth_main * 0.95f){
		discard;
	}
	
	// if color is pretty similar, discard
	float color_diff = length(color_neighbor - color_main);
	if(color_diff < 0.1f) {
		discard;
	}
	
	FragMask = 0;
}
