#version 450

#include "common_particle.h"

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = inColor;
}

