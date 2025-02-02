#version 430 core

// Credit: https://github.com/davudk/OpenGL-TileMap-Demos/blob/master/Resources/BufferedRenderer.vert

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aIndex;

uniform mat4 projection;

out vec2 texCoord;
flat out int index;

void main() {
	gl_Position = projection * vec4(aPosition, 0.0, 1.0);
	texCoord = aTexCoord;
	index = int(aIndex);
}
