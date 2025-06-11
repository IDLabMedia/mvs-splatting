#version 330 core
layout (location = 0) in vec2 TexCoords; // input from the VBO, i.e. (u,v) texture coordinates

// variables to pass to geometry shader
//out vs_out
//{
//    vec2 TexCoord;
//	float inputDepth;
//}vertex;
out fs_in
{
    vec2 TexCoord;
	float viewDepth;
}vertex;

// input camera parameters
uniform float width;
uniform float height;
uniform mat4 model;
uniform vec2 in_f;  // focal length
uniform vec2 in_pp; // principal point

// output camera parameters
uniform mat4 view;
uniform float out_width;
uniform float out_height;
uniform vec2 out_f;
uniform vec2 out_pp;
uniform vec2 out_near_far;

uniform sampler2D depthTex;

void main()
{
	vertex.TexCoord = TexCoords;
	
	float depth = texture(depthTex, TexCoords).x;
	//vertex.inputDepth = depth;
	
	// unproject to find the worldPosition of the current pixel
	vec4 worldPosition;
	// perspective unprojection
	if(depth > 0){
		float x = (TexCoords.x * width - in_pp.x) / in_f.x * depth;
		float y = (TexCoords.y * height - (in_pp.y + 2.0f * (height * 0.5f - in_pp.y))) / in_f.y * depth;
	
		vec4 localPosition = vec4(x,y,depth,1.0f);
		worldPosition = model * localPosition;
	}
	else {
		// discard this vertex by placing it behind the near plane (it will be culled)
		worldPosition = vec4(0,0,-10,1);
	}
	worldPosition = worldPosition / worldPosition.w;
	
	// project onto the output image
	vec4 viewPosition = view * worldPosition;
	viewPosition = viewPosition / viewPosition.w;
	vertex.viewDepth = viewPosition.z;
	
	if(viewPosition.z > 0){
		float u = viewPosition.x / viewPosition.z * out_f.x + out_pp.x;
		float v = viewPosition.y / viewPosition.z * out_f.y + (out_pp.y + 2.0f * (out_height * 0.5f - out_pp.y));
		float normalised_depth = (viewPosition.z - out_near_far.x) / (out_near_far.y - out_near_far.x);
		gl_Position = vec4(2.0f * u / out_width - 1.0f, 2.0f * v / out_height - 1.0f, normalised_depth, 1.0f);
		// Now, invert the Y axis to convert from COLMAP to OpenGL NDC space
		gl_Position.y = -gl_Position.y;
	}
	else {
		// discard this vertex by placing it behind the near plane (it will be culled)
		gl_Position = vec4(0,0,-10,1);
	}
}