#pragma once
#include "Composition/Component.h"
#include "Common/ComponentTypeID.h"
#include "Serialization/Serialization.h"

namespace Framework
{
	struct Vec2
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	class RigidBodyComponent : public GameComponent
	{
	public:
		Vec2 velocity;
		float width = 1.0f;
		float height = 1.0f;

		RigidBodyComponent() = default;
		RigidBodyComponent(float w, float h) : width(w), height(h)
		{

		}

		void Serialize(ISerializer& s) override
		{
			if (s.HasKey("velocity_x")) StreamRead(s, "velocity_x", velocity.x);
			if (s.HasKey("velocity_y")) StreamRead(s, "velocity_y", velocity.y);
			if (s.HasKey("width")) StreamRead(s, "width", width);
			if (s.HasKey("height")) StreamRead(s, "height", height);
		}
	};
}