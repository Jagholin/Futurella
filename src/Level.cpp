#include "Level.h"
#include "AsteroidField.h"
#include "Asteroid.h"
#include <random>
#include <cmath>

Level::Level(int asteroidNumber, float turbulence, float density)
{
	std::random_device randDevice;

//Generate Asteroid Field
	//step 1: scatter asteroids. save to AsteroidField
	AsteroidField *scatteredAsteroids = new AsteroidField();
	float astSizeDif = astMaxSize - astMinSize;
	float asteroidSpaceCubeSidelength = astMaxSize * (2 + 2 - density * 2);
	float asteroidFieldSpaceCubeSideLength = pow(pow(asteroidSpaceCubeSidelength, 3)*asteroidNumber, 0.333f);
	float radius;
	osg::Vec3f pos;
	for (int i = 0; i < asteroidNumber; i++) 
	{
		pos.set(
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
			randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max());
		radius = (randDevice()*astSizeDif / randDevice.max() + astMinSize)*0.5f;
		scatteredAsteroids->addAsteroid(pos, radius);
	}

	//step 2: move asteroids to avoid overlappings (heuristic). save to Level::asteroidField
	asteroids = new AsteroidField();
	for (int i = 0; i < asteroidNumber; i++) 
	{
		float favoredDistance = (asteroidSpaceCubeSidelength - astMaxSize) / 2 + scatteredAsteroids->getAsteroid(i)->getRadius();
		float boundingBoxRadius = favoredDistance + astMaxSize/2;
		std::list<Asteroid*> candidates = scatteredAsteroids->getAsteroidsInBlock(scatteredAsteroids->getAsteroid(i)->getPosition(), osg::Vec3f(boundingBoxRadius, boundingBoxRadius, boundingBoxRadius));
		osg::Vec3f shift(0,0,0);
		int numberOfCloseAsteroids = 0;
		for (std::list<Asteroid*>::iterator it = candidates.begin(); it != candidates.end(); it++) {
			if (*it != scatteredAsteroids->getAsteroid(i)) {
				osg::Vec3f d = scatteredAsteroids->getAsteroid(i)->getPosition() - (*it)->getPosition();
				float scale = -((d.length() - (*it)->getRadius()) - favoredDistance);
				if (scale > 0) {
					d.normalize();
					shift += shift + (d * scale *0.5f); //push away from other close asteroids
					numberOfCloseAsteroids++;
				}
			}
		}
		asteroids->addAsteroid(scatteredAsteroids->getAsteroid(i)->getPosition() + shift, scatteredAsteroids->getAsteroid(i)->getRadius());
		
	}
	delete scatteredAsteroids;

};

Level::~Level()
{
	delete asteroids;
}

Asteroid*
Level::getAsteroid(int id)
{
	return asteroids->getAsteroid(id);
}

int
Level::getAsteroidLength()
{
	return asteroids->getLength();
}