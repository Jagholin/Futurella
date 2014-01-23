#pragma once
#include <osg/Vec3>

class Asteroid
{
public:
    Asteroid(osg::Vec3f position, float radius);
    osg::Vec3f getPosition();
    float getRadius();
private:
    osg::Vec3f m_position;
    float m_radius;
};

