
layout(binding = 0) uniform UBOModel
{
	float modelAlpha;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 view;
	mat4 viewProj;
	mat4 invViewProj;
	vec2 viewport;
} viewData;

struct ParticleSystem
{
	vec3 wind;
	float deltaT;
	float speed;
	float random;
	uint frameNum;
};

layout(std140, binding = 2) uniform ParticleSystemBuffer
{
	ParticleSystem particleSystem;
};


