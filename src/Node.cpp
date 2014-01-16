#include "Node.h"
#include "Asteroid.h"

Node::Node(Asteroid* a)
{
    for (int i = 0; i < 8; ++i) m_child[i] = nullptr;
    m_asteroid = a;
}


Node*
Node::getNextChild(osg::Vec3f *a)
{
    return m_child[getContainingChildIndex(a)];
}

Node**
Node::getChildren()
{
    return m_child;
}

int
Node::getContainingChildIndex(osg::Vec3f *vector)
{
    int index = 0;
    if (m_asteroid->getPosition().x() < vector->x()) index += 1;
    if (m_asteroid->getPosition().y() < vector->y()) index += 2;
    if (m_asteroid->getPosition().z() < vector->z()) index += 4;
    return index;
}

Asteroid*
Node::getAsteroid()
{
    return m_asteroid;
}

void
Node::addAsteroid(Asteroid* asteroid)
{
    int index = getContainingChildIndex(&asteroid->getPosition());
    if (m_child[index])
        m_child[index]->addAsteroid(asteroid);
    else
        m_child[index] = new Node(asteroid);
}
