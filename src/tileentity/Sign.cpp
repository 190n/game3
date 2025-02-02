#include <iostream>

#include "Tileset.h"
#include "entity/Player.h"
#include "game/Game.h"
#include "realm/Realm.h"
#include "tileentity/Sign.h"
#include "ui/Canvas.h"
#include "ui/MainWindow.h"
#include "ui/SpriteRenderer.h"
#include "ui/tab/TextTab.h"

namespace Game3 {
	Sign::Sign(Identifier tilename, Position position_, std::string text_, std::string name_):
		TileEntity(std::move(tilename), ID(), std::move(position_), false), text(std::move(text_)), name(std::move(name_)) {}

	void Sign::toJSON(nlohmann::json &json) const {
		TileEntity::toJSON(json);
		json["text"] = text;
		json["name"] = name;
	}

	bool Sign::onInteractNextTo(const std::shared_ptr<Player> &player) {
		getRealm()->getGame().setText(text, name, true, true);
		player->queueForMove([player](const auto &) {
			player->getRealm()->getGame().canvas.window.textTab->hide();
			return true;
		});
		return true;
	}

	void Sign::absorbJSON(Game &game, const nlohmann::json &json) {
		TileEntity::absorbJSON(game, json);
		text = json.at("text");
		name = json.at("name");
	}

	// void Sign::render(SpriteRenderer &sprite_renderer) const {
	// 	auto realm = getRealm();
	// 	if (tileID != tileSets.at(realm->type)->getEmpty()) {
	// 		auto &tilemap = *realm->tilemap2;
	// 		const auto tilesize = tilemap.tileSize;
	// 		const auto x = (tileID % (tilemap.setWidth / tilesize)) * tilesize;
	// 		const auto y = (tileID / (tilemap.setWidth / tilesize)) * tilesize;
	// 		sprite_renderer.drawOnMap(tilemap.texture, position.column, position.row, x / 2, y / 2, tilesize, tilesize);
	// 	}
	// }
}
