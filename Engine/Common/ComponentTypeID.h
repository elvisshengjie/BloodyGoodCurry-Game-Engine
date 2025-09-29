

#pragma once
#include <cstdint>

namespace Framework
{
	enum class ComponentTypeId : std::uint16_t //save memory always 2 bytes, it also cannot hold negative values
	{
		//Invalid component id
		CT_None = 0,
		CT_TransformComponent,
		CT_RenderComponent,
		CT_CircleRenderComponent,
	
		//more
		CT_MaxComponents,
		// For Input
		CT_InputComponents,
		// For rigid body
		CT_RigidBodyComponents
	};
}