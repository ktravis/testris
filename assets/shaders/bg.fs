#version 300 es
precision highp float;

in vec2 texcoord;
in vec2 screenPos;

uniform vec4 tint;
uniform sampler2D tex;
uniform float time;
uniform vec2 dim;

out vec4 fragmentColor;

void main() {
    float c = 50.0;
    float t = mod(step(c, mod(screenPos.y - time/8.0, 2.0*c)) + step(c, mod(screenPos.x - time/5.0, 2.0*c)), 2.0);
    float d = length(screenPos - dim/2.0);
    float a = 0.05 + 0.2 * smoothstep(length(dim/2.0), 0.0, d);// * smoothstep(0.0, 200.0, screenPos.y);
    fragmentColor = vec4(0.2, max(0.0, t*0.7), 0.8, a);
}

