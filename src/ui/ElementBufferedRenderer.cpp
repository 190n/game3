#include <iostream>

#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "resources.h"
#include "Tiles.h"
#include "realm/Realm.h"
#include "ui/ElementBufferedRenderer.h"
#include "util/Util.h"

// Credit: https://github.com/davudk/OpenGL-TileMap-Demos/blob/master/Renderers/ElementBufferedRenderer.cs

namespace Game3 {
	ElementBufferedRenderer::ElementBufferedRenderer(): reshader(blur_frag) {}

	ElementBufferedRenderer::~ElementBufferedRenderer() {
		reset();
	}

	void ElementBufferedRenderer::reset() {
		if (initialized) {
			glDeleteVertexArrays(1, &vaoHandle);
			glDeleteBuffers(1, &eboHandle);
			glDeleteBuffers(1, &vboHandle);
			glDeleteProgram(shaderHandle);
			glDeleteTextures(1, &lfbTexture);
			glDeleteTextures(1, &lfbBlurredTexture);
			glDeleteSamplers(1, &sampler);
			glDeleteFramebuffers(1, &lfbHandle1);
			glDeleteFramebuffers(1, &lfbHandle2);
			tilemap.reset();
			initialized = false;
		}
	}

	void ElementBufferedRenderer::init(const TilemapPtr &tilemap_, const TileSet &tileset) {
		if (initialized)
			reset();
		tilemap = tilemap_;
		createShader();
		generateVertexBufferObject();
		generateElementBufferObject();
		generateVertexArrayObject();
		generateLightingFrameBuffer();
		generateSampler();
		const auto bright_shorts = tileset.getBright();
		brightTiles.assign(bright_shorts.begin(), bright_shorts.end());
		brightTiles.resize(8, -1);
		initialized = true;
	}

	void ElementBufferedRenderer::render(float divisor) {
		if (!initialized)
			return;
		tilemap->texture.bind();

		if (dirty) {
			recomputeLighting();
			dirty = false;
		}

		glActiveTexture(GL_TEXTURE1); CHECKGL
		glBindTexture(GL_TEXTURE_2D, lfbTexture); CHECKGL
		glBindVertexArray(vaoHandle); CHECKGL
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboHandle); CHECKGL

		glm::mat4 projection(1.f);
		projection = glm::scale(projection, {tilemap->tileSize, -tilemap->tileSize, 1}) *
		             glm::scale(projection, {scale / backbufferWidth, scale / backbufferHeight, 1}) *
		             glm::translate(projection, {center.x() - tilemap->width / 2.f, center.y() - tilemap->height / 2.f, 0});

		glUseProgram(shaderHandle); CHECKGL

