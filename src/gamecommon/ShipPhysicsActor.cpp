#include "ShipPhysicsActor.h"

ShipPhysicsActor::ShipPhysicsActor(btRigidBody* slave)
{
    m_slaveBody = slave;
}

ShipPhysicsActor::~ShipPhysicsActor()
{
    // nop
}

void ShipPhysicsActor::updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
{
    //m_slaveBody->clearForces();
    btTransform trans;
    m_slaveBody->getMotionState()->getWorldTransform(trans);
    btQuaternion worldRot, worldRotInv, force(0, 0, -4.0f, 0);
    worldRot = trans.getRotation();
    worldRotInv = worldRot.inverse();
    force = worldRot * force * worldRotInv;
    m_slaveBody->applyCentralForce(btVector3(force.x(), force.y(), force.z()));
    m_slaveBody->applyTorque(btVector3(0, -0.1, 0));
    m_slaveBody->activate();
}

void ShipPhysicsActor::debugDraw(btIDebugDraw* debugDrawer)
{
    // nop
}
