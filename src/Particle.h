struct Particle
{
    float px, py;
    float vx = 0.0f, vy = 0.0f;
    float density;

    Particle();

    Particle(int w, int h);

    static
    std::vector<Particle>
    generate(
        int count,
        int w, int h);

    std::string text() const;

    void test();
};
