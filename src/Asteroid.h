#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
using namespace Magnum;

class Asteroid
{
public:
    Asteroid(Vector3 position, float radius);
    Vector3 getPosition();
    float getRadius();
private:
    Vector3 m_position;
    float m_radius;
};

