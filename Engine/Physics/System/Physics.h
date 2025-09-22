#pragma once
// Physics.h
#include <vector>
#include "Physics/Collision/Collision.h"

namespace Framework
{
	class PhysicsObject
	{
	public:
		PhysicsObject(float x, float y, float width, float height);

		void SetVelocity(float vx, float vy);
		void Update(float dt);

		AABB GetAABB() const;

		float posX, posY;
		float width, height;
		float velX = 0.0f, velY = 0.0f;
	};

	class PhysicsSystem
	{
	public:
		void addObject(PhysicsObject* obj);
		void Update(float dt);

	private:
		std::vector<PhysicsObject*> objects;
	};
}