#include "Asteroid.h"

Asteroid::Asteroid(osg::Vec3f pos, float r)
{
    m_position = pos;
    m_radius = r;
}

osg::Vec3f
Asteroid::getPosition()
{
    return m_position;
}

float
Asteroid::getRadius()
{
    return m_radius;
}
