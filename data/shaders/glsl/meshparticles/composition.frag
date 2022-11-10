#version 450

layout (binding = 0) uniform sampler2D samplerSceneAlbedo;
layout (binding = 1) uniform sampler2D samplerParticleAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 sceneAlbedo = texture(samplerSceneAlbedo, inUV);
	vec4 particleAlbedo = texture(samplerParticleAlbedo, inUV);

	/*
	if (particleAlbedo.a > 0.0)
	{
		// outFragColor = sceneAlbedo;
		outFragColor = particleAlbedo;
	}
	else
	{
		outFragColor = sceneAlbedo;
	}
	*/

	if (sceneAlbedo.a > 0.0)
	{
		outFragColor = sceneAlbedo;
	}
	else
	{
		outFragColor = vec4(particleAlbedo.rgb * particleAlbedo.a, 1.0);
	}
}