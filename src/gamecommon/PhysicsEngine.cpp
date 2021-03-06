#include "PhysicsEngine.h"
#include "ShipPhysicsActor.h"

#include <OpenThreads/Thread>
#include <cassert>
#include <iostream>

class VehicleMotionState : public btMotionState
{
protected:
    PhysicsEngine* m_engine;
    unsigned int m_vehicle;
    btTransform m_currTransform;

    volatile bool m_transformManuallyChanged;
public:
    VehicleMotionState(PhysicsEngine* eng, unsigned int vehicleId, btTransform initTransform)
    {
        m_engine = eng;
        m_vehicle = vehicleId;
        m_currTransform = initTransform;
        m_transformManuallyChanged = false;
    }

    ~VehicleMotionState()
    {
        std::cerr << "~VehicleMotionState\n";
    }

    void setPosition(osg::Vec3f* a)
    {
        m_currTransform.setOrigin(btVector3(a->x(), a->y(), a->z()));
        m_transformManuallyChanged = true;
    }

    void getWorldTransform(btTransform& worldTrans) const override
    {

        worldTrans = m_currTransform;
    }

    void setWorldTransform(const btTransform& worldTrans) override
    {
        if (m_transformManuallyChanged)
            m_transformManuallyChanged = false;
        else
          m_currTransform = worldTrans;
        if (m_engine->m_motionCallbacks.count(m_vehicle) > 0)
        {
            btVector3 transl = worldTrans.getOrigin();
            btQuaternion rot = worldTrans.getRotation();

            m_engine->m_motionCallbacks[m_vehicle]
                (osg::Vec3f(transl.x(), transl.y(), transl.z()), osg::Quat(rot.x(), rot.y(), rot.z(), rot.w()));
        }
    }
};

PhysicsEngine::PhysicsEngine() :
m_collisionConfig(new btDefaultCollisionConfiguration),
m_collisionDispatcher(new btCollisionDispatcher(m_collisionConfig)),
m_broadphase(new btDbvtBroadphase),
m_constraintSolver(new btSequentialImpulseConstraintSolver)
{
    m_physicsWorld = new btDiscreteDynamicsWorld(m_collisionDispatcher, m_broadphase, m_constraintSolver, m_collisionConfig);
    m_physicsWorld->setGravity(btVector3(0, 0, 0));
    m_vehicleShape = nullptr;
    m_nextUsedId = 0;

    m_physicsThread = OpenThreads::Thread::CurrentThread();
}

PhysicsEngine::~PhysicsEngine()
{
    // cleanup

    // first deleting actors
    for (auto actPair : m_actors)
    {
        m_physicsWorld->removeAction(actPair.second);
        delete actPair.second;
    }
    m_actors.clear();

    // Then removing rigid bodies and collision objects
    btCollisionObjectArray& collObjects = m_physicsWorld->getCollisionObjectArray();
    for (int i = 0; i < m_physicsWorld->getNumCollisionObjects(); ++i)
    {
        btCollisionObject* obj = collObjects.at(i);
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        delete obj;
    }
    collObjects.clear();
    m_vehicles.clear();

    for (int i = 0; i < m_collisionShapes.size(); ++i)
    {
        btCollisionShape* shape = m_collisionShapes.at(i);
        delete shape;
    }
    m_collisionShapes.clear();
    delete m_vehicleShape;

    delete m_physicsWorld;
    delete m_constraintSolver;
    delete m_broadphase;
    delete m_collisionDispatcher;
    delete m_collisionConfig;
}


void PhysicsEngine::setShipTransformation(unsigned int shipId, btTransform t)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    m_vehicles[shipId]->setLinearVelocity(btVector3(0, 0, 0));
    m_vehicles[shipId]->setWorldTransform(t);
}


osg::Vec3f PhysicsEngine::getShipPosition(unsigned int shipId)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    btVector3 v =  m_vehicles[shipId]->getWorldTransform().getOrigin();
    return osg::Vec3f(v.x(), v.y(), v.z());
}

