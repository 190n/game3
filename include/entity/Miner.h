#pragma once

#include "Types.h"
#include "entity/Worker.h"

namespace Game3 {
	/** Lives in a town and mines ore during the day. */
	class Miner: public Worker {
		public:
			static Identifier ID() { return {"base", "entity/miner"}; }
			constexpr static Index RADIUS = 50;
			constexpr static float HARVESTING_TIME = 5.f;
			constexpr static float SELLING_TIME = 5.f;

			Index chosenResource = -1;
			float harvestingTime;

			static std::shared_ptr<Miner> create(Game &, RealmID overworld_realm, RealmID house_realm, const Position &house_position, const std::shared_ptr<Building> &keep_);
			static std::shared_ptr<Miner> fromJSON(Game &, const nlohmann::json &);

			void toJSON(nlohmann::json &) const override;
			void absorbJSON(Game &, const nlohmann::json &) override;
			bool onInteractNextTo(const std::shared_ptr<Player> &) override;
			void tick(Game &, float delta) override;
			Glib::ustring getName() override { return "Miner"; }

			friend class Entity;

		protected:
			float sellTime = 0.f;

			Miner();
			Miner(RealmID overworld_realm, RealmID house_realm, const Position &house_position, const std::shared_ptr<Building> &keep_);

			void interact(const Position &);

		private:
			void wakeUp();
			void goToResource();
			void startHarvesting();
			void harvest(float delta);
			void sellInventory();
	};

	void to_json(nlohmann::json &, const Miner &);
}
