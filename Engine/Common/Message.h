#pragma once

namespace Framework
{
	namespace Mid
	{
		enum MessageIdType
		{
			None,
			Quit,
			Collide,
			MouseClick
		};

	};

	class Message
	{
	public: 
		Message(Mid::MessageIdType id) : MessageId(id) {};
		Mid::MessageIdType MessageId;
		virtual ~Message() {};
	};
}