#pragma once

#include "realm/Realm.h"

namespace Game3 {
	class Inventory;

	class Cave: public Realm {
		public:
			static Identifier ID() { return {"base", "realm/cave"}; }
			RealmID parentRealm;
			size_t entranceCount = 1;

			Cave(const Cave &) = delete;
			Cave(Cave &&) = delete;
			~Cave() override;

			Cave & operator=(const Cave &) = delete;
			Cave & operator=(Cave &&) = delete;

			bool interactGround(const std::shared_ptr<Player> &, const Position &) override;
			void reveal(const Position &);

			friend class Realm;

		protected:
			Cave() = delete;
			Cave(Game &, RealmID, RealmID parent_realm, TilemapPtr tilemap1_, TilemapPtr tilemap2_, TilemapPtr tilemap3_, BiomeMapPtr, int seed_);
			Cave(Game &, RealmID, RealmID parent_realm, TilemapPtr tilemap1_, BiomeMapPtr, int seed_);

			void absorbJSON(const nlohmann::json &) override;
			void toJSON(nlohmann::json &) const override;
	};
}
