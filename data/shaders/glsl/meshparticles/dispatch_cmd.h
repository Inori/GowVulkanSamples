
struct VkDispatchIndirectCommand
{
	uint x;
	uint y;
	uint z;
};

struct DispatchBuffer
{
	VkDispatchIndirectCommand cmd;
	uint particleCount;
};

