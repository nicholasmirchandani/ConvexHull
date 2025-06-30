#version 330 core

out vec4 frag_color;

void main() {
	// Everything on the line should be Green!
	frag_color = vec4(55.0 / 256.0, 135.0 / 256.0, 55.0 / 256.0, 1.0);
}