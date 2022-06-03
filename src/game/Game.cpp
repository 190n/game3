#include <iostream>

#include "game/Game.h"
#include "util/Util.h"

namespace Game3 {
	void to_json(nlohmann::json &json, const Game &game) {
		json["realms"] = std::unordered_map<std::string, nlohmann::json>();
		for (const auto &[id, realm]: game.realms)
			json["realms"][std::to_string(id)] = nlohmann::json(*realm);
	}

	void from_json(const nlohmann::json &json, Game &game) {
		for (const auto &[string, realm_json]: json.at("realms").get<std::unordered_map<std::string, nlohmann::json>>())
			game.realms.emplace(parseUlong(string), std::make_shared<Realm>(realm_json));
	}
}
