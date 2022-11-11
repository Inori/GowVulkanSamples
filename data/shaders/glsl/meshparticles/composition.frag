#version 450

layout (binding = 0) uniform sampler2D samplerSceneAlbedo;
layout (binding = 1) uniform sampler2D samplerParticleAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 sceneAlbedo = texture(samplerSceneAlbedo, inUV);
	vec4 particleAlbedo = texture(samplerParticleAlbedo, inUV);

	if (particleAlbedo.a > 0)
	{
		//outFragColor = vec4(particleAlbedo.r, particleAlbedo.g, particleAlbedo.b, 1.0);
		outFragColor = vec4(0.0, particleAlbedo.g, 0.7, 1.0);
	}
	else
	{
		outFragColor = sceneAlbedo;
	}
}