		glUniform1i(glGetUniformLocation(shaderHandle, "texture0"), 0); CHECKGL
		glUniform1i(glGetUniformLocation(shaderHandle, "texture1"), 1); CHECKGL
		glUniformMatrix4fv(glGetUniformLocation(shaderHandle, "projection"), 1, GL_FALSE, glm::value_ptr(projection)); CHECKGL
		glUniform1f(glGetUniformLocation(shaderHandle,  "divisor"), divisor); CHECKGL
		glUniform1iv(glGetUniformLocation(shaderHandle, "bright_tiles"), brightTiles.size(), brightTiles.data()); CHECKGL
		glUniform1i(glGetUniformLocation(shaderHandle,  "tile_size"), static_cast<GLint>(tilemap->tileSize)); CHECKGL
		glUniform1i(glGetUniformLocation(shaderHandle,  "tileset_size"), static_cast<GLint>(tilemap->setWidth)); CHECKGL
		glUniform2f(glGetUniformLocation(shaderHandle,  "map_size"), static_cast<GLfloat>(tilemap->width), static_cast<GLfloat>(tilemap->height)); CHECKGL
		glDrawElements(GL_TRIANGLES, tilemap->tiles.size() * 6, GL_UNSIGNED_INT, (GLvoid *) 0); CHECKGL
	}

	void ElementBufferedRenderer::reupload() {
		glDeleteVertexArrays(1, &vaoHandle);
		glDeleteBuffers(1, &vboHandle);
		generateVertexBufferObject();
		generateVertexArrayObject();
	}

	bool ElementBufferedRenderer::onBackbufferResized(int width, int height) {
		if (TilemapRenderer::onBackbufferResized(width, height)) {
			generateLightingTexture();
			return true;
		}
		return false;
	}

	void ElementBufferedRenderer::createShader() {
		const GLchar *vert_ptr = reinterpret_cast<const GLchar *>(buffered_vert);
		int vert_handle = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert_handle, 1, &vert_ptr, reinterpret_cast<const GLint *>(&buffered_vert_len));
		glCompileShader(vert_handle);
		check(vert_handle);

		const GLchar *frag_ptr = reinterpret_cast<const GLchar *>(buffered_frag);
		int frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_handle, 1, &frag_ptr, reinterpret_cast<const GLint *>(&buffered_frag_len));
		glCompileShader(frag_handle);
		check(frag_handle);

		shaderHandle = glCreateProgram();
		glAttachShader(shaderHandle, vert_handle);
		glAttachShader(shaderHandle, frag_handle);
		glLinkProgram(shaderHandle);
		check(shaderHandle, true);

		glDetachShader(shaderHandle, vert_handle);
		glDeleteShader(vert_handle);

		glDetachShader(shaderHandle, frag_handle);
		glDeleteShader(frag_handle);
	}

	void ElementBufferedRenderer::generateVertexBufferObject() {
		glGenBuffers(1, &vboHandle);
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

		const size_t float_count = tilemap->tiles.size() * 20;
		auto vertex_data = std::make_unique<float[]>(float_count);
		size_t i = 0;

		const auto set_width = tilemap->setWidth / tilemap->tileSize;
		const float divisor = set_width;
		const float ty_size = 1.f / divisor - tileTexturePadding * 2;

		for (int x = 0; x < tilemap->width; ++x)
			for (int y = 0; y < tilemap->height; ++y) {
				const auto tile = (*tilemap)(x, y);
				const float tx0 = (tile % set_width) / divisor + tileTexturePadding;
				const float ty0 = (tile / set_width) / divisor + tileTexturePadding;

				constexpr int to_add = 5;

				// Vertex 0 (top left)
				vertex_data[i + 0] = x; // position x
				vertex_data[i + 1] = y; // position y
				vertex_data[i + 2] = tx0; // texcoord x
				vertex_data[i + 3] = ty0; // texcoord y
				vertex_data[i + 4] = static_cast<float>(tile);
				i += to_add;

				// Vertex 1 (top right)
				vertex_data[i + 0] = x + 1; // position x
				vertex_data[i + 1] = y;     // position y
				vertex_data[i + 2] = tx0 + ty_size; // texcoord x
				vertex_data[i + 3] = ty0;           // texcoord y
				vertex_data[i + 4] = static_cast<float>(tile);
				i += to_add;

				// Vertex 2 (bottom left)
				vertex_data[i + 0] = x;     // position x
				vertex_data[i + 1] = y + 1; // position y
				vertex_data[i + 2] = tx0;           // texcoord x
				vertex_data[i + 3] = ty0 + ty_size; // texcoord y
				vertex_data[i + 4] = static_cast<float>(tile);
				i += to_add;

				// Vertex 3 (bottom right)
				vertex_data[i + 0] = x + 1; // position x
				vertex_data[i + 1] = y + 1; // position y
				vertex_data[i + 2] = tx0 + ty_size; // texcoord x
				vertex_data[i + 3] = ty0 + ty_size; // texcoord y
				vertex_data[i + 4] = static_cast<float>(tile);
				i += to_add;
			}

		glBufferData(GL_ARRAY_BUFFER, float_count * sizeof(float), vertex_data.get(), GL_STATIC_DRAW);
	}

	void ElementBufferedRenderer::generateElementBufferObject() {
		glGenBuffers(1, &eboHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboHandle);

		size_t index_count = tilemap->tiles.size() * 6;
		auto indices = std::make_unique<unsigned[]>(index_count);

		unsigned i = 0, j = 0;
		for (int x = 0; x < tilemap->width; ++x) {
			for (int y = 0; y < tilemap->height; ++y) {
				indices[i + 0] = j;
				indices[i + 1] = j + 1;
				indices[i + 2] = j + 2;
				indices[i + 3] = j + 1;
				indices[i + 4] = j + 2;
				indices[i + 5] = j + 3;
				i += 6;
				j += 4;
			}
		}

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned), indices.get(), GL_STATIC_DRAW);
	}

	void ElementBufferedRenderer::generateVertexArrayObject() {
		glGenVertexArrays(1, &vaoHandle);

		glBindVertexArray(vaoHandle);
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid *) 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid *) (sizeof(float) * 2));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid *) (sizeof(float) * 4));
	}

	void ElementBufferedRenderer::generateLightingFrameBuffer() {
		glGenFramebuffers(1, &lfbHandle1);
		glGenFramebuffers(1, &lfbHandle2);
		generateLightingTexture();
	}

	void ElementBufferedRenderer::generateLightingTexture() {
		if (lfbTexture != 0)
			glDeleteTextures(1, &lfbTexture);

		if (lfbBlurredTexture != 0)
			glDeleteTextures(1, &lfbBlurredTexture);

		glGenTextures(1, &lfbTexture); CHECKGL
		glGenTextures(1, &lfbBlurredTexture); CHECKGL
		const auto width  = tilemap->tileSize * tilemap->width;
		const auto height = tilemap->tileSize * tilemap->height;

		glBindTexture(GL_TEXTURE_2D, lfbBlurredTexture); CHECKGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr); CHECKGL
		constexpr GLint filter = GL_NEAREST;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); CHECKGL
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, lfbTexture); CHECKGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr); CHECKGL
		rectangle.update(width, height);
		reshader.update(width, height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); CHECKGL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); CHECKGL
		glGenerateMipmap(GL_TEXTURE_2D);

	}

	void ElementBufferedRenderer::generateSampler() {
		if (sampler != 0)
			glDeleteSamplers(1, &sampler);
		glGenSamplers(1, &sampler);
	}

	void ElementBufferedRenderer::recomputeLighting() {
		if (!tilemap)
			return;

		GLint gtk_buffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &gtk_buffer); CHECKGL
		glBindFramebuffer(GL_FRAMEBUFFER, lfbHandle1); CHECKGL

		generateLightingTexture();

		GLint saved_viewport[4];
		glGetIntegerv(GL_VIEWPORT, saved_viewport);
		const auto tilesize = tilemap->tileSize;
		const auto width    = tilesize * tilemap->width;
		const auto height   = tilesize * tilemap->height;
		glViewport(0, 0, width, height); CHECKGL

		glDrawBuffer(GL_COLOR_ATTACHMENT0); CHECKGL

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lfbTexture, 0); CHECKGL

		// Clearing to half-white because the color in the lightmap will be multiplied by two
		glClearColor(.5f, .5f, .5f, 0.f); CHECKGL
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECKGL

		for (Index row = 0; row < tilemap->height; ++row) {
			for (Index column = 0; column < tilemap->width; ++column) {
				const Position pos(row, column);
				const auto tile = (*tilemap)(pos);
				if (tile == Monomap::LAVA) {
					const float x = column * tilesize;
					const float y = row * tilesize;
					const float radius = 1.5f;
					rectangle.drawOnScreen({1.f, .5f, 0.f, .5f}, x - radius * tilesize, y - radius * tilesize, (2.f * radius + 1.f) * tilesize, (2.f * radius + 1.f) * tilesize);
				}
			}
		}

		reshader.bind();
		reshader.set("xs", static_cast<float>(width));
		reshader.set("ys", static_cast<float>(height));
		reshader.set("r", 5.f);

		for (int i = 0; i < 16; ++i) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lfbBlurredTexture, 0); CHECKGL
			glDrawBuffer(GL_COLOR_ATTACHMENT0); CHECKGL
			reshader.set("axis", 0);
			reshader(lfbTexture);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lfbTexture, 0); CHECKGL
			glDrawBuffer(GL_COLOR_ATTACHMENT0); CHECKGL
			reshader.set("axis", 1);
			reshader(lfbBlurredTexture);
		}

		for (Index row = 0; row < tilemap->height; ++row) {
			for (Index column = 0; column < tilemap->width; ++column) {
				const Position pos(row, column);
				const auto tile = (*tilemap)(pos);
				if (tile == Monomap::LAVA) {
					const float x = column * tilesize;
					const float y = row * tilesize;
					rectangle.drawOnScreen({1.f, 1.f, 1.f, 1.f}, x, y, tilesize, tilesize);
				}
			}
		}

		glViewport(saved_viewport[0], saved_viewport[1], static_cast<GLsizei>(saved_viewport[2]), static_cast<GLsizei>(saved_viewport[3]));
		glBindFramebuffer(GL_FRAMEBUFFER, gtk_buffer); CHECKGL
	}
}
