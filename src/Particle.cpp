#include <iostream>
#include <vector>
#include <string>
#include "Particle.h"

Particle::Particle()
    : Particle(0, 0)
{
}

Particle::Particle(int w, int h)
    : px(w),
      py(h),
      vx(0),
      vy(0),
      density(0)
{
}
std::vector<Particle>
Particle::generate(
    int count,
    int width, int height)
{
    std::vector<Particle> ret;

    /* initialize random seed: */
    srand(0);
    // srand(time(NULL));

    for (int k = 0; k < count; k++)
        ret.emplace_back(
            rand() % width,
            rand() % height);

    return ret;
}

std::string Particle::text() const
{
    std::string ret;
    ret = std::to_string(px) +
          ", " + std::to_string(py) +
          " ";
    return ret;
}

void Particle::test()
{
    auto vParticles = generate(100, 600, 600);
    float xtotal = 0;
    for (auto &p : vParticles)
    {
        std::cout << p.text();
        xtotal += p.px;
    }
    std::cout << "\n";

    float xav = xtotal / 100;

    std::cout << "average x location: " << xav << "\n";

    // expect the average particle to be located near the center
    if( abs( xav - 300 )  > 50  )
        std::cout << "unit test failed\n";
}