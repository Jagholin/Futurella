#include "SpaceShip.h"

SpaceShip::SpaceShip()
{
	velocity = osg::Vec3f(0, 0, 0);
	setPosition(osg::Vec3f(0, 0, 0));
	setOrientation(osg::Quat(0, osg::Vec3f(1, 0, 0)));
	stateBack = stateAcc = stateLeft = stateRight = stateDown = stateUp = false;
	acceleration = 2;
	steerability = 1;
}

void SpaceShip::setPosition(osg::Vec3f p){
	position = p;
}

void SpaceShip::setOrientation(osg::Quat o){
	orientation = o;
}

void SpaceShip::setAccelerate(bool state){
	stateAcc = state;
}

void SpaceShip::setLeft(bool state){
	stateLeft = state;
}

void SpaceShip::setRight(bool state){
	stateRight = state;
}

void SpaceShip::setUp(bool state){
	stateUp = state;
}

void SpaceShip::setDown(bool state){
	stateDown = state;
}

void SpaceShip::setBack(bool state){
    stateBack = state;
}

osg::Vec3f SpaceShip::getCenter(){
	return position;
}

float SpaceShip::getSpeed(){
	return velocity.length();
}

osg::Vec3f SpaceShip::getVelocity(){
	return velocity;
}

osg::Quat SpaceShip::getOrientation(){
	return orientation;
}

void SpaceShip::update(float dt){

	position += velocity*dt;

	if (stateAcc != stateBack)
	{
		if (stateAcc)
			velocity += orientation*osg::Vec3f(0, 0, -acceleration * dt);
		else
			velocity += orientation*osg::Vec3f(0, 0, acceleration * dt);
	}

	if (stateLeft != stateRight){
		float amount = steerability * dt;
		if (stateLeft){
			//roll left
			osg::Quat q(amount, osg::Vec3f(0, 0, 1));
			orientation = q * orientation;
		}
		else{
			//roll right
			osg::Quat q(-amount, osg::Vec3f(0, 0, 1));
			orientation = q * orientation;
		}
	}
	if (stateUp != stateDown){
		float amount = steerability * dt;
		if (stateUp){
			//rear up
			osg::Quat q(-amount, osg::Vec3f(1, 0, 0));
			orientation = q * orientation;
		}
		else{
			//rear down
			osg::Quat q(amount, osg::Vec3f(1, 0, 0));
			orientation = q * orientation;
		}
	}
}

