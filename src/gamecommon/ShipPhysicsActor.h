#pragma once
#include <btBulletDynamicsCommon.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
using namespace Magnum;

class ShipPhysicsActor : public btActionInterface
{
public:
    explicit ShipPhysicsActor(btRigidBody* slave);
    virtual ~ShipPhysicsActor();

    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);

    virtual void debugDraw(btIDebugDraw* debugDrawer);

    void setForceVector(const Vector3&);
    void setRotationAxis(const Vector3&);
    
protected:
    btRigidBody* m_slaveBody;

    btVector3 m_forceVec, m_torqueVec;
};
