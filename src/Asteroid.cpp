#include "Asteroid.h"

Asteroid::Asteroid(Vector3 pos, float r)
{
    m_position = pos;
    m_radius = r;
}

Vector3
Asteroid::getPosition()
{
    return m_position;
}

float
Asteroid::getRadius()
{
    return m_radius;
}
