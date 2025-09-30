#pragma once
#include "Composition/Component.h"
#include "Common/ComponentTypeID.h"
#include "Serialization/Serialization.h"

namespace Framework
{
	class RigidBodyComponent : public GameComponent
	{
	public:
		float velX = 1.0f;
		float velY = 1.0f;
		float width = 1.0f;
		float height = 1.0f;

		void initialize() override {}
		void SendMessage(Message& m) override { (void)m; }

		void Serialize(ISerializer& s) override
		{
			if (s.HasKey("velocity_x")) StreamRead(s, "velocity_x", velX);
			if (s.HasKey("velocity_y")) StreamRead(s, "velocity_y", velY);
			if (s.HasKey("width")) StreamRead(s, "width", width);
			if (s.HasKey("height")) StreamRead(s, "height", height);
		}

		std::unique_ptr<GameComponent>Clone() const override 
		{
			// Create new CircleRenderComponent on heap
			// Wrap inside unique_ptr so it is automatically clean up if something goes wrong
			auto copy = std::make_unique<RigidBodyComponent>();
			//copy the values 
			copy->velX = velX;
			copy->velY = velY;
			copy->width = width;
			copy->height = height;
			//Transfer ownership to whoever call clone()
			return copy;

		}
	};
}