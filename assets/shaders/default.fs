#version 300 es
precision highp float;

in vec2 texcoord;

uniform vec4 tint;
uniform sampler2D tex;

out vec4 fragmentColor;

void main() {
   fragmentColor = tint * texture(tex, texcoord);
}
