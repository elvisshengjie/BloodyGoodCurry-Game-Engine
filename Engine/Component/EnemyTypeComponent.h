#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include <string>
namespace Framework
{
    
    class EnemyTypeComponent : public GameComponent 
    {
        public:
            enum class EnemyType{phyiscal, ranged};
            EnemyType type{EnemyType::phyiscal};
            EnemyTypeComponent() = default;
            EnemyTypeComponent(EnemyType t) : type(t){}
            void initialize() override {}
            void SendMessage(Message& m) override { (void)m; }
            void Serialize(ISerializer& s) override 
            {
                std::string typeStr;
                if (s.HasKey("type"))
                {
                    std::string typeStr;
                    StreamRead(s,"type",typeStr);
                    if(typeStr=="ranged" ||typeStr=="Ranged"){type = EnemyType::ranged;}
                    else type = EnemyType::phyiscal;
                }
            }
            std::unique_ptr<GameComponent> Clone() const override 
            {
             auto copy = std::make_unique<EnemyTypeComponent>();
             copy->type = type;
             return copy;
            }
    };
}