#pragma once
#include <btBulletDynamicsCommon.h>

class ShipPhysicsActor : public btActionInterface
{
public:
    explicit ShipPhysicsActor(btRigidBody* slave);
    virtual ~ShipPhysicsActor();

    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep);

    virtual void debugDraw(btIDebugDraw* debugDrawer);
    
protected:
    btRigidBody* m_slaveBody;
};
