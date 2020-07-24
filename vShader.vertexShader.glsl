#version 450 core

in vec3 vertex;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform float maxX;
uniform float maxY;

uniform float minDepth;
uniform float maxDepth;

out VS_OUT{
	vec3 vertexCoord;
	vec2 uv;
}vs_out;

void main(void) {
	vs_out.vertexCoord = vertex;
	vs_out.vertexCoord[2] = maxDepth - minDepth - vertex[2];
	vs_out.uv = vec2(vertex[0] / maxX,1- vertex[1] / maxY);
	gl_Position = proj_matrix*mv_matrix*vec4(vs_out.vertexCoord, 1.0);
}