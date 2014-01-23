#include "SpaceShip.h"
#include <osg/ShapeDrawable>

BEGIN_DECLNETMESSAGE(ShipStateData, 5101)
osg::Vec3f position;
osg::Vec4f orientation;
osg::Vec3f velocity;
END_DECLNETMESSAGE()

BEGIN_NETTORAWMESSAGE_QCONVERT(ShipStateData)
outStr << position << orientation << velocity;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(ShipStateData)
inStr >> temp->position >> temp->orientation >> temp->velocity;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(SpaceShipConstructionData)
outStr << pos << orient;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(SpaceShipConstructionData)
inStr >> temp->pos >> temp->orient;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(ShipStateData);
REGISTER_NETMESSAGE(SpaceShipConstructionData);

SpaceShip::SpaceShip(osg::Vec3f pos, osg::Vec4f orient, bool hostSide /* = false */):
position(pos), orientation(orient), m_hostSide(hostSide)
{
	for (int i = 0; i < inputTypesNumber; i++) inputState[i] = false; //No keys pressed yet
    osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0, 0, 0), 0.1f, 0.1f, 0.3f);
    osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
    s->setColor(osg::Vec4(1, 1, 1, 1));
    osg::ref_ptr<osg::Geode> d = new osg::Geode();
    d->addDrawable(s);

    m_shipNode = d;
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->setMatrix(osg::Matrix::translate(0, 0, 0));
    m_transformGroup->addChild(m_shipNode);
}

SpaceShip::SpaceShip(osg::Node* shipNode):
m_shipNode(shipNode), m_hostSide(true)
{
	for (int i = 0; i < inputTypesNumber; i++) inputState[i] = false; // No keys pressed yet
    if (!m_shipNode)
    {
        osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0, 0, 0), 0.1f, 0.1f, 0.3f);
        osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
        s->setColor(osg::Vec4(1, 1, 1, 1));
        osg::ref_ptr<osg::Geode> d = new osg::Geode();
        d->addDrawable(s);

        m_shipNode = d;
    }
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->setMatrix(osg::Matrix::translate(0, 0, 0));
    m_transformGroup->addChild(m_shipNode);

	velocity = osg::Vec3f(0, 0, 0);
	setPosition(osg::Vec3f(0, 0, 0));
	setOrientation(osg::Quat(0, osg::Vec3f(1, 0, 0)));
	acceleration = 4;
	steerability = 2;
	friction = 0.7f;
}

SpaceShip::~SpaceShip()
{
    osg::Node::ParentList parents = m_transformGroup->getParents();

    for (osg::Group* parent : parents)
        parent->removeChild(m_transformGroup);
}

void SpaceShip::setPosition(osg::Vec3f p){
	position = p;
}

void SpaceShip::setOrientation(osg::Quat o){
	orientation = o;
}

void SpaceShip::setInput(inputType t, bool state){
	inputState[t] = state;
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

osg::MatrixTransform* SpaceShip::getTransformGroup()
{
    return m_transformGroup.get();
}

void SpaceShip::update(float dt){
	//position update
	position += velocity*dt;

	//Friction
	velocity *= pow(friction, dt);

	//handle input
	if (inputState[ACCELERATE] != inputState[BACK])
	{
		if (inputState[ACCELERATE])
			velocity += orientation*osg::Vec3f(0, 0, -acceleration * dt);
		else
			velocity += orientation*osg::Vec3f(0, 0, acceleration * dt);
	}

	if (inputState[LEFT] != inputState[RIGHT]){
		float amount = steerability * dt;
		if (inputState[LEFT]){
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
	if (inputState[UP] != inputState[DOWN]){
		float amount = steerability * dt;
		if (inputState[UP]){
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

	

    m_transformGroup->setMatrix(osg::Matrix::translate(getCenter()));
    osg::Matrix rotationMatrix;
    getOrientation().get(rotationMatrix);
    m_transformGroup->preMult(rotationMatrix);
}

bool SpaceShip::takeMessage(const NetMessage::const_pointer& msg, MessagePeer*)
{
    if (!m_hostSide && msg->gettype() == NetShipStateDataMessage::type)
    {
        NetShipStateDataMessage::const_pointer realMsg = msg->as<NetShipStateDataMessage>();

        // set position, rotation and velocity.
        position = realMsg->position;
        velocity = realMsg->velocity;
        orientation.set(realMsg->orientation);

        update(0);
        return true;
    }
    return false;
}

void SpaceShip::setHostSide(bool hSide)
{
    m_hostSide = hSide;
}

NetSpaceShipConstructionDataMessage::pointer SpaceShip::createConstructorMessage() const
{
    if (!m_hostSide)
        return nullptr;

    NetSpaceShipConstructionDataMessage::pointer result(new NetSpaceShipConstructionDataMessage);
    result->orient = orientation.asVec4();
    result->pos = position;
    return result;
}

