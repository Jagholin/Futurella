#include "Level.h"
#include <random>
#include <cmath>


Level::Level(int asteroidNumber, float turbulence, float density)
{
	std::random_device randDevice;

	//Generate Asteroid Field
	asteroidsLength = asteroidNumber;
	asteroids = new Asteroid[asteroidNumber];


	//step 1: scatter asteroids
	float astMinSize = 0.2f, astMaxSize = 1.0f, astSizeDif = astMaxSize - astMinSize;
	float spaceCubeSideLength = pow( pow(astMaxSize,3)*asteroidNumber*(10 - density * 8), 0.333f);
	float posx, posy, posz;
	for (int i = 0; i < asteroidNumber; i++) 
	{
		posx = randDevice()*spaceCubeSideLength / randDevice.max();
		posy = randDevice()*spaceCubeSideLength / randDevice.max();
		posz = randDevice()*spaceCubeSideLength / randDevice.max();
		asteroids[i].position = osg::Vec3f(posx, posy, posz);
		asteroids[i].radius = (randDevice()*astSizeDif / randDevice.max() + astMinSize)*0.5f;
	}

	//TODO
	//step 2: move asteroids to remove overlappings
	

};

Level::~Level()
{
	delete[] asteroids;
};

Asteroid
Level::getAsteroid(int id)
{
	Asteroid ast = { asteroids[id].position, asteroids[id].radius };
	return ast;
};

int
Level::getAsteroidLength()
{
	return asteroidsLength;
};
