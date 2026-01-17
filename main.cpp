#include <vector>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <random>

const int height = 600;
const int width = 600;
const int screen_width = width;
const int screen_height = height;
unsigned int num_particles = 1024;
const unsigned int WORKGROUP_SIZE = 256;

float viscocity = 10.0f;
const float mass = 1.0f;
const float smoothing_radius = 16.f;
const float gravity = 200.0f;
const float timeStep = 0.005f;
const int stepsPerRender = 1;
const float max_speed = 200.0f;
const float size = 20;

const float rest_density = 8.0f;
const float gas_constant = 200.0f;
const float damping = 0.6f;

std::string LoadGLSL(const std::string& filename)
{
    std::ifstream file("GLSL scripts/" + filename);
    if (!file.is_open())
        throw std::runtime_error("Failed to open GLSL file: " + filename);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string compute = LoadGLSL("shader.glsl");
std::string vertex  = LoadGLSL("vertex.glsl");
std::string fragment  = LoadGLSL("fragment.glsl");

static GLuint compileShader(GLenum stage, const char* src) {
    GLuint s = glCreateShader(stage);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        char log[8192]; GLsizei len=0;
        glGetShaderInfoLog(s, sizeof(log), &len, log);
        std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        std::exit(EXIT_FAILURE);
    }
    return s;
}

static GLuint linkProgram(const std::vector<GLuint>& shaders) {
    GLuint prog = glCreateProgram();
    for(auto s : shaders) glAttachShader(prog, s);
    glLinkProgram(prog);
    GLint ok=0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if(!ok) {
        char log[8192]; GLsizei len=0;
        glGetProgramInfoLog(prog, sizeof(log), &len, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
        std::exit(EXIT_FAILURE);
    }
    return prog;
}

struct Particle {
    float px, py;
    float vx = 0.0f, vy = 0.0f;
    float density;
};

int main() {
    if(!glfwInit()) {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "He he he, simulation", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "Compiling Shader" << std::endl;
    GLuint cs = compileShader(GL_COMPUTE_SHADER, compute.c_str());
    GLuint shaderProg = linkProgram({cs});
    glDeleteShader(cs);

    std::cout << "Finished compiling shader,  compiling vertex shader" << std::endl;
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex.c_str());
    std::cout << "Finished compiling vertex shader,  compiling fragment shader" << std::endl;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment.c_str());
    std::cout << "Finished compiling fragment shader" << std::endl;
    GLuint renderProg = linkProgram({vs, fs});
    glDeleteShader(vs); glDeleteShader(fs);

    // Particles Setup
    std::vector<Particle> particles;
    particles.resize(num_particles);
    
    for (int i = 0; i < num_particles; i++) {
        particles[i].px = rand() % width;
        particles[i].py = rand() % height;
        particles[i].vx = 0;
        particles[i].vy = 0;
        particles[i].density = 0;
    }

    GLuint pssbo;
    glGenBuffers(1, &pssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Get uniform locations:
    GLint viscocityLocShader = glGetUniformLocation(shaderProg, "viscocity");
    GLint num_particlesLocShader = glGetUniformLocation(shaderProg, "num_particles");
    GLint massLocShader = glGetUniformLocation(shaderProg, "mass");
    GLint particle_sizeLocShader = glGetUniformLocation(shaderProg, "particle_size");
    GLint smoothing_radiusLocShader = glGetUniformLocation(shaderProg, "smoothing_radius");
    GLint forceLocShader = glGetUniformLocation(shaderProg, "force");
    GLint timeStepLocShader = glGetUniformLocation(shaderProg, "timeStep");
    GLint max_speedLocShader = glGetUniformLocation(shaderProg, "max_speed");
    GLint rest_densityLocShader = glGetUniformLocation(shaderProg, "rest_density");
    GLint gas_constantLocShader = glGetUniformLocation(shaderProg, "gas_constant");
    GLint dampingLocShader = glGetUniformLocation(shaderProg, "damping");
    GLint widthLocShader = glGetUniformLocation(shaderProg, "width");
    GLint heightLocShader = glGetUniformLocation(shaderProg, "height");

    GLint screenSizeLocRender = glGetUniformLocation(renderProg, "screenSize");
    GLint sizeLocRender = glGetUniformLocation(renderProg, "size");
    GLint max_speedLocRender = glGetUniformLocation(renderProg, "max_speed");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int frames = 0;

    while(!glfwWindowShouldClose(window)) {
        //_sleep(0); // Just for better viewing while debugging
        frames++;

        glUseProgram(shaderProg);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pssbo);

        // Set uniform values:
        glUniform1f(viscocityLocShader, viscocity);
        glUniform1i(num_particlesLocShader, num_particles);
        glUniform1f(massLocShader, mass);
        glUniform1f(particle_sizeLocShader, size);
        glUniform1f(smoothing_radiusLocShader, smoothing_radius);
        glUniform2f(forceLocShader, 0.0f, -gravity);
        glUniform1f(timeStepLocShader, timeStep);
        glUniform1f(max_speedLocShader, max_speed);
        glUniform1f(rest_densityLocShader, rest_density);
        glUniform1f(gas_constantLocShader, gas_constant);
        glUniform1f(dampingLocShader, damping);
        glUniform1i(widthLocShader, width);
        glUniform1i(heightLocShader, height);

        unsigned int groups = (num_particles + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(renderProg);
        glBindVertexArray(vao);
        glUniform2f(screenSizeLocRender, width, height);
        glUniform1f(sizeLocRender, size);
        glUniform1f(max_speedLocRender, max_speed);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pssbo);
        glDrawArrays(GL_POINTS, 0, num_particles);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    glDeleteProgram(shaderProg);
    glDeleteProgram(renderProg);
    glDeleteBuffers(1, &pssbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Maybe add a way to remove particles if they get too close to a well, likely just check if the distance is less than a given size.