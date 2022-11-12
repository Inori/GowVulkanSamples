
layout(binding = 0) uniform UBOModel
{
	float alphaReference;
	float deltaAlphaEstimation;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 view;
	mat4 viewProj;
	mat4 invViewProj;
	vec2 viewport;
} viewData;

layout(binding = 2) uniform UBOInstance
{
	mat4 transform[2];
} instanceData;

