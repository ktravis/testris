#version 300 es
precision highp float;

/* based on https://www.shadertoy.com/view/Ms3XWH */

in vec2 texcoord;

uniform vec4 tint;
uniform sampler2D tex;
uniform float time;

out vec4 fragmentColor;

const float range = 0.05;
const float noiseQuality = 250.0;
const float noiseIntensity = 0.0088;
const float offsetIntensity = 0.02;
const float colorOffsetIntensity = 1.3;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float verticalBar(float pos, float uvY, float offset) {
    float edge0 = (pos - range);
    float edge1 = (pos + range);

    float x = smoothstep(edge0, pos, uvY) * offset;
    x -= smoothstep(pos, edge1, uvY) * offset;
    return x;
}

void main() {
	vec2 uv = texcoord;
    
    for (float i = 0.0; i < 0.71; i += 0.1313)
    {
        float d = mod(time * i, 1.7);
        float o = sin(1.0 - tan(time * 0.24 * i));
    	o *= offsetIntensity;
        uv.x += verticalBar(d, uv.y, o);
    }
    
    float uvY = uv.y;
    uvY *= noiseQuality;
    uvY = float(int(uvY)) * (1.0 / noiseQuality);
    float noise = rand(vec2(time * 0.00001, uvY));
    uv.x += noise * noiseIntensity;

    vec2 offsetR = vec2(0.006 * sin(time), 0.0) * colorOffsetIntensity;
    vec2 offsetG = vec2(0.0073 * (cos(time * 0.97)), 0.0) * colorOffsetIntensity;
    
    float r = texture(tex, uv + offsetR).r;
    float g = texture(tex, uv + offsetG).g;
    float b = texture(tex, uv).b;

    vec4 t = vec4(r, g, b, 1.0);
    fragmentColor = t;
}
