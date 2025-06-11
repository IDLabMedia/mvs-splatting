#version 330 core
layout(points) in;
layout(points, max_vertices = 1) out;

in vec3 vs_position[];
in vec3 vs_color[];
in float vs_scale[];
in int vs_mask[];

out vec3 out_position;
out vec3 out_color;
out float out_scale;

void main() {
	if(vs_mask[0] > 0.5f) {
        out_position = vs_position[0];
		out_color = vs_color[0];
		out_scale = vs_scale[0];
		
        EmitVertex();
        EndPrimitive();
    }
}
