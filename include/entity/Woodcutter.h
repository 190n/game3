#pragma once

#include "entity/Worker.h"

namespace Game3 {
	/** Lives in a town and chops wood during the day. */
	class Woodcutter: public Worker {
		public:
			static Identifier ID() { return {"base", "entity/woodcutter"}; }
			constexpr static Index RADIUS = 50;
			constexpr static float HARVESTING_TIME = 5.f;
			constexpr static float SELLING_TIME = 5.f;

			Index chosenResource = -1;
			float harvestingTime;

			static std::shared_ptr<Woodcutter> create(Game &, RealmID overworld_realm, RealmID house_realm, Position house_position, std::shared_ptr<Building> keep_);
			static std::shared_ptr<Woodcutter> fromJSON(Game &, const nlohmann::json &);

			void toJSON(nlohmann::json &) const override;
			void absorbJSON(Game &, const nlohmann::json &) override;
			bool onInteractNextTo(const std::shared_ptr<Player> &) override;
			void tick(Game &, float delta) override;
			Glib::ustring getName() override { return "Woodcutter"; }

			friend class Entity;

		protected:
			float sellTime = 0.f;

			Woodcutter();
			Woodcutter(RealmID overworld_realm, RealmID house_realm, Position house_position, std::shared_ptr<Building> keep_);

			void interact(const Position &);

		private:
			void wakeUp();
			void goToResource();
			void startHarvesting();
			void harvest(float delta);
			void sellInventory();
	};

	void to_json(nlohmann::json &, const Woodcutter &);
}
