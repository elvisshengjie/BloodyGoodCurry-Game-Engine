#include "HitBoxSystem.h"
#include "Composition/Component.h"
#include "LogicSystem.h"
#include <iostream>

namespace Framework
{
	HitBoxSystem::HitBoxSystem(LogicSystem& logicRef) : logic(logicRef)
	{

	}

	HitBoxSystem::~HitBoxSystem()
	{
		Shutdown();
	}

	void HitBoxSystem::Initialize()
	{
		activeHitBoxes.clear();
	}

	void HitBoxSystem::Shutdown()
	{
		activeHitBoxes.clear();
	}

	void HitBoxSystem::SpawnHitBox(GameObjectComposition* attacker,
		float targetX, float targetY,
		float width, float height,
		float damage,
		float duration)
	{
		if (!attacker)
			return;

		auto hitbox = std::make_unique<HurtBoxComponent>();
		hitbox->spawnX = targetX;
		hitbox->spawnY = targetY;
		hitbox->width = width;
		hitbox->height = height;
		hitbox->damage = damage;
		hitbox->duration = duration;
		hitbox->ActivateHurtBox();

		ActiveHitBox active;
		active.hitbox = std::move(hitbox);
		active.owner = attacker;
		active.timer = duration;

		activeHitBoxes.push_back(std::move(active));
	}

	void HitBoxSystem::Update(float dt)
	{
		bool hit = false;
		for (auto it = activeHitBoxes.begin(); it != activeHitBoxes.end();)
		{
			it->timer -= dt;	
			for (auto* obj : logic.LevelObjects())
			{
				if (!obj || obj == it->owner)
					continue;

				auto* hurtbox = obj->GetComponentType<HurtBoxComponent>(ComponentTypeId::CT_HurtBoxComponent);

				if (hurtbox && hurtbox->active)
				{
					AABB hitboxAABB(it->hitbox->spawnX, it->hitbox->spawnY,
									it->hitbox->width, it->hitbox->height);

					AABB hurtboxAABB(hurtbox->spawnX, hurtbox->spawnY,
									 hurtbox->width, hurtbox->height);

					if (Collision::CheckCollisionRectToRect(hitboxAABB, hurtboxAABB))
					{
						// [Insert code here] Whoever the hurtbox owner is will take damage here
						hit = true;
						break;
					}
				}
			}

			if (hit)
			{
				it = activeHitBoxes.erase(it);
			}
			else // Remove hitbox if duration expires
			{
				it->timer -= dt;
				if (it->timer <= 0.f)
				{
					it = activeHitBoxes.erase(it);
				}
				else
				{
					it++;
				}
			}
		}
	}
}