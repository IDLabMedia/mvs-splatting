#version 330 core
layout(location = 0) out float FragOut;

in vec2 TexCoords;

uniform float width;
uniform float height;
uniform vec2 layer_to_depth;

uniform sampler2D inputTex;

void main()
{
	FragOut = 1;
	
	float depth_c = texture(inputTex, TexCoords).r;
	// Iterate over nearby pixels and calculate the depth difference.
	// If the difference is too large, mask off pixel
	int radius = 5;
	
	int gap = 8;
	float layer = sqrt(depth_c / layer_to_depth.x - layer_to_depth.y);
	float min_depth = layer_to_depth.x * (layer - gap) * (layer - gap) + layer_to_depth.y;
	float max_depth = layer_to_depth.x * (layer + gap) * (layer + gap) + layer_to_depth.y;
	
	
	float sum = 0;
    for (int y = -radius; y <= radius && FragOut > 0.5f; y++) {
        for (int x = -radius; x <= radius; x++) {
			float dist = length(vec2(x,y));
			vec2 coordsNeighbor = TexCoords + vec2(x / width, y / height);
			
			float depth_n = texture(inputTex, coordsNeighbor).r;
			
			if(depth_n < min_depth || depth_n > max_depth) { // TODO
				FragOut = 0;
				break;
			}
        }
    }
	
	if(FragOut > 0.5f){
		discard;
	}
}