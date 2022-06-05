#pragma once

#include <random>

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/glcanvas.h>

#include "Image.h"
#include "Texture.h"
#include "Types.h"
#include "ui/RectangleRenderer.h"
#include "ui/SpriteRenderer.h"

namespace Game3 {
	class Game;

	class Canvas: public nanogui::GLCanvas {
		public:
			constexpr static float DEFAULT_SCALE = 2.f;

			std::shared_ptr<Game> game;
			nanogui::Vector2f center {0.f, 0.f};
			float scale = DEFAULT_SCALE;
			SpriteRenderer spriteRenderer {*this};
			RectangleRenderer rectangleRenderer {*this};
			float magic = 8.f;

			Canvas(nanogui::Widget *parent);

			void draw(NVGcontext *) override;
			void drawGL() override;
			bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
			bool mouseDragEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
			bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

		private:
			constexpr static float HEADER_HEIGHT = 56.f;

			NVGcontext *context = nullptr;
			int font = -1;
	};
}
