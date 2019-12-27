#version 140

varying vec4 color;
varying vec2 texcoord;

uniform vec4 tint;
uniform sampler2D tex;

void main() {
   gl_FragData[0] = tint * color * texture(tex, texcoord);
}
