#ifndef hSpaceShip_included
#define hSpaceShip_included

#include <osg\Group>

class SpaceShip
{
public:
	SpaceShip();

	void setPosition(osg::Vec3f position);
	void setOrientation(osg::Quat rotation);

	void setAccelerate(bool state);
	void setLeft(bool state);
	void setRight(bool state);
	void setUp(bool state);
	void setDown(bool state);
    void setBack(bool state);

	void update(float deltaTime);

	osg::Vec3f getCenter();
	float getSpeed();
	osg::Vec3f getVelocity();
	osg::Quat getOrientation();
private:
	osg::Vec3f position, velocity;
	osg::Quat orientation;

	bool stateBack, stateAcc, stateLeft, stateRight, stateUp, stateDown;

	float acceleration, steerability;

};

#endif//hSpaceShip_included