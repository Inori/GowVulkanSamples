#version 450

#include "common_scene.h"

layout (location = 1) in vec2 inUV;

layout (binding = 3) uniform sampler2D particleSpawn;

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
}