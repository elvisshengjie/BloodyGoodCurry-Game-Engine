#pragma once

enum MessageID
{
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_M,
    KEY_S
};

class Message
{
    public:
    MessageID type;
    Message(MessageID t) : type(t) {}
};