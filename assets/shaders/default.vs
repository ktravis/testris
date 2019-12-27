#version 140

in vec4 position;
in vec4 c;
in vec2 tc;

uniform mat4 proj;
uniform mat4 model;

out vec4 color;
out vec2 texcoord;

void main() {
   color = c;
   texcoord = tc;
   gl_Position = proj * model * position;
}
