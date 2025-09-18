

#pragma once
#include <cstdint>

namespace Framework
{
	enum class ComponentTypeId : std::uint16_t //save memory always 2 bytes, it also cannot hold negative values
	{
		//Invalid component id
		CT_None = 0,
		CT_Transform,
		CT_Render,
		CT_TestComponent,
		//more
		CT_MaxComponents
	};
}