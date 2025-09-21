#pragma once
#include "Message.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

class MessageBus
{
    public:
        using Callback = std::function<void()>;
        void subscribe(MessageID id, Callback cb){subscribers[id].push_back(cb);}
        void publish(const Message& msg)
        {  auto it = subscribers.find(msg.type);
            if (it != subscribers.end())
            { for (auto& cb : it->second) cb();}
        }
    private:
        std::unordered_map<MessageID, std::vector<Callback>> subscribers;
};