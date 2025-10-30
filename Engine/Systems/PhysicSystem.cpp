#include "PhysicSystem.h"
#include <iostream>

namespace Framework {
    PhysicSystem::PhysicSystem(LogicSystem& logic)
        : logic(logic) {
    }
    void PhysicSystem::Initialize() {
        //for (GOC Object : GameObjs)
        //{

        //}

    }
    void PhysicSystem::Update(float dt)
    {
        //for (GOC)
        const auto& info = logic.Collision();
        if (info.playerValid && info.targetValid)
        {
            if (Collision::CheckCollisionRectToRect(info.player, info.target))
            {
                std::cout << "Collision detected!" << std::endl;
            }
        }
    }

    void PhysicSystem::Shutdown() {
        
    }
}

