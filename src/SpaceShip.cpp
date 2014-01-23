#include "SpaceShip.h"
#include <osg/ShapeDrawable>

BEGIN_DECLNETMESSAGE(ShipStateData, 5101, false)
osg::Vec3f position;
osg::Vec4f orientation;
osg::Vec3f velocity;
END_DECLNETMESSAGE()

BEGIN_NETTORAWMESSAGE_QCONVERT(ShipStateData)
out << position << orientation << velocity;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(ShipStateData)
in >> position >> orientation >> velocity;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(SpaceShipConstructionData)
out << pos << orient;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(SpaceShipConstructionData)
in >> pos >> orient;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(ShipStateData);
REGISTER_NETMESSAGE(SpaceShipConstructionData);

SpaceShip::SpaceShip(osg::Vec3f pos, osg::Vec4f orient, bool hostSide /* = false */):
m_position(pos), m_orientation(orient), m_hostSide(hostSide)
{
	for (int i = 0; i < inputTypesNumber; i++) m_inputState[i] = false; //No keys pressed yet
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
	for (int i = 0; i < inputTypesNumber; i++) m_inputState[i] = false; // No keys pressed yet
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

    m_velocity = osg::Vec3f(0, 0, 0);
    setPosition(osg::Vec3f(0, 0, 0));
    setOrientation(osg::Quat(0, osg::Vec3f(1, 0, 0)));
    m_acceleration = 4;
	m_steerability = 2;
	m_friction = 0.7f;
}

SpaceShip::~SpaceShip()
{
    osg::Node::ParentList parents = m_transformGroup->getParents();

    for (osg::Group* parent : parents)
        parent->removeChild(m_transformGroup);
}

void SpaceShip::setPosition(osg::Vec3f p)
{
    m_position = p;
}

void SpaceShip::setOrientation(osg::Quat o)
{
    m_orientation = o;
}

void SpaceShip::setInput(inputType t, bool state){
	m_inputState[t] = state;
}

osg::Vec3f SpaceShip::getCenter()
{
    return m_position;
}

float SpaceShip::getSpeed()
{
    return m_velocity.length();
}

osg::Vec3f SpaceShip::getVelocity()
{
    return m_velocity;
}

osg::Quat SpaceShip::getOrientation()
{
    return m_orientation;
}

osg::MatrixTransform* SpaceShip::getTransformGroup()
{
    return m_transformGroup.get();
}


void SpaceShip::update(float dt){
	//position update
	m_position += m_velocity*dt;

	//Friction
	m_velocity *= pow(m_friction, dt);

	//handle input
	if (m_inputState[ACCELERATE] != m_inputState[BACK])
	{
		if (m_inputState[ACCELERATE])
			m_velocity += m_orientation*osg::Vec3f(0, 0, -m_acceleration * dt);
		else
			m_velocity += m_orientation*osg::Vec3f(0, 0, m_acceleration * dt);
	}

	if (m_inputState[LEFT] != m_inputState[RIGHT]){
		float amount = m_steerability * dt;
		if (m_inputState[LEFT]){
			//roll left
			osg::Quat q(amount, osg::Vec3f(0, 0, 1));
			m_orientation = q * m_orientation;
		}
		else{
			//roll right
			osg::Quat q(-amount, osg::Vec3f(0, 0, 1));
			m_orientation = q * m_orientation;
		}
	}
	if (m_inputState[UP] != m_inputState[DOWN]){
		float amount = m_steerability * dt;
		if (m_inputState[UP]){
			//rear up
			osg::Quat q(-amount, osg::Vec3f(1, 0, 0));
			m_orientation = q * m_orientation;
		}
		else{
			//rear down
			osg::Quat q(amount, osg::Vec3f(1, 0, 0));
			m_orientation = q * m_orientation;
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
        m_position = realMsg->position;
        m_velocity = realMsg->velocity;
        m_orientation.set(realMsg->orientation);

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
    result->orient = m_orientation.asVec4();
    result->pos = m_position;
    return result;
}

