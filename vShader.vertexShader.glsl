#version 450 core

in vec3 vertex;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

out VS_OUT{
	vec3 vertexCoord;
}vs_out;

void main(void) {
	vs_out.vertexCoord = vertex;
	gl_Position = proj_matrix*mv_matrix*vec4(vertex, 1.0);
}