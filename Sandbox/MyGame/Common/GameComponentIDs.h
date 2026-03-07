/*********************************************************************************************
 \file      GameComponentIDs.h
 \par       SofaSpuds
 \brief     Registers BloodyGoodCurry gameplay component IDs with the runtime registry.
*********************************************************************************************/
#pragma once

#include "Common/ComponentTypeID.h"

namespace mygame
{
    class GameComponentIDs
    {
    public:
        /*************************************************************************************
         \brief Returns the singleton storing BloodyGoodCurry's component IDs.
        *************************************************************************************/
        static GameComponentIDs& Get()
        {
            static GameComponentIDs ids;
            return ids;
        }

        /*************************************************************************************
         \brief Registers all current-game component names with the runtime registry.
         \details
           - Safe to call more than once.
           - Reserves the legacy IDs currently used by BloodyGoodCurry save data so
             existing levels continue to deserialize correctly.
        *************************************************************************************/
        void Init()
        {
            if (initialized)
                return;

            auto& registry = Framework::ComponentTypeRegistry::Get();
            glowComponent = registry.Register("GlowComponent", Framework::ComponentTypeId::CT_GlowComponent);
            playerComponent = registry.Register("PlayerComponent", Framework::ComponentTypeId::CT_PlayerComponent);
            playerHealthComponent = registry.Register("PlayerHealthComponent", Framework::ComponentTypeId::CT_PlayerHealthComponent);
            playerAttackComponent = registry.Register("PlayerAttackComponent", Framework::ComponentTypeId::CT_PlayerAttackComponent);
            playerHudComponent = registry.Register("PlayerHUDComponent", Framework::ComponentTypeId::CT_PlayerHUDComponent);
            enemyComponent = registry.Register("EnemyComponent", Framework::ComponentTypeId::CT_EnemyComponent);
            enemyDecisionTreeComponent = registry.Register("EnemyDecisionTreeComponent", Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent);
            enemyAttackComponent = registry.Register("EnemyAttackComponent", Framework::ComponentTypeId::CT_EnemyAttackComponent);
            enemyHealthComponent = registry.Register("EnemyHealthComponent", Framework::ComponentTypeId::CT_EnemyHealthComponent);
            enemyTypeComponent = registry.Register("EnemyTypeComponent", Framework::ComponentTypeId::CT_EnemyTypeComponent);
            wayPointComponent = registry.Register("WayPointComponent", Framework::ComponentTypeId::CT_WayPointComponent);
            zoomTriggerComponent = registry.Register("ZoomTriggerComponent", Framework::ComponentTypeId::CT_ZoomTriggerComponent);
            gateTargetComponent = registry.Register("GateTargetComponent", Framework::ComponentTypeId::CT_GateTargetComponent);
            initialized = true;
        }

