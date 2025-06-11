#version 330 core
layout(location = 0) out float FragError;

in vec2 TexCoords;

uniform float width;
uniform float height;
uniform int radius;
uniform int step;
uniform int nrTextures;

uniform sampler2D errorTex[4]; 
uniform sampler2D colorTex; 

void main()
{
	vec3 color_c = texture(colorTex, TexCoords).rgb;
	
	float sums[4] = float[4](0, 0, 0, 0);
	float counts[4] = float[4](0, 0, 0, 0);
	
    for (int y = -radius; y <= radius; y+=step) {
        for (int x = -radius; x <= radius; x+=step) {
			float dist = length(vec2(x,y));
			if(dist <= radius){
				vec2 coordsNeighbor = TexCoords + vec2(x / width, y / height);
				
				// take color difference into account
				vec3 color_n = texture(colorTex, coordsNeighbor).rgb;
				float color_diff = length(color_c - color_n);
				float weight = 1 / (5 * color_diff + 1); // small color differences have a larger weight then large color differences
				
				for(int i = 0; i < nrTextures; i++){	
					float error = texture(errorTex[i], coordsNeighbor).r;
					sums[i] += error * weight;
					counts[i] += weight;
				}
			}
        }
    }
	
	// takes minimum
	float lowest_error = 9999;
	for (int i = 0; i < nrTextures; i++) {
        float error = sums[i] / counts[i];
		if(error <= lowest_error){
			lowest_error = error;
		}
    }
	
    FragError = lowest_error;
}