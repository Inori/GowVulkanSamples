
struct VkDispatchIndirectCommand
{
	uint x;
	uint y;
	uint z;
};

struct VkDrawIndirectCommand 
{
	uint    vertexCount;
	uint    instanceCount;
	uint    firstVertex;
	uint    firstInstance;
};

struct GpuCmdBuffer
{
	uint particleCount;
	VkDispatchIndirectCommand dispatchCmd;
	VkDrawIndirectCommand drawCmd;
};

