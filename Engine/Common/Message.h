#pragma once
#include <cstdint>

namespace Framework
{
	enum class MessageId : std::uint16_t {
		None,
		Quit,
		Collide,
		MouseClick
	 };

	class Message
	{
		Message(MessageId id) : MsgId(id) {};
		virtual ~Message() = default;
		MessageId MsgId;


	
	};
}