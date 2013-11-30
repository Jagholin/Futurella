#include "Asteroid.h"

Asteroid::Asteroid(osg::Vec3f pos, float r){
	position = pos;
	radius = r;
}

osg::Vec3f 
Asteroid::getPosition(){
	return position;
}

float 
Asteroid::getRadius(){
	return radius;
}
