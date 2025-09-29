#pragma once
// Physics.h
#include <vector> // Might not need since we made our own
#include "Math/Vector_2D.h"
#include "Physics/Collision/Collision.h"
#include "Composition/Component.h"
#include "Common/System.h"
#include "Factory/Factory.h"
#include "Component/TransformComponent.h"

namespace Framework
{
	class RigidBodyComponent : public GameComponent
	{
	public:
		float width = 1.0f, height = 1.0f;
		float velX = 0.0f, velY = 0.0f;
	};

	class PhysicsSystem : public ISystem
	{
	public:
		void Update(float dt) override
		{
			for (auto& [id, obj] : FACTORY->Objects())
			{
				if (!obj)
					continue;

				auto* rb = obj->GetComponentType<RigidBodyComponent>(ComponentTypeId::CT_RigidBodyComponents);
				auto* tr = obj->GetComponentType<TransformComponent>(ComponentTypeId::CT_TransformComponent);

				if (!rb || !tr)
					continue;

				// Update pos based on vel
				tr->x += rb->velX * dt;
				tr->y += rb->velY * dt;

				// Check collision (later)
			}
		}

		std::string GetName() override { return "PhysicsSystem"; }
	};
}