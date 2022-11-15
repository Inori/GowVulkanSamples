
// Work group size of particle.comp
#define PARTICLE_COMPUTE_WORKGROUP_SIZE 64

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
	uint particleCount;                      // particle count emitted this frame
	VkDispatchIndirectCommand dispatchCmd;
	VkDrawIndirectCommand drawCmd;
};

struct GlobalParticleData
{
	uint particleCountMax;		// ring buffer size, never change
	uint particleIndex;			// ring buffer index
	uint renderCount;			// particle render count this frame, equals compute shader thread count
	uint cachedCount;			// slot count in particle ring buffer before current particleIndex
	uint newEmiitedCount;		// newly emitted particle count this frame
};

