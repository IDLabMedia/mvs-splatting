#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 color;

// output camera parameters
uniform mat4 view;
uniform float width;
uniform float height;
uniform vec2 focal;
uniform vec2 pp;
uniform vec2 out_near_far;

void main()
{
	color = aColor;
	vec4 worldPosition = vec4(aPos, 1);
	
	// project onto the output image
	vec4 viewPosition = view * worldPosition;
	viewPosition = viewPosition / viewPosition.w;
	if(viewPosition.z > 0){
		float u = viewPosition.x / viewPosition.z * focal.x + pp.x;
		float v = viewPosition.y / viewPosition.z * focal.y + (pp.y + 2.0f * (height * 0.5f - pp.y)); // TODO
		//float v = viewPosition.y / viewPosition.z * focal.y + pp.y;
		float normalised_depth = (viewPosition.z - out_near_far.x) / (out_near_far.y - out_near_far.x);
		gl_Position = vec4(2.0f * u / width - 1.0f, 2.0f * v / height - 1.0f, normalised_depth, 1.0f);
		// Now, invert the Y axis to convert from COLMAP to OpenGL NDC space
		gl_Position.y = -gl_Position.y;
	}
	else {
		gl_Position = vec4(0,0,-10,1);
	}
}