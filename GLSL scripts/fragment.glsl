#version 430

uniform float max_speed;

in flat int vel;
out vec4 fragColor;

void main() {
    float d = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.75, 0.0, d);
    
    float value = 2*vel/max_speed;
    
    fragColor = vec4(value, 0.8, 1.0-value, alpha);
}