#include <iostream>

#include "Tileset.h"
#include "game/Game.h"
#include "game/Inventory.h"
#include "realm/Cave.h"
#include "tileentity/Building.h"

namespace Game3 {
	Cave::Cave(Game &game_, RealmID id_, RealmID parent_realm, TilemapPtr tilemap1_, TilemapPtr tilemap2_, TilemapPtr tilemap3_, BiomeMapPtr biome_map, int seed_):
		Realm(game_, id_, ID(), std::move(tilemap1_), std::move(tilemap2_), std::move(tilemap3_), std::move(biome_map), seed_), parentRealm(parent_realm) {}

	Cave::Cave(Game &game_, RealmID id_, RealmID parent_realm, TilemapPtr tilemap1_, BiomeMapPtr biome_map, int seed_):
		Realm(game_, id_, ID(), std::move(tilemap1_), std::move(biome_map), seed_), parentRealm(parent_realm) {}

	Cave::~Cave() {
		// Assumptions:
		// - All entrances to a given cave realm appear in exactly one realm.
		//    -> If we find a cave entrance in one realm, we don't need to search other realms for another entrance to the same cave.
		// - All cave entrances in a given realm lead to the same cave.
		//    -> If we find one cave entrance in a realm, we can stop after destroying its linked cave and we don't have to look for more entrances.
		auto &game = getGame();
		for (const auto &[index, tile_entity]: tileEntities) {
			if (tile_entity->tileID != "base:tile/cave"_id)
				continue;
			if (auto building = std::dynamic_pointer_cast<Building>(tile_entity)) {
				if (auto cave_realm = std::dynamic_pointer_cast<Cave>(game.realms.at(building->innerRealmID)))
					game.realms.erase(building->innerRealmID);
				else
					std::cerr << "Cave entrance leads to realm " + std::to_string(building->innerRealmID) + ", which isn't a cave. Not erasing.\n";
				break;
			}
		}
	}

	bool Cave::interactGround(const std::shared_ptr<Player> &player, const Position &position) {
		if (Realm::interactGround(player, position))
			return true;

		const Index index = getIndex(position);

		std::optional<ItemStack> ore_stack;

		const TileID tile2 = (*tilemap2)[index];
		const Identifier &tile_id = (*tilemap2->tileset)[tile2];

		Game &game = getGame();

		if (tile_id == "base:tile/cave_coal"_id)
			ore_stack.emplace(game, "base:item/coal"_id, 1);
		else if (tile_id == "base:tile/cave_copper"_id)
			ore_stack.emplace(game, "base:item/copper_ore"_id, 1);
		else if (tile_id == "base:tile/cave_diamond"_id)
			ore_stack.emplace(game, "base:item/diamond_ore"_id, 1);
		else if (tile_id == "base:tile/cave_gold"_id)
			ore_stack.emplace(game, "base:item/gold_ore"_id, 1);
		else if (tile_id == "base:tile/cave_iron"_id)
			ore_stack.emplace(game, "base:item/iron_ore"_id, 1);
		else if (tile_id == "base:tile/cave_wall"_id)
			ore_stack.emplace(game, "base:item/stone"_id, 1);

		if (ore_stack) {
			Inventory &inventory = *player->inventory;
			if (auto *stack = inventory.getActive()) {
				if (stack->hasAttribute("base:attribute/pickaxe"_id) && !inventory.add(*ore_stack)) {
					setLayer2(index, tilemap2->tileset->getEmpty());
					getGame().activateContext();
					renderer2.reupload();
					reveal(position);
					if (stack->reduceDurability())
						inventory.erase(inventory.activeSlot);
					return true;
				}
			}
		}

		return false;
	}

	void Cave::reveal(const Position &position) {
		if (!isValid(position))
			return;

		if ((*tilemap2)[getIndex(position)] == tilemap2->tileset->getEmptyID()) {
			bool changed = false;
			const TileID void3 = (*tilemap3->tileset)["base:tile/void"];
			const TileID empty3 = tilemap3->tileset->getEmptyID();
			for (Index row_offset = -1; row_offset <= 1; ++row_offset) {
				for (Index column_offset = -1; column_offset <= 1; ++column_offset) {
					if (row_offset != 0 || column_offset != 0) {
						const Position offset_position = position + Position(row_offset, column_offset);
						if (!isValid(offset_position))
							continue;
						const Index index = getIndex(offset_position);
						const auto tile3 = (*tilemap3)[index];
						if (tile3 == void3) {
							tilemap3->set(index, empty3);
							changed = true;
						}
					}
				}
			}
			if (changed) {
				getGame().activateContext();
				renderer3.reupload();
			}
		}
	}

	void Cave::absorbJSON(const nlohmann::json &json) {
		Realm::absorbJSON(json);
		parentRealm = json.at("parentRealm");
		entranceCount = json.contains("entranceCount")? json.at("entranceCount").get<decltype(entranceCount)>() : 1;
	}

	void Cave::toJSON(nlohmann::json &json) const {
		Realm::toJSON(json);
		json["parentRealm"] = parentRealm;
		if (entranceCount != 1)
			json["entranceCount"] = entranceCount;
	}
}