unsigned int PhysicsEngine::addCollisionSphere(osg::Vec3f pos, float radius, float mass)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    btSphereShape* newShape = new btSphereShape(radius);
    m_collisionShapes.push_back(newShape);

    btTransform initialTransform;
    initialTransform.setIdentity();
    initialTransform.setOrigin(btVector3(pos.x(), pos.y(), pos.z()));

    //btMotionState* motionState = new btDefaultMotionState(initialTransform);
    //btVector3 inertia;
    //newShape->calculateLocalInertia(0.0, inertia);

    // TODO: upgrade to the rigid body status?
    btCollisionObject* newBody = new btCollisionObject;
    newBody->setCollisionShape(newShape);
    newBody->setWorldTransform(initialTransform);
    m_collisionObjects.insert(std::make_pair(m_nextUsedId, newBody));
    ++m_nextUsedId;

    m_physicsWorld->addCollisionObject(newBody, COLLISION_ASTEROIDGROUP, COLLISION_SHIPGROUP);
    return m_nextUsedId - 1;
}

void PhysicsEngine::removeCollisionSphere(unsigned int id)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    btCollisionObject* obj = m_collisionObjects[id];
    m_physicsWorld->removeCollisionObject(obj);

    m_collisionObjects.erase(id);
    btCollisionShape* shape = obj->getCollisionShape();
    m_collisionShapes.remove(shape);
    delete obj;
    delete shape;
}

unsigned int PhysicsEngine::addUserVehicle(const osg::Vec3f& pos, const osg::Vec3f& sizes, const osg::Quat& orient, float mass)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    if (! m_vehicleShape)
        m_vehicleShape = new btBoxShape(btVector3(sizes.x() / 2, sizes.y() / 2, sizes.z() / 2));
    //m_collisionShapes.push_back(m_vehicleShape);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setRotation(btQuaternion(orient.x(), orient.y(), orient.z(), orient.w()));
    startTransform.setOrigin(btVector3(pos.x(), pos.y(), pos.z()));

    VehicleMotionState* motionState = new VehicleMotionState(this, m_nextUsedId, startTransform);
    btVector3 inertia;
    m_vehicleShape->calculateLocalInertia(mass, inertia);

    btRigidBody* body = new btRigidBody(mass, motionState, m_vehicleShape, inertia);
    m_vehicles[m_nextUsedId] = body;
    ++m_nextUsedId;

    body->setDamping(0.5f, 0.7f);
    m_physicsWorld->addRigidBody(body, COLLISION_SHIPGROUP, COLLISION_SHIPGROUP | COLLISION_ASTEROIDGROUP);

    return m_nextUsedId - 1;
}

btRigidBody* PhysicsEngine::getBodyById(unsigned int id)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    return m_vehicles[id];
}

ShipPhysicsActor* PhysicsEngine::getActorById(unsigned int id)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    if (m_actors.count(id) == 0)
    {
        ShipPhysicsActor* myActor = new ShipPhysicsActor(m_vehicles[id]);
        m_physicsWorld->addAction(myActor);
        m_actors[id] = myActor;
    }
    return m_actors[id];
}

void PhysicsEngine::addMotionCallback(unsigned int id, const t_motionFunc& cb)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    m_motionCallbacks[id] = cb;
}

void PhysicsEngine::physicsTick(float msDelta)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    //static int a = 0;

    /*m_realTime += msDelta;
    if (m_realTime > 1000.f / 180.f)
    {
        // We can run stepSimulation
        int stepCount = std::floor(m_realTime * 180.f / 1000.f);
        m_realTime -= stepCount * 1000.f / 180.f;
        for (int i = 0; i < stepCount; ++i)
            assert(m_physicsWorld->stepSimulation(1.f / 180.f, 1, 1.f/180.f) == 1);
    }*/

    m_physicsWorld->stepSimulation(msDelta / 1000.f, 30, 1.f / 120.f);

    //if ((++a) % 50 == 0)
      //  std::cerr << "Real: " << m_realTime << "ms, Physics: " << m_physicsTime << "ms\n";
}

void PhysicsEngine::removeVehicle(unsigned int id)
{
    assert(OpenThreads::Thread::CurrentThread() == m_physicsThread);

    std::cerr << "Removing a vehicle\n";
    btRigidBody* rigBody = getBodyById(id);

    // remove an associated actor, if one exists
    if (m_actors.count(id) > 0)
    {
        ShipPhysicsActor* act = m_actors[id];
        m_physicsWorld->removeAction(act);
        delete act;
        m_actors.erase(id);
    }

    m_physicsWorld->removeRigidBody(rigBody);

    // erase motion state
    if (rigBody->getMotionState())
        delete rigBody->getMotionState();
    delete rigBody;
    m_vehicles.erase(id);
    m_motionCallbacks.erase(id);
}

