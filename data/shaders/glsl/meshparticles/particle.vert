#version 450

#include "common_particle.h"

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() 
{
	// gl_Position = viewData.projection * viewData.view * viewData.model * inPos;
	float x = (inPos.x/1920.0) * 2.0 - 1.0;
	float y = (inPos.y/1080.0) * 2.0 - 1.0;
	gl_Position = vec4(x, y, 0.0, 1.0);

	gl_PointSize = 2.0;
	outColor = inColor;
}


