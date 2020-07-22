#version 450 core

out vec4 color;

uniform float minDepth;
uniform float maxDepth;

in VS_OUT{
	vec3 vertexCoord;
}fs_in;

void main(void) {
	color = vec4((fs_in.vertexCoord[2]-minDepth)*0.5/(maxDepth-minDepth)+0.5, (fs_in.vertexCoord[2] - minDepth) * 0.5 / (maxDepth - minDepth) + 0.5, (fs_in.vertexCoord[2] - minDepth) * 0.5 / (maxDepth - minDepth) + 0.5,1.0);
}

