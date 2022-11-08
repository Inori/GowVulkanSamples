#version 450

#include "common_scene.h"

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outPos;

void main() 
{
	vec4 instancePos = inPos + instanceData.instancePos[gl_InstanceIndex];
	gl_Position = viewData.projection * viewData.modelView * instancePos;
	
	outUV = inUV;

	// Vertex position in view space
	outPos = vec3(viewData.modelView * inPos);

	// Normal in view space
	mat3 normalMatrix = transpose(inverse(mat3(viewData.modelView)));
	outNormal = normalMatrix * inNormal;

	outColor = inColor;
}
