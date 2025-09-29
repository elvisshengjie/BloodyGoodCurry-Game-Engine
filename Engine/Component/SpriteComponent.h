#pragma once
#include "Composition/Component.h"
#include "Serialization/Serialization.h"
#include "Resource_Manager/Resource_Manager.h"

namespace Framework {
	class SpriteComponent : public GameComponent {
	public:
		//runtime
		unsigned int texture_id{ 0 };  //GL id 


		std::string texture_key;
		std::string path;

		void initialize() override {
			if (!texture_key.empty()) {
				texture_id = Resource_Manager::getTexture(texture_key);
				if (!texture_id && !path.empty()) {
					// load file and re-fetch id
					if (Resource_Manager::load(texture_key, path)) {
						texture_id = Resource_Manager::getTexture(texture_key);
					}
				}
			}
		}
		void Serialize(ISerializer& s) override {
			if (s.HasKey("texture_key")) StreamRead(s, "texture_key", texture_key);
		}

		std::unique_ptr<GameComponent> Clone() const override {
			auto copy = std::make_unique<SpriteComponent>();
			copy->texture_key = texture_key;
			copy->texture_id = texture_id;
			copy->path = path;
			return copy;
		}

		void SendMessage(Message&) override {}
	};


}