#pragma once
#include "Composition/Component.h"
#include "LogicSystem.h"

namespace Framework
{

	class GameObjectComposition;
	class HurtBoxComponent;
	class LogicSystem;

	class HitBoxSystem
	{
	public:
		HitBoxSystem(LogicSystem& logic);
		~HitBoxSystem();

		void Initialize();
		void Update(float dt);
		void Shutdown();

		void SpawnHitBox(GameObjectComposition* attacker,
			float targetX, float targetY,
			float width = 0.2f, float height = 0.2f,
			float damage = 1.0f,
			float duration = 0.1f);

	private:
		struct ActiveHitBox
		{
			std::unique_ptr<HurtBoxComponent> hitbox;
			GameObjectComposition* owner;
			float timer;
		};

		LogicSystem& logic;
		std::vector<ActiveHitBox> activeHitBoxes; 
	};



}
