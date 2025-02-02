#include "Tileset.h"
#include "biome/Volcanic.h"
#include "item/Item.h"
#include "lib/noise.h"
#include "realm/Realm.h"
#include "tileentity/ItemSpawner.h"
#include "util/Timer.h"
#include "util/Util.h"
#include "worldgen/WorldGen.h"

namespace Game3 {
	void Volcanic::init(Realm &realm, int noise_seed, const std::shared_ptr<double[]> &saved_noise) {
		Biome::init(realm, noise_seed, saved_noise);
	}

	void Volcanic::generate(Index row, Index column, std::default_random_engine &, const noise::module::Perlin &perlin, const WorldGenParams &params) {
		Realm &realm = *getRealm();
		const auto wetness = params.wetness;

		auto &layer1  = realm.tilemap1->getTilesUnsafe();
		auto &tileset = *realm.tilemap1->tileset;
		const Index index = realm.getIndex(row, column);

		const double noise = perlin.GetValue(row / Biome::NOISE_ZOOM, column / Biome::NOISE_ZOOM, 0.666);
		savedNoise[index] = noise;

		static const Identifier deeper_water  = "base:tile/deeper_water"_id;
		static const Identifier deep_water    = "base:tile/deep_water"_id;
		static const Identifier water         = "base:tile/water"_id;
		static const Identifier shallow_water = "base:tile/shallow_water"_id;
		static const Identifier volcanic_sand = "base:tile/volcanic_sand"_id;
		static const Identifier lava          = "base:tile/lava"_id;
		static const Identifier volcanic_rock = "base:tile/volcanic_rock"_id;

		if (noise < wetness) {
			layer1[index] = tileset[deeper_water];
		} else if (noise < wetness + 0.1) {
			layer1[index] = tileset[deep_water];
		} else if (noise < wetness + 0.2) {
			layer1[index] = tileset[water];
		} else if (noise < wetness + 0.3) {
			layer1[index] = tileset[shallow_water];
		} else if (noise < wetness + 0.4) {
			layer1[index] = tileset[volcanic_sand];
		} else if (0.85 < noise) {
			layer1[index] = tileset[lava];
		} else {
			layer1[index] = tileset[volcanic_rock];
		}
	}

	void Volcanic::postgen(Index row, Index column, std::default_random_engine &rng, const noise::module::Perlin &, const WorldGenParams &) {
		Realm &realm = *getRealm();
		static std::uniform_int_distribution distribution(0, 199);

		if (realm.getLayer1(row, column) == realm.getTileset()["base:tile/volcanic_sand"]) {
			if (distribution(rng) < 1) {
				Game &game = realm.getGame();
				std::vector<ItemStack> mushrooms {
					{game, "base:item/indigo_milkcap"_id},
					{game, "base:item/black_trumpet"_id},
					{game, "base:item/grey_knight"_id},
				};

				realm.add(TileEntity::create<ItemSpawner>(game, Position(row, column), 0.0002f, std::move(mushrooms)));
			}
		}
	}
}
