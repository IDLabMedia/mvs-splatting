#version 330 core
layout(location = 0) out vec3 FragOut;

in vec2 TexCoords;

uniform int nrTextures; 
uniform int offset; 
uniform sampler2D errorTex[32]; 

// Outputs a vec3, which is actually:
//  - float (actually int) : the best layer
//	- float                : the lowest error
//  - float (actually int) : nr layers close to lowest error
void main()
{
	float lowest_error = 9999;
	int best_layer = 0;
	float errors[32];
    for (int i = 0; i < nrTextures; i++) {
        errors[i] = texture(errorTex[i], TexCoords).r;
		if (errors[i] < lowest_error){
			lowest_error = errors[i];
			best_layer = i + offset;
		}
    }
	
	int nr_low_error_layers = 0;
	float thresh = lowest_error + 0.01f;
	for (int i = 0; i < nrTextures; i++) {
		if (errors[i] < thresh){
			nr_low_error_layers++;
		}
    }
	
	FragOut = vec3(float(best_layer), lowest_error, nr_low_error_layers);
}