        /*************************************************************************************
         \brief Returns the runtime ID used for GlowComponent.
        *************************************************************************************/
        Framework::ComponentTypeId GlowComponent() const { return glowComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for PlayerComponent.
        *************************************************************************************/
        Framework::ComponentTypeId PlayerComponent() const { return playerComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for PlayerHealthComponent.
        *************************************************************************************/
        Framework::ComponentTypeId PlayerHealthComponent() const { return playerHealthComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for PlayerAttackComponent.
        *************************************************************************************/
        Framework::ComponentTypeId PlayerAttackComponent() const { return playerAttackComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for PlayerHUDComponent.
        *************************************************************************************/
        Framework::ComponentTypeId PlayerHUDComponent() const { return playerHudComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for EnemyComponent.
        *************************************************************************************/
        Framework::ComponentTypeId EnemyComponent() const { return enemyComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for EnemyDecisionTreeComponent.
        *************************************************************************************/
        Framework::ComponentTypeId EnemyDecisionTreeComponent() const { return enemyDecisionTreeComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for EnemyAttackComponent.
        *************************************************************************************/
        Framework::ComponentTypeId EnemyAttackComponent() const { return enemyAttackComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for EnemyHealthComponent.
        *************************************************************************************/
        Framework::ComponentTypeId EnemyHealthComponent() const { return enemyHealthComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for EnemyTypeComponent.
        *************************************************************************************/
        Framework::ComponentTypeId EnemyTypeComponent() const { return enemyTypeComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for WayPointComponent.
        *************************************************************************************/
        Framework::ComponentTypeId WayPointComponent() const { return wayPointComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for ZoomTriggerComponent.
        *************************************************************************************/
        Framework::ComponentTypeId ZoomTriggerComponent() const { return zoomTriggerComponent; }
        /*************************************************************************************
         \brief Returns the runtime ID used for GateTargetComponent.
        *************************************************************************************/
        Framework::ComponentTypeId GateTargetComponent() const { return gateTargetComponent; }

    private:
        bool initialized{ false };
        Framework::ComponentTypeId glowComponent{ Framework::ComponentTypeId::CT_GlowComponent };
        Framework::ComponentTypeId playerComponent{ Framework::ComponentTypeId::CT_PlayerComponent };
        Framework::ComponentTypeId playerHealthComponent{ Framework::ComponentTypeId::CT_PlayerHealthComponent };
        Framework::ComponentTypeId playerAttackComponent{ Framework::ComponentTypeId::CT_PlayerAttackComponent };
        Framework::ComponentTypeId playerHudComponent{ Framework::ComponentTypeId::CT_PlayerHUDComponent };
        Framework::ComponentTypeId enemyComponent{ Framework::ComponentTypeId::CT_EnemyComponent };
        Framework::ComponentTypeId enemyDecisionTreeComponent{ Framework::ComponentTypeId::CT_EnemyDecisionTreeComponent };
        Framework::ComponentTypeId enemyAttackComponent{ Framework::ComponentTypeId::CT_EnemyAttackComponent };
        Framework::ComponentTypeId enemyHealthComponent{ Framework::ComponentTypeId::CT_EnemyHealthComponent };
        Framework::ComponentTypeId enemyTypeComponent{ Framework::ComponentTypeId::CT_EnemyTypeComponent };
        Framework::ComponentTypeId wayPointComponent{ Framework::ComponentTypeId::CT_WayPointComponent };
        Framework::ComponentTypeId zoomTriggerComponent{ Framework::ComponentTypeId::CT_ZoomTriggerComponent };
        Framework::ComponentTypeId gateTargetComponent{ Framework::ComponentTypeId::CT_GateTargetComponent };
    };

    /*************************************************************************************
     \brief Convenience accessors matching the legacy CT_* call style on the game side.
    *************************************************************************************/
    inline Framework::ComponentTypeId CT_GlowComponent() { return GameComponentIDs::Get().GlowComponent(); }
    inline Framework::ComponentTypeId CT_PlayerComponent() { return GameComponentIDs::Get().PlayerComponent(); }
    inline Framework::ComponentTypeId CT_PlayerHealthComponent() { return GameComponentIDs::Get().PlayerHealthComponent(); }
    inline Framework::ComponentTypeId CT_PlayerAttackComponent() { return GameComponentIDs::Get().PlayerAttackComponent(); }
    inline Framework::ComponentTypeId CT_PlayerHUDComponent() { return GameComponentIDs::Get().PlayerHUDComponent(); }
    inline Framework::ComponentTypeId CT_EnemyComponent() { return GameComponentIDs::Get().EnemyComponent(); }
    inline Framework::ComponentTypeId CT_EnemyDecisionTreeComponent() { return GameComponentIDs::Get().EnemyDecisionTreeComponent(); }
    inline Framework::ComponentTypeId CT_EnemyAttackComponent() { return GameComponentIDs::Get().EnemyAttackComponent(); }
    inline Framework::ComponentTypeId CT_EnemyHealthComponent() { return GameComponentIDs::Get().EnemyHealthComponent(); }
    inline Framework::ComponentTypeId CT_EnemyTypeComponent() { return GameComponentIDs::Get().EnemyTypeComponent(); }
    inline Framework::ComponentTypeId CT_WayPointComponent() { return GameComponentIDs::Get().WayPointComponent(); }
    inline Framework::ComponentTypeId CT_ZoomTriggerComponent() { return GameComponentIDs::Get().ZoomTriggerComponent(); }
    inline Framework::ComponentTypeId CT_GateTargetComponent() { return GameComponentIDs::Get().GateTargetComponent(); }
}
