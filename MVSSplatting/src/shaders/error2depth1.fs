#version 330 core
layout(location = 0) out float FragDepth;
layout(location = 1) out float FragMask;

in vec2 TexCoords;

uniform vec2 layer_to_depth;
uniform int nrTextures; 
uniform sampler2D inputTex[10]; 

void main()
{
	float lowest_error = 9999;
	int best_layer = 0;
	float errors[10];
	float nr_low_error_layers[10];
    for (int i = 0; i < nrTextures; i++) {
		// each pixel in inputTex[i] contains:
		//  - float (actually int) : the best layer
		//	- float                : the lowest error
		//  - float (actually int) : nr layers close to lowest error
		vec3 tmp = texture(inputTex[i], TexCoords).xyz;
        errors[i] = tmp.y;
		nr_low_error_layers[i] = tmp.z;
		if (errors[i] < lowest_error){
			lowest_error = errors[i];
			best_layer = int(tmp.x);
		}
    }
	
	// approximately estimate nr_low_error_layers
	float nr_low_error_layers_total = 0;
	float thresh_inv = 100; // = 1/0.01f
	for (int i = 0; i < nrTextures; i++) {
		// linearly interpolate like this:
		// if errors[i] == lowest_error,          then nr_low_error_layers_total += 1 * nr_low_error_layers[i]
		// if errors[i] == lowest_error + thresh, then nr_low_error_layers_total += 0 * nr_low_error_layers[i]
		nr_low_error_layers_total +=  max(0, 1.0f - (errors[i] - lowest_error)  * thresh_inv) * nr_low_error_layers[i];
	}
	
	// layer to depth
	FragDepth = layer_to_depth.x * (best_layer * best_layer) + layer_to_depth.y; 
	FragMask = (nr_low_error_layers_total > 20 || lowest_error > 0.1f)? 0 : 1;
}