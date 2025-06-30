#version 330 core

in vec4 position;
in vec4 color_in;

out vec4 vert_color;

void main() {
	gl_Position = position;
	vert_color = color_in;
}