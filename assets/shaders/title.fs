#version 300 es
precision highp float;

in vec2 texcoord;
in vec2 screenPos;

uniform vec4 tint;
uniform sampler2D tex;
uniform float time;

out vec4 fragmentColor;

void main() {
    vec4 t = tint;
    t.x = 0.8 * cos((screenPos.x + screenPos.y)/400.0 + time/700.0) + 0.2;
    t.y = 1.0 - 0.8 * cos((screenPos.x + screenPos.y)/250.0 + time/700.0);
    t.z = 0.8;
    fragmentColor = t * texture(tex, texcoord);
}
