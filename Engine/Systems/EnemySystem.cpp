
#include "EnemySystem.h"
#include <iostream>

using namespace Framework;

EnemySystem::EnemySystem(gfx::Window& window) : window(&window) {}

void EnemySystem::Initialize()
{
    std::cout << "[EnemySystem] Creating manual enemy...\n";
    // Create a new enemy GameObject
    GOC* enemy = FACTORY->CreateEmptyComposition();
    // Transform
    auto* transform = enemy->EmplaceComponent<TransformComponent>(ComponentTypeId::CT_TransformComponent);
    // Center of the window
    transform->x = window->Width()* 0.5f;
    transform->y = window->Height()* 0.5f;
    transform->rot = 0.0f;

    // Render
    auto* render = enemy->EmplaceComponent<RenderComponent>(ComponentTypeId::CT_RenderComponent);
    render->w = 64;
    render->h = 64;
    render->r = 1.0f;
    render->g = 0.0f;
    render->b = 0.0f;
    render->a = 1.0f;

    // Sprite
    auto* sprite = enemy->EmplaceComponent<SpriteComponent>(ComponentTypeId::CT_SpriteComponent);
    sprite->texture_key = "enemy_png"; // key from Resource_Manager
    sprite->initialize();               // load texture at runtime

    // Enemy-specific components
    enemy->EmplaceComponent<EnemyComponent>(ComponentTypeId::CT_EnemyComponent);

    auto* attack = enemy->EmplaceComponent<EnemyAttackComponent>(ComponentTypeId::CT_EnemyAttackComponent);
    attack->damage = 15;
    attack->attack_speed = 1.2f;

    auto* health = enemy->EmplaceComponent<EnemyHealthComponent>(ComponentTypeId::CT_EnemyHealthComponent);
    health->health = 100;
    health->maxhealth = 100;

    enemy->EmplaceComponent<EnemyDecisionTreeComponent>(ComponentTypeId::CT_EnemyDecisionTreeComponent);

    auto* typeComp = enemy->EmplaceComponent<EnemyTypeComponent>(ComponentTypeId::CT_EnemyTypeComponent);
    typeComp->Etype = EnemyTypeComponent::EnemyType::physical;
    enemy->initialize();
    // Store the enemy for update/shutdown
    enemies.push_back(enemy);

    std::cout << "[EnemySystem] Manual enemy created successfully.\n";

}

void EnemySystem::Update(float dt)
{
    (void)dt;
}

void EnemySystem::Shutdown()
{
    std::cout << "[EnemySystem] Shutdown.\n";
    for (GOC* enemy : enemies) 
    {FACTORY->Destroy(enemy);}
    enemies.clear();
}


