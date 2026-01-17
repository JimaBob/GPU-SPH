#version 430

struct Particle {
    vec2 pos;
    vec2 vel;
    float density;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

uniform vec2 screenSize;
uniform float size;

out flat int vel;

void main() {
    uint id = uint(gl_VertexID);
    vel = int(length(particles[id].vel));
    
    // Transform from pixel coordinates to clip space
    vec2 pos = (particles[id].pos/screenSize) * 2.0 - vec2(1.0);
    
    gl_Position = vec4(pos, 0.0, 1.0);
    gl_PointSize = float(size);
}