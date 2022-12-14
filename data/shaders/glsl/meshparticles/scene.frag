#version 450
#extension GL_EXT_debug_printf : enable

#include "common_scene.h"
#include "gpu_cmd.h"

layout(early_fragment_tests) in;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPos;

layout (binding = 3) uniform sampler2D particleSpawn;

struct AppendJob 
{
	vec2 screenPos;
};

layout(binding = 4) buffer AppendBuffer
{
   AppendJob appendJobs[];
};

layout(binding = 5) buffer SSBOGpuCmdBuffer
{
   GpuCmdBuffer gpuCmdBuffer;
};

layout (location = 0) out vec4 outColor;


void main() 
{
	float gray = texture(particleSpawn, inUV).r;
	float modelAlpha = gray;

	// As alpha reference value increase,
	// model alpha which is less than alpha reference will be invisable.
	if (modelAlpha < modelData.alphaReference)
	{
		discard;
	}

	uint flag = floatBitsToUint(inUV.x) & 1;

	// Determine if current pixel will be invisable next frame.
	float nextAlphaReference = modelData.alphaReference + modelData.deltaAlphaEstimation;
	if (modelAlpha < nextAlphaReference && flag != 0)
	{
		// If it's going to be invisable, append information in the append buffer,
		// such that it can be replaced by particle next frame.
		uint index = atomicAdd(gpuCmdBuffer.particleCount, 1);
		appendJobs[index].screenPos = vec2(gl_FragCoord.x, gl_FragCoord.y);

		//debugPrintfEXT("index %d\n", index);
	}

	outColor = vec4(gray, gray, gray, 1.0) * vec4(inColor, 1.0);
}