#version 450

#include "common_particle.h"

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() 
{
	gl_Position = viewData.viewProj * inPos;

	gl_PointSize = 2.0;
	outColor = inColor;
}