#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	vec4 instancePos[2];
	float modelAlpha;
} modelData;

layout (binding = 2) uniform sampler2D samplerMaterial;
layout (binding = 3) uniform sampler2D samplerNoise;

layout (location = 0) out vec4 outColor;


void main() 
{
	float gray = texture(samplerMaterial, inUV).r;
	//float alphaNoise = texture(samplerNoise, inUV).r;
	float alphaNoise = 1 - gray;

	if (modelData.modelAlpha > alphaNoise)
	{
		outColor = vec4(gray, gray, gray, 1.0) * vec4(inColor, 1.0);
	}
	else
	{
		outColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}