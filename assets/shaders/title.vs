#version 300 es
precision highp float;

in vec4 position;
in vec2 texCoord;

uniform float time;
uniform mat4 modelview;
uniform mat4 proj;
uniform vec2 mouse;

out vec2 texcoord;
out vec2 screenPos;

void main() {
   texcoord = texCoord;
   vec4 pos = modelview * position;
   pos.y += 16.0 * sin((time/12.0 + pos.x)/50.0);
   vec2 d = pos.xy - mouse;
   float len = length(d);
   pos.xy += min(8.0, 500.0 / len) * normalize(d);
   screenPos = pos.xy;
   gl_Position = proj * pos;
}
