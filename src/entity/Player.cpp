#include <iostream>

#include "entity/Player.h"
#include "game/Game.h"
#include "realm/Realm.h"
#include "ui/Canvas.h"
#include "ui/MainWindow.h"

namespace Game3 {
	Player::Player(EntityID id__): Entity(id__, Entity::PLAYER_TYPE) {}

	std::shared_ptr<Player> Player::fromJSON(const nlohmann::json &json) {
		auto out = Entity::create<Player>(json.at("id"));
		out->absorbJSON(json);
		return out;
	}

	nlohmann::json Player::toJSON() const {
		nlohmann::json json;
		to_json(json, *this);
		return json;
	}

	void Player::absorbJSON(const nlohmann::json &json) {
		Entity::absorbJSON(json);
		money = json.contains("money")? json.at("money").get<MoneyCount>() : 0;
		tooldown = json.contains("tooldown")? json.at("tooldown").get<float>() : 0.f;
	}

	void Player::tick(Game &game, float delta) {
		Entity::tick(game, delta);
		tooldown = std::max(0.f, tooldown - delta);
		Direction final_direction = direction;
		if (movingLeft)
			move(final_direction = Direction::Left);
		if (movingRight)
			move(final_direction = Direction::Right);
		if (movingUp)
			move(final_direction = Direction::Up);
		if (movingDown)
			move(final_direction = Direction::Down);
		direction = final_direction;
	}

	void Player::interactOn() {
		auto realm = getRealm();
		auto player = std::dynamic_pointer_cast<Player>(shared_from_this());
		auto entity = realm->findEntity(position, player);
		if (!entity)
			return;
		entity->onInteractOn(player);
	}

	void Player::interactNextTo() {
		auto realm = getRealm();
		auto player = std::dynamic_pointer_cast<Player>(shared_from_this());
		const Position next_to = nextTo();
		auto entity = realm->findEntity(next_to, player);
		bool interesting = false;
		if (entity)
			interesting = entity->onInteractNextTo(player);
		if (!interesting)
			if (auto tileEntity = realm->tileEntityAt(next_to))
				interesting = tileEntity->onInteractNextTo(player);
		if (!interesting)
			realm->interactGround(player, next_to);
	}

	void Player::teleport(const Position &position, const std::shared_ptr<Realm> &new_realm) {
		Entity::teleport(position, new_realm);
		auto &game = new_realm->getGame();
		game.activeRealm = new_realm;
		game.canvas.window.glContext()->make_current();
		new_realm->reupload();
		focus(game.canvas, false);
	}

	void Player::addMoney(MoneyCount to_add) {
		money += to_add;
		getRealm()->getGame().signal_player_money_update().emit(std::dynamic_pointer_cast<Player>(shared_from_this()));
	}

	void to_json(nlohmann::json &json, const Player &player) {
		to_json(json, static_cast<const Entity &>(player));
		json["isPlayer"] = true;
		if (0 < player.money)
			json["money"] = player.money;
		if (0.f < player.tooldown)
			json["tooldown"] = player.tooldown;
	}
}
