#version 450 core

out vec4 color;

in VS_OUT{
	vec3 vertexCoord;
}fs_in;

void main(void) {
	color = vec4((fs_in.vertexCoord[2] + 50.0) / 100.0, (fs_in.vertexCoord[2] + 50.0) / 100.0, (fs_in.vertexCoord[2]+50.0)/100.0, 1.0);
}

