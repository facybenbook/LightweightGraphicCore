#pragma once

#include <iostream>
#include <string>

#include "Core/Object.hpp"
#include "Core/Mesh.hpp"
#include "Core/Vulkan/Material.hpp"
#include "Core/Vulkan/UniformBuffer.hpp"
#include "Component.hpp"
#include "Core/Vulkan/DescriptorSet.hpp"

namespace LWGC
{
	class		Renderer : public Object, public Component
	{
		private:
			struct LWGC_PerObject
			{
				glm::mat4	model;
			};

			LWGC_PerObject		_perObject;
			ComponentIndex		_renderContextIndex;
			UniformBuffer		_uniformModelBuffer;
			DescriptorSet		_perRendererSet;

		protected:

			void			Initialize(void) noexcept override;
			virtual void	RecordDrawCommand(VkCommandBuffer cmd) noexcept = 0;
			virtual void	UpdateUniformData(void);
			void			Update(void) noexcept override;

		public:
			Material *	_material;
			Renderer(void);
			Renderer(Material * material);
			Renderer(const Renderer &) = delete;
			virtual ~Renderer(void);

			Renderer &	operator=(Renderer const & src) = delete;

			virtual Bounds	GetBounds(void) noexcept;
			virtual void	RecordCommands(VkCommandBuffer cmd);

			void	OnEnable(void) noexcept override;
			void	OnDisable(void) noexcept override;

			Material *	GetMaterial(void);
			void	SetMaterial(Material * tmp);

			VkDescriptorSet		GetDescriptorSet(void);

			virtual uint32_t	GetType(void) const noexcept override = 0;
	};

	std::ostream &	operator<<(std::ostream & o, Renderer const & r);
}
