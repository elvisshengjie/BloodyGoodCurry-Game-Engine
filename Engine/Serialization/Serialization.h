#pragma once
#include <string>

namespace Framework 
{
	class ISerializer
	{
	public:
		virtual bool Open(const std::string& file)=0;
		virtual bool IsGood() = 0;
	
		// Hierarchy navigation
		virtual bool EnterObject(const std::string& key) = 0;  // Move into object by key
		virtual void ExitObject() = 0;                         // Move back out

		// Check if key exists
		virtual bool HasKey(const std::string& key) const = 0;

		// Read primitives by key
		virtual void ReadInt(const std::string& key, int& out) = 0;
		virtual void ReadFloat(const std::string& key, float& out) = 0;
		virtual void ReadString(const std::string& key, std::string& out) = 0;
		
		virtual bool EnterArray(const std::string& key) = 0;
		virtual void ExitArray() = 0;
		virtual size_t ArraySize() const = 0;
		virtual bool EnterIndex(size_t i) = 0;
	};


	template<typename type>
	inline void StreamRead(ISerializer& stream, type& typeInstance)
	{
		typeInstance.Serialize(stream);
	}
	inline void StreamRead(ISerializer& stream, const std::string& key, int& out) {
		stream.ReadInt(key, out);
	}

	inline void StreamRead(ISerializer& stream, const std::string& key, float& out) {
		stream.ReadFloat(key, out);
	}

	inline void StreamRead(ISerializer& stream, const std::string& key, std::string& out) {
		stream.ReadString(key, out);
	}

}