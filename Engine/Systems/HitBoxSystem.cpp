/*********************************************************************************************
 \file      HitBoxSystem.cpp
 \par       SofaSpuds
 \author    Ho Jun (h.jun@digipen.edu) - Primary Author, 100%
 \brief     Spawns and updates short-lived hit boxes for attack interactions.
 \details   This lightweight system manages transient attack volumes (HitBoxComponent):
			- Creation: SpawnHitBox() attaches owner/context and a lifetime timer.
			- Lifetime: Each active hit box counts down; removed when it expires or hits.
			- Collision: On each Update(), checks hit box vs. world hurt boxes (other objs'
			  HitBoxComponent flagged active) via AABB overlap.
			- Integration: Driven by LogicSystem (e.g., mouse click creates a hit box in
			  the player�s facing direction).

			Notes:
			* The same HitBoxComponent struct is reused for both "hit" and "hurt" roles:
			  - Newly spawned (this system) is used as the "hit" volume.
			  - Other objects expose their "hurt" volume when HitBoxComponent::active == true.
			* Collision uses AABB vs AABB through Collision::CheckCollisionRectToRect.
			* This module stores hit boxes internally (not added to factory); they are
			  ephemeral gameplay helpers rather than persistent game objects.
 \copyright
			All content �2025 DigiPen Institute of Technology Singapore.
			All rights reserved.
*********************************************************************************************/

#include "HitBoxSystem.h"
#include "Composition/Component.h"
#include "LogicSystem.h"
#include <iostream>
#include "Component/HitBoxComponent.h"

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

		auto newhitbox = std::make_unique<HitBoxComponent>();
		newhitbox->spawnX = targetX;
		newhitbox->spawnY = targetY;
		newhitbox->width = width;
		newhitbox->height = height;
		newhitbox->damage = damage;
		newhitbox->duration = duration;
		newhitbox->owner = attacker;
		newhitbox->ActivateHurtBox();

		ActiveHitBox active;
		active.hitbox = std::move(newhitbox);
		active.owner = attacker;
		active.timer = duration;

		activeHitBoxes.push_back(std::move(active));
	}

	void HitBoxSystem::Update(float dt)
	{
		for (auto it = activeHitBoxes.begin(); it != activeHitBoxes.end();)
		{
			bool hit = false;
			it->timer -= dt;

			AABB hitboxAABB(
				it->hitbox->spawnX,
				it->hitbox->spawnY,
				it->hitbox->width,
				it->hitbox->height
			);
			for (auto* obj : logic.LevelObjects())
			{
				if (!obj || obj == it->owner)
					continue;

				auto* hitbox = obj->GetComponentType<HitBoxComponent>(ComponentTypeId::CT_HitBoxComponent);

				if (hitbox)
				{
					if (!hitbox->active)
						hitbox->ActivateHurtBox(); // ensure it's active

					AABB playerHit(it->hitbox->spawnX, it->hitbox->spawnY,
						it->hitbox->width, it->hitbox->height);

					AABB hurtboxAABB(hitbox->spawnX, hitbox->spawnY,
									 hitbox->width, hitbox->height);

					AABB enemyHit(hitbox->spawnX, hitbox->spawnY,
						hitbox->width, hitbox->height);

					/*if (Collision::CheckCollisionRectToRect(playerHit, enemyHit))*/
						std::cout << "a";
					if (Collision::CheckCollisionRectToRect(playerHit, enemyHit))
					{
						auto* health = obj->GetComponentType<EnemyHealthComponent>(ComponentTypeId::CT_EnemyHealthComponent);
						if (health)
						{
							health->TakeDamage(static_cast<int>(it->hitbox->damage));
							std::cout << "Enemy hit! Remaining HP: " << health->enemyHealth << "\n";
						}
						hit = true;
						break;
					}
				}
			}

			if (hit || it->timer <= 0.f)
			{
				it = activeHitBoxes.erase(it);
			}
			else // Remove hitbox if duration expires
			{
				it++;
			}
		}
	}
}