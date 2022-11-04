#version 450

#include "common_scene.h"

layout(early_fragment_tests) in;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPos;

layout (binding = 3) uniform sampler2D samplerMaterial;
layout (binding = 4) uniform sampler2D samplerNoise;

struct AppendJob 
{
	vec2 screenPos;
};

struct VkDispatchIndirectCommand
{
	uint x;
	uint y;
	uint z;
};

layout(binding = 5) buffer AppendBuffer
{
   AppendJob appendJobs[];
};

layout(binding = 6) buffer DispatchBuffer
{
   VkDispatchIndirectCommand dispatchCommand;
};

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
		uint indexMinusOne = atomicAdd(dispatchCommand.x, 1);
		uint index = indexMinusOne + 1;

		appendJobs[index].screenPos = vec2(gl_FragCoord.x, gl_FragCoord.y);

		discard;
	}
}