#pragma once
#include <string>

namespace Framework 
{
	class ISerializer
	{
	public:
		virtual bool Open(const std::string& file)=0;
		virtual bool IsGood() = 0;
		virtual void ReadFloat(float& i) = 0;
		virtual void ReadInt(int& i) = 0;
		virtual void ReadString(std::string& str) = 0;

	};


	template<typename type>
	inline void StreamRead(ISerializer& stream, type& typeInstance)
	{
		typeInstance.Serialize(stream);
	}
	inline void StreamRead(ISerializer& stream, float& f)
	{
		stream.ReadFloat(f);
	}

	inline void StreamRead(ISerializer& stream, int& i) {

		stream.ReadInt(i);
	}
	inline void StreamRead(ISerializer& stream, std::string& str)
	{
		stream.ReadString(str);
	}
}