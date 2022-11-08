
layout(binding = 0) uniform UBOModel
{
	float modelAlpha;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 projection;
	mat4 modelView;
} viewData;

struct ParticleSystem
{
	float deltaT;
};

layout(std140, binding = 2) uniform ParticleSystemBuffer
{
	ParticleSystem particleSystem;
} instanceData;

