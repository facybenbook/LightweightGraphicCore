#include "Texture.hpp"

#include <unistd.h>

#include "Core/Application.hpp"

// STB has some unused parameters, os we ignore them
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define STB_IMAGE_IMPLEMENTATION
#include STB_INCLUDE_IMAGE
#pragma GCC diagnostic pop
#pragma clang diagnostic pop

using namespace LWGC;

Texture::Texture(void) : width(0), height(0), depth(1), arraySize(1), autoGenerateMips(false), usage(0),
	allocated(false), maxMipLevel(1), image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE),
	layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
	instance = VulkanInstance::Get();
	device = instance->GetDevice();
	graphicCommandBufferPool = instance->GetCommandBufferPool();
	Application::Get()->_textureTable.RegsiterObject(this);
}

Texture::Texture(Texture const & src)
{
	*this = src;
}

Texture::~Texture(void)
{
	if (allocated)
	{
		vkDestroyImage(device, image, nullptr);
		vkFreeMemory(device, memory, nullptr);
		vkDestroyImageView(device, view, nullptr);
	}
}

void		Texture::Destroy(void) noexcept
{
	Application::Get()->_textureTable.UnregisterObject(this);
	delete this;
}

Texture &	Texture::operator=(Texture const & src)
{
	if (this != &src)
	{
		this->width = src.width;
		this->height = src.height;
		this->autoGenerateMips = src.autoGenerateMips;
		this->view = src.view;
	}
	return (*this);
}

void			Texture::AllocateImage(VkImageViewType viewType)
{
	this->allocated = true;

	Vk::CreateImage(width, height, depth, arraySize, maxMipLevel, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
	view = Vk::CreateImageView(image, format, maxMipLevel, viewType, VK_IMAGE_ASPECT_COLOR_BIT);
}

// TODO: HDR and EXR support (stbi_us)
stbi_uc *		Texture::LoadFromFile(const std::string & fileName, int & width, int & height)
{
	// Check for file validity:
	if (access(fileName.c_str(), F_OK | R_OK) == -1)
	{
		throw std::runtime_error("Failed to load texture image, not a valid file: " + fileName);
	}

	int texChannels;
	stbi_uc *pixels = stbi_load(fileName.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

	if (!pixels)
		throw std::runtime_error("Failed to load texture image from file: " + fileName);

	return pixels;
}

void			Texture::UploadImage(stbi_uc * pixels, VkDeviceSize devizeSize, glm::ivec3 imageSize, glm::ivec3 offset)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Vk::CreateBuffer(devizeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	Vk::UploadToMemory(stagingBufferMemory, pixels, devizeSize);

	TransitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Vk::CopyBufferToImage(stagingBuffer, image, imageSize, offset);
	TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void			Texture::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = graphicCommandBufferPool->BeginSingle();
	TransitionImageLayout(commandBuffer, image, oldLayout, newLayout);
	graphicCommandBufferPool->EndSingle(commandBuffer);
}

void			Texture::TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = maxMipLevel;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = arraySize;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            cmd,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        	);

	layout = newLayout;
}

void			Texture::ChangeLayout(VkCommandBuffer cmd, VkImageLayout targetLayout)
{
	TransitionImageLayout(cmd, this->image, this->layout, targetLayout);
}

void			Texture::ChangeLayout(VkImageLayout targetLayout)
{
	TransitionImageLayout(this->image, this->layout, targetLayout);
}

void			Texture::UploadImageWithMips(VkImage image, VkFormat format, void * pixels, VkDeviceSize devizeSize, glm::ivec3 imageSize, glm::ivec3 offset)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Vk::CreateBuffer(devizeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	Vk::UploadToMemory(stagingBufferMemory, pixels, devizeSize);

	TransitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    Vk::CopyBufferToImage(stagingBuffer, image, imageSize, offset);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	GenerateMipMaps(image, format, width, height);
}

void			Texture::GenerateMipMaps(VkImage image, VkFormat format, int32_t width, int32_t height)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(VulkanInstance::Get()->GetPhysicalDevice(), format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = graphicCommandBufferPool->BeginSingle();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < (uint32_t)maxMipLevel; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
					   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &blit,
					   VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = maxMipLevel - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);

	graphicCommandBufferPool->EndSingle(commandBuffer);
}

int				Texture::GetWidth(void) const noexcept { return (this->width); }
int				Texture::GetHeight(void) const noexcept { return (this->height); }
int				Texture::GetDepth(void) const noexcept { return (this->depth); }
VkImageView		Texture::GetView(void) const noexcept { return this->view; }
VkImage			Texture::GetImage(void) const noexcept { return this->image; }
VkImageLayout	Texture::GetLayout(void) const noexcept { return this->layout; }
bool			Texture::GetAutoGenerateMips(void) const noexcept { return this->autoGenerateMips; }

std::ostream &	operator<<(std::ostream & o, Texture const & r)
{
	o << "Texture" << std::endl;
	(void)r;
	return (o);
}
