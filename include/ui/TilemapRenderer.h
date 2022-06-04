#pragma once

#include <memory>

#include <nanogui/common.h>

#include "Tilemap.h"

namespace Game3 {
	class TilemapRenderer {
		public:
			constexpr static float tileTextureSize = 1 / 10.f;
			constexpr static float tileTexturePadding = 1 / 2048.f;
			float scale = 2.f;
			int backbufferWidth = -1;
			int backbufferHeight = -1;

			nanogui::Vector2f center {0.f, 0.f};
			std::shared_ptr<Tilemap> tilemap;

			TilemapRenderer() = default;
			virtual ~TilemapRenderer() = default;

			virtual void initialize(const std::shared_ptr<Tilemap> &) = 0;
			virtual void render() = 0;
			virtual void onBackbufferResized(int width, int height);

		protected:
			static void check(int handle, bool is_link = false);
	};
}
