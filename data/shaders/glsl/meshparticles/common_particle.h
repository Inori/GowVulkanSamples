
layout(binding = 0) uniform UBOModel
{
	float modelAlpha;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 projection;
	mat4 modelView;
	vec2 viewport;
};

struct ParticleSystem
{
	float deltaT;
	float speed;
	float random;
};

layout(std140, binding = 2) uniform ParticleSystemBuffer
{
	ParticleSystem particleSystem;
};


