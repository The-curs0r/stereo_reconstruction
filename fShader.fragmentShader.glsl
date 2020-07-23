#version 450 core

out vec4 color;

uniform sampler2D img_texture;

uniform float minDepth;
uniform float maxDepth;

in VS_OUT{
	vec3 vertexCoord;
	vec2 uv;
}fs_in;

void main(void) {
	color = vec4((fs_in.vertexCoord[2] - minDepth) * 0.5 / (maxDepth - minDepth) + 0.5, (fs_in.vertexCoord[2] - minDepth) * 0.5 / (maxDepth - minDepth) + 0.5, (fs_in.vertexCoord[2] - minDepth) * 0.5 / (maxDepth - minDepth) + 0.5, 1.0);
	color *= texture(img_texture, fs_in.uv);
}

