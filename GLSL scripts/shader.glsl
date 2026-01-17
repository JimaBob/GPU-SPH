#version 430

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Particle {
    vec2 pos;
    vec2 vel;
    float density;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

uniform float viscocity;
uniform int num_particles;
uniform float mass; // usually just 1
uniform float particle_size;
uniform float smoothing_radius; // radius of smoothing kernel
uniform vec2 force; // usually just gravity
uniform float timeStep; // dt
uniform float max_speed;
uniform float rest_density; // Target density
uniform float gas_constant; // Force coefficient 
uniform float damping; // How much is lost after wall reounds
uniform int width;
uniform int height;

const float PI = 3.14159265358979323846;

uint gid() {
    return gl_GlobalInvocationID.x;
}

float viscosity_laplacian(float r, float visc_const) {
    if (r >= smoothing_radius) return 0.f;
    return visc_const * (smoothing_radius - r);
}

vec2 poly6_grad(vec2 dp, float r, float C6, float hh) {
    if (r >= smoothing_radius || r == 0.f) return vec2(0.0);
    float val = hh - r*r;
    float scale = C6 * val * val;
    return scale * dp;
}

float poly6(float r, float C, float hh) {
    if (r >= smoothing_radius) return 0.0;
    float val = hh - r*r;
    return C * val * val * val;
}

float density_to_pressure(float density) {
    return gas_constant * (density - rest_density);
}

void handle_boundaries(uint id) {
    const float boundary_damping = 0.8;
    
    // Left boundary
    if (particles[id].pos.x - particle_size < 0.0) {
        particles[id].pos.x = particle_size;
        particles[id].vel.x = abs(particles[id].vel.x) * boundary_damping;
    }
    // Right boundary
    if (particles[id].pos.x + particle_size > width) {
        particles[id].pos.x = width - particle_size;
        particles[id].vel.x = -abs(particles[id].vel.x) * boundary_damping;
    }
    // Bottom boundary
    if (particles[id].pos.y - particle_size < 0.0) {
        particles[id].pos.y = particle_size;
        particles[id].vel.y = abs(particles[id].vel.y) * boundary_damping;
    }
    // Top boundary
    if (particles[id].pos.y + particle_size > height) {
        particles[id].pos.y = height - particle_size;
        particles[id].vel.y = -abs(particles[id].vel.y) * boundary_damping;
    }
}

void handle_particle_collisions(uint id) {
    for (int i = 0; i < num_particles; i++) {
        if (i == id) continue;
        
        vec2 dp = particles[i].pos - particles[id].pos;
        float r = length(dp);
        float min_dist = 2.0 * particle_size;
        
        if (r < min_dist && r > 0.0001) {
            // Normalize direction
            vec2 n = dp / r;
            
            // Overlap amount
            float overlap = min_dist - r;
            float separation = overlap * 0.5 + 0.01;
            
            // Separate particles
            particles[id].pos -= n * separation;
            particles[i].pos += n * separation;
            
            // Relative velocity
            vec2 dv = particles[i].vel - particles[id].vel;
            float vel_dot = dot(dv, n);
            
            // Only apply impulse if particles are moving toward each other
            if (vel_dot < 0.0) {
                float impulse = vel_dot * 0.5;
                particles[id].vel += impulse * n;
                particles[i].vel -= impulse * n;
            }
        }
    }
}

void main() {
    uint id = gid();
    if (id >= num_particles) return;

    const float C = 4.0/(PI * pow(smoothing_radius, 8));
    const float C6 = -6*C;
    const float hh = smoothing_radius*smoothing_radius;
    const float visc_const = (45.0f / (PI * pow(smoothing_radius, 6)));

    // Compute density
    float rho = 0.0;
    for (int i = 0; i < num_particles; i++) {
        float r = length(particles[id].pos - particles[i].pos);
        rho += mass * poly6(r, C, hh);
    }
    particles[id].density = max(rho, 0.000001);
    float P = density_to_pressure(particles[id].density);

    // Compute pressure

    vec2 f = vec2(0.0);
    vec2 visc = vec2(0.0);

    for (int i = 0; i < num_particles; i++) { // Needs neighbourhood search still
        if (i == id) continue;

        vec2 dp = particles[i].pos - particles[id].pos;
        float r = length(dp);

        if (r < 0.0001) r = 0.0001;
        if (r >= smoothing_radius || r == 0.0) continue;

        float rhoi = max(particles[i].density, 0.000001);
        float Pi = density_to_pressure(rhoi);
        vec2 grad = poly6_grad(dp, r, C6, hh);

        float coeff = -mass * (P + Pi) / (2.0 * rhoi);
        f += coeff * grad;

        visc += (particles[i].vel - particles[id].vel) * viscosity_laplacian(r, visc_const) / rhoi;
    }

    // Apply forces/accelerate
    f += viscocity * mass * visc;
    particles[id].vel += (f / particles[id].density + force / mass) * timeStep;

    float speed = length(particles[id].vel);
    if (speed > max_speed) {
        particles[id].vel = (particles[id].vel / speed) * max_speed;
    }

    // Move and process boundaries

    particles[id].pos += particles[id].vel * timeStep;
    handle_boundaries(id);

    // also like mouse stuff and any necessary interactions
}