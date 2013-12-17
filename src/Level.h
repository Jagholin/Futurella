#ifndef hLevel_included
#define hLevel_included

#include <list>
#include <vector>

class AsteroidField;

class Asteroid;

class Level //level information
{
public:
	Level(int numberOfAsteroids, float turbulence, float density); //generates level
	~Level();

	int getAsteroidLength();
	Asteroid* getAsteroid(int asteroidId);

private:
	const float astMinSize = 0.2f, astMaxSize = 1.0f;
	float asteroidSpaceCubeSidelength;
	AsteroidField *asteroids;
	
};

#endif//hLevel_included ndef
