#ifndef hAsteroid_included
#define hAsteroid_included

#include <osg\Vec3>

class Asteroid
{
public:
	Asteroid(osg::Vec3f position, float radius);
	osg::Vec3f getPosition();
	float getRadius();
private:
	osg::Vec3f position;
	float radius;
};

#endif//hAsteroid_included ndef
