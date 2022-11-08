
layout(binding = 0) uniform UBOModel
{
	float alphaReference;
	float deltaAlphaEstimation;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 projection;
	mat4 modelView;
} viewData;

layout(binding = 2) buffer SRVInstance
{
	vec4 instancePos[];
} instanceData;

