
layout(binding = 0) uniform UBOModel
{
	float alphaReference;
} modelData;

layout(binding = 1) uniform UBOView
{
	mat4 projection;
	mat4 model;
	mat4 view;
} viewData;

layout(binding = 2) buffer SRVInstance
{
	vec4 instancePos[];
} instanceData;

