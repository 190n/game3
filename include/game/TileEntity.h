#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "Position.h"
#include "Types.h"

namespace Game3 {
	class Player;
	class Realm;
	class SpriteRenderer;

	class TileEntity: public std::enable_shared_from_this<TileEntity> {
		public:
			constexpr static TileEntityID BUILDING = 1;

			RealmID realmID = 0;
			std::weak_ptr<Realm> weakRealm;
			TileID tileID = 0;
			TileEntityID tileEntityID = 0;
			Position position {-1, -1};
			bool solid = false;

			template <typename T, typename... Args>
			static std::shared_ptr<T> create(Args && ...args) {
				auto out = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
				out->init();
				return out;
			}

			virtual ~TileEntity() = default;

			static std::shared_ptr<TileEntity> fromJSON(const nlohmann::json &);

			virtual void init() {}
			/** Returns the TileEntity ID. This is not the tile ID, which corresponds to a tile in the tileset. */
			virtual TileEntityID getID() const = 0;
			virtual void render(SpriteRenderer &) const {}
			/** Removes the tile entity from existence. */
			virtual void remove();
			/** Handles when the player interacts with the tile in front of them and that tile contains this tile entity. */
			virtual void onInteractNextTo(const std::shared_ptr<Player> &) {}
			void setRealm(const std::shared_ptr<Realm> &);
			std::shared_ptr<Realm> getRealm() const;

		protected:
			TileEntity() = default;
			TileEntity(TileID tile_id, TileEntityID tile_entity_id, const Position &position_, bool solid_):
				tileID(tile_id), tileEntityID(tile_entity_id), position(position_), solid(solid_) {}

			virtual void absorbJSON(const nlohmann::json &);
			virtual void toJSON(nlohmann::json &) const;

			friend void to_json(nlohmann::json &, const TileEntity &);
	};
}
