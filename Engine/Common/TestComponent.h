#pragma once
// TestComponent.h
#pragma once
#include "Composition/Component.h"
#include <iostream>

namespace Framework {

    // A simple message payload to ping the component
    struct PingMessage : public Message {
        int value{};
        PingMessage(int v) : Message(MessageId::None), value(v) {}
    };

    class TestComponent : public GameComponent {
    public:
        // Some state to prove serialization works
        std::string name{ "unnamed" };
        int hp{ 100 };

        void initialize() override {
            std::cout << "[TestComponent] initialize: name=" << name << ", hp=" << hp << "\n";
        }

        void SendMessage(Message& m) override {
            // Prove message dispatch works
            if (auto* ping = dynamic_cast<PingMessage*>(&m)) {
                std::cout << "[TestComponent] got PingMessage value=" << ping->value << "\n";
            }
            else if (m.MsgId == MessageId::Quit) {
                std::cout << "[TestComponent] got Quit message\n";
            }
        }

        void Serialize(ISerializer& s) override {
            if (s.EnterObject("TestComponent")) {
                StreamRead(s, "name", name);
                StreamRead(s, "hp", hp);
                s.ExitObject();
            }
        }
    };

}