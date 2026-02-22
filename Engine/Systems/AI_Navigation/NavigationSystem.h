#pragma once
#include "Factory/Factory.h"
#include "Physics/Collision/Collision.h"
#include "Physics/Dynamics/RigidBodyComponent.h"
#include "Component/TransformComponent.h"
#include "Component/WayPointComponent.h"
#include "Component/EnemyComponent.h"
#include "Component/EnemyDecisionTreeComponent.h"
#include "../Engine/Graphics/Window.hpp"
#include "GraphNode.h"
#include "NavGraph.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <algorithm>
namespace Framework
{
	class NavSystem :public Framework::ISystem {
	public:
		explicit NavSystem(gfx::Window& window);
		void Initialize() override;
		void Update(float dt) override;
		void draw() override;
		void Shutdown() override;
		std::string GetName() override { return "NavSystem"; }
	private:
		gfx::Window* window;
		NavGraph navGraph;
		void RequestPath(GOC* enemy, int goalNodeID);
		void BuildGraphFromWaypoints();
	};
}