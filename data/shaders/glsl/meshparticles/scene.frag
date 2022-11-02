#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	float modelAlpha;
} modelData;

layout (binding = 2) uniform sampler2D samplerMaterial;
layout (binding = 3) uniform sampler2D samplerNoise;

layout (location = 0) out vec4 outColor;


void main() 
{
	outColor = texture(samplerMaterial, inUV) * vec4(inColor, 1.0);
}