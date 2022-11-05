/*
* Vulkan Example - Disintegrating Meshes with Particles in 'God of War'
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION true


class VulkanExample : public VulkanExampleBase
{
public:

	vkglTF::Model sphere;
	vks::Texture2D particlespawn;

	constexpr static uint32_t particleCount = 128 * 1024;
	constexpr static uint32_t instanceCount = 2;

	struct UBOModelData {
		float alphaReference = 0.0f;
	} uboModelData;

	struct UBOViewlData {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uboViewData;

	struct SRVInstanceData {
		glm::vec4 instancePos[instanceCount];
	} srvInstanceData;

	// Append buffer unit
	struct AppendJob {
		glm::vec2 screenPos;
	};

	// SSBO particle declaration
	struct Particle {
		glm::vec4 pos;
		glm::vec4 color;
	};

	struct ParticleSystem {					// Compute shader uniform block object
		float deltaT;						// Frame delta time
	} particleSystem;

	struct DispatchBuffer {
		VkDispatchIndirectCommand cmd;
		uint32_t particleCount;
	};

	struct {
		vks::Buffer modelData;
		vks::Buffer viewData;
		vks::Buffer particleSystem;
	} uniformBuffers;

	struct {
		vks::Buffer instancing;
		// VkDispatchIndirectCommand
		vks::Buffer dispatch;
		// AppendJob
		vks::Buffer append;
		// Particle
		vks::Buffer particle;
	} resourceBuffers;

	struct {
		VkPipeline depthOnly;
		VkPipeline scene;
		VkPipeline compute;
		VkPipeline gpuCmd;
		VkPipeline particle;
	} pipelines;

	struct {
		VkPipelineLayout scene;
		VkPipelineLayout compute;
		VkPipelineLayout gpuCmd;
		VkPipelineLayout paraticle;
	} pipelineLayouts;

	struct {
		const uint32_t count = 64;
		VkDescriptorSet scene;
		VkDescriptorSet compute;
		VkDescriptorSet gpuCmd;
		VkDescriptorSet particle;
	} descriptorSets;

	struct {
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout compute;
		VkDescriptorSetLayout gpuCmd;
		VkDescriptorSetLayout particle;
	} descriptorSetLayouts;


	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
		void destroy(VkDevice device)
		{
			vkDestroyImage(device, image, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};

	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		void setSize(int32_t w, int32_t h)
		{
			this->width = w;
			this->height = h;
		}
		void destroy(VkDevice device)
		{
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);
		}
	};

	struct {
		struct DepthOnly : public FrameBuffer {
			FrameBufferAttachment depth;
		} depthOnly;
	} offscreenFrameBuffers;

	// One sampler for the frame buffer color attachments
	VkSampler colorSampler;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Disintegrating Meshes with Particles";
		camera.type = Camera::CameraType::lookat;
		camera.position = { 0.0f, 0.0f, -2.5f };
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		particlespawn.destroy();

		resourceBuffers.instancing.destroy();
		resourceBuffers.dispatch.destroy();
		resourceBuffers.append.destroy();
		resourceBuffers.particle.destroy();

		vkDestroySampler(device, colorSampler, nullptr);

		vkDestroyPipeline(device, pipelines.scene, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.scene, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
	}

	void getEnabledFeatures()
	{
		enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
		enabledFeatures.fragmentStoresAndAtomics = deviceFeatures.fragmentStoresAndAtomics;
	}

	// Create a frame buffer attachment
	void createAttachment(
		VkFormat format,
		VkImageUsageFlagBits usage,
		FrameBufferAttachment* attachment,
		uint32_t width,
		uint32_t height)
	{
		VkImageAspectFlags aspectMask = 0;

		attachment->format = format;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

		VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspectMask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = attachment->image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
	}

	void prepareOffscreenFramebuffers()
	{
		// Attachments
		offscreenFrameBuffers.depthOnly.setSize(width, height);

		// Use the default depth buffer created in VulkanExampleBase::setupDepthStencil
		offscreenFrameBuffers.depthOnly.depth.image = depthStencil.image;
		offscreenFrameBuffers.depthOnly.depth.view = depthStencil.view;
		offscreenFrameBuffers.depthOnly.depth.format = depthFormat;
		offscreenFrameBuffers.depthOnly.depth.mem = depthStencil.mem;;

		// Depth only
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = offscreenFrameBuffers.depthOnly.depth.format;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pDepthStencilAttachment = &depthReference;

			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = &attachmentDescription;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenFrameBuffers.depthOnly.renderPass));

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = offscreenFrameBuffers.depthOnly.renderPass;
			fbufCreateInfo.pAttachments = &offscreenFrameBuffers.depthOnly.depth.view;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.width = offscreenFrameBuffers.depthOnly.width;
			fbufCreateInfo.height = offscreenFrameBuffers.depthOnly.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenFrameBuffers.depthOnly.frameBuffer));
		}

		// Shared sampler used for all color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &colorSampler));
	}

	void setupRenderPass()
	{
		std::array<VkAttachmentDescription, 2> attachments = {};
		// Color attachment
		attachments[0].format = swapChain.colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// Depth attachment
		// Note that we use previously generated depth buffer, so we need to keep its' content
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;

		// Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}

	void loadAssets()
	{
		vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor;
		const uint32_t gltfLoadingFlags = vkglTF::FileLoadingFlags::FlipY | vkglTF::FileLoadingFlags::PreTransformVertices;

		sphere.loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue, gltfLoadingFlags);
		particlespawn.loadFromFile(getAssetPath() + "textures/particlespawn.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkDeviceSize offsets[1] = { 0 };

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));


			/*
				First pass: Depth only
			*/
			{
				std::vector<VkClearValue> clearValues(1);
				clearValues[0].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = offscreenFrameBuffers.depthOnly.renderPass;
				renderPassBeginInfo.framebuffer = offscreenFrameBuffers.depthOnly.frameBuffer;
				renderPassBeginInfo.renderArea.extent.width = offscreenFrameBuffers.depthOnly.width;
				renderPassBeginInfo.renderArea.extent.height = offscreenFrameBuffers.depthOnly.height;
				renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
				renderPassBeginInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = vks::initializers::viewport((float)offscreenFrameBuffers.depthOnly.width, (float)offscreenFrameBuffers.depthOnly.height, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(offscreenFrameBuffers.depthOnly.width, offscreenFrameBuffers.depthOnly.height, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.depthOnly);

				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
				sphere.draw(drawCmdBuffers[i], instanceCount, 0, pipelineLayouts.scene);

				vkCmdEndRenderPass(drawCmdBuffers[i]);
			}


			/*
				Second pass: Scene rendering
			*/
			{
				std::vector<VkClearValue> clearValues(2);
				clearValues[0].color = defaultClearColor;
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = VulkanExampleBase::frameBuffers[i];
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
				renderPassBeginInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);

				sphere.draw(drawCmdBuffers[i], instanceCount, 0, pipelineLayouts.scene);

				drawUI(drawCmdBuffers[i]);

				vkCmdEndRenderPass(drawCmdBuffers[i]);
			}



			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, descriptorSets.count);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		// Depth and scene pass
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				// Binding 0 : Shader model data uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
				// Binding 1 : Shader view data uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
				// Binding 2 : Instance data
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2),
				// Binding 3 : material texture
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
				// Binding 4 : Append buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
				// Binding 5 : Dispatch buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5)
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.scene));

			// Shared pipeline layout used by all pipelines
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.scene, 1);
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.scene));
		}

		// Particle pass
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				// Binding 0 : Shader model data uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
				// Binding 1 : Shader view data uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
				// Binding 2 : Particle system uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2)
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayout =
				vks::initializers::descriptorSetLayoutCreateInfo(
					setLayoutBindings.data(),
					static_cast<uint32_t>(setLayoutBindings.size()));

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.particle));

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.particle, 1);
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.paraticle));
		}

		// Dispatch command calculate pass
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				// Binding 0 : GPU dispatch command
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0)
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayout =
				vks::initializers::descriptorSetLayoutCreateInfo(
					setLayoutBindings.data(),
					static_cast<uint32_t>(setLayoutBindings.size()));

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.gpuCmd));

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.gpuCmd, 1);
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.gpuCmd));
		}

		// Compute pass
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				// Binding 0 : Particle system uniform buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
				// Binding 1 : Append buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
				// Binding 2 : Particle buffer
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayout =
				vks::initializers::descriptorSetLayoutCreateInfo(
					setLayoutBindings.data(),
					static_cast<uint32_t>(setLayoutBindings.size()));

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.compute));

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.compute, 1);
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.compute));
		}
	}

	void setupDescriptorSet()
	{

		// Depth and scene pass
		{
			std::vector<VkWriteDescriptorSet> writeDescriptorSets;
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.scene, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));

			writeDescriptorSets = 
			{
				// Binding 0: Shader model data uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.modelData.descriptor),
				// Binding 1: Shader view data uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &uniformBuffers.viewData.descriptor),
				// Binding 2: Shader instance buffer
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &resourceBuffers.instancing.descriptor),
				// Binding 3 : Material texture
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &particlespawn.descriptor),
				// Binding 4 : Append buffer
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &resourceBuffers.append.descriptor),
				// Binding 5 : Dispatch buffer
				vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, &resourceBuffers.append.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		// Particle pass
		{
			std::vector<VkWriteDescriptorSet> writeDescriptorSets;
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.particle, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.particle));

			writeDescriptorSets =
			{
				// Binding 0: Shader model data uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets.particle, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.modelData.descriptor),
				// Binding 1: Shader view data uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets.particle, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &uniformBuffers.viewData.descriptor),
				// Binding 2: Particle system
				vks::initializers::writeDescriptorSet(descriptorSets.particle, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &uniformBuffers.particleSystem.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		// Dispatch command calculate pass
		{
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.gpuCmd, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.gpuCmd));

			std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
			{
				// Binding 0 : GPU dispatch command
				vks::initializers::writeDescriptorSet(descriptorSets.gpuCmd, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &resourceBuffers.dispatch.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);
		}

		// Compute pass
		{
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.compute,1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.compute));

			std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
			{
				// Binding 0 : Particle system buffer
				vks::initializers::writeDescriptorSet(descriptorSets.compute, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.particleSystem.descriptor),
				// Binding 1 : Append buffer
				vks::initializers::writeDescriptorSet(descriptorSets.compute, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &resourceBuffers.append.descriptor),
				// Binding 1 : Particle buffer
				vks::initializers::writeDescriptorSet(descriptorSets.compute, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &resourceBuffers.particle.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);
		}
	}

	void prepareGraphicsPipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		// The depth buffer is already prepared in previous depth-only pass,
		// so we don't write the depth buffer in final pass,
		// and only fire fragment shader on equal depth value.
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;

		// Enable alpha blend
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayouts.scene, renderPass, 0);
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Vertex input state from glTF model loader
		pipelineCreateInfo.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal });
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.layout = pipelineLayouts.scene;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		// Final scene pipeline
		shaderStages[0] = loadShader(getShadersPath() + "meshparticles/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "meshparticles/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.scene));

		// Depth only pipeline
		{
			pipelineCreateInfo.renderPass = offscreenFrameBuffers.depthOnly.renderPass;
			pipelineCreateInfo.layout = pipelineLayouts.scene;

			// We don't need color attachments
			colorBlendState.attachmentCount = 0;
			colorBlendState.pAttachments = nullptr;

			// Enable depth test and detph write
			VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
			pipelineCreateInfo.pDepthStencilState = &depthStencilState;

			shaderStages[1] = loadShader(getShadersPath() + "meshparticles/depth.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.depthOnly));
		}
	}

	void prepareResourceBuffers()
	{
		// Instance buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&resourceBuffers.instancing,
			sizeof(srvInstanceData)));

		// Setup instanced model positions
		srvInstanceData.instancePos[0] = glm::vec4(0.0f);
		srvInstanceData.instancePos[1] = glm::vec4(-3.0f, 0.0, -4.0f, 0.0f);

		vks::Buffer stagingBuffer;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			sizeof(srvInstanceData),
			&srvInstanceData));

		vulkanDevice->copyBuffer(&stagingBuffer, &resourceBuffers.instancing, queue);

		stagingBuffer.destroy();

		// Dispatch buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&resourceBuffers.dispatch,
			sizeof(DispatchBuffer)));

		// Append buffer
		VkDeviceSize appendBufferSize = width * height * sizeof(AppendJob);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&resourceBuffers.append,
			appendBufferSize));

		// Particle buffer
		VkDeviceSize particleBufferSize = particleCount * sizeof(Particle);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&resourceBuffers.particle,
			particleBufferSize));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Model data
		vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.modelData,
			sizeof(uboModelData));

		// View data
		vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.viewData,
			sizeof(uboViewData));

		// Particle system
		vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.particleSystem,
			sizeof(particleSystem));

		// Update
		updateUniformBufferModel();
		updateUniformBufferView();
	}

	void prepareComputePipelines()
	{
		// Create pipelines

		{
			VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(pipelineLayouts.compute, 0);
			computePipelineCreateInfo.stage = loadShader(getShadersPath() + "meshparticles/particle.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
			VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipelines.compute));
		}

		{
			VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(pipelineLayouts.gpuCmd, 0);
			computePipelineCreateInfo.stage = loadShader(getShadersPath() + "meshparticles/gpu_cmd.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
			VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipelines.gpuCmd));
		}
	}

	void updateUniformBufferModel()
	{
		uboModelData.alphaReference = timer;

		VK_CHECK_RESULT(uniformBuffers.modelData.map());
		uniformBuffers.modelData.copyTo(&uboModelData, sizeof(uboModelData));
		uniformBuffers.modelData.unmap();
	}

	void updateUniformBufferView()
	{
		uboViewData.projection = camera.matrices.perspective;
		uboViewData.view = camera.matrices.view;
		uboViewData.model = glm::mat4(1.0f);

		VK_CHECK_RESULT(uniformBuffers.viewData.map());
		uniformBuffers.viewData.copyTo(&uboViewData, sizeof(uboViewData));
		uniformBuffers.viewData.unmap();
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareOffscreenFramebuffers();
		prepareUniformBuffers();
		prepareResourceBuffers();
		setupDescriptorPool();
		setupDescriptorSetLayout();
		setupDescriptorSet();
		prepareGraphicsPipelines();
		prepareComputePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared) {
			return;
		}

		draw();

		updateUniformBufferModel();
		if (camera.updated) {
			updateUniformBufferView();
		}
	}

	virtual void viewChanged()
	{
		// updateUniformBufferModel();
		updateUniformBufferView();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->sliderFloat("Alpha Reference", &uboModelData.alphaReference, 0.0f, 1.0f)) {
				updateUniformBufferModel();
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()