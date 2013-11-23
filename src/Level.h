#ifndef level_h_included
#define level_h_included

#include <osg\Vec3>

struct Asteroid
{
	osg::Vec3f position;
	float radius;
};

class Level //level information
{
public:
	Level(int numberOfAsteroids, float turbulence, float density); //generates level
	~Level();

	int getAsteroidLength();
	Asteroid getAsteroid(int asteroidId);

private:
	int asteroidsLength;
	Asteroid* asteroids;

};

#endif // !levelgenerator_h_included
