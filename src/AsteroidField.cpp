#include <iostream>
#include "AsteroidField.h"
#include "Node.h"
#include "Asteroid.h"

AsteroidField::AsteroidField()
{
    m_root = nullptr;
}

AsteroidField::~AsteroidField()
{
    for (std::vector<Asteroid*>::iterator it = m_asteroidIndex.begin(); it != m_asteroidIndex.end(); it++)
        delete *it;
    deleteSubtreeStructure(m_root);
}

Asteroid*
AsteroidField::addAsteroid(Vector3 pos, float radius)
{
    Asteroid *a = new Asteroid(pos, radius);
    m_asteroidIndex.push_back(a);
    if (m_root == nullptr)
        m_root = new Node(a);
    else
        m_root->addAsteroid(a);
    return a;
}

Asteroid*
AsteroidField::getAsteroid(int id)
{
    return m_asteroidIndex[id];
}

unsigned int
AsteroidField::getLength()
{
    return m_asteroidIndex.size();
}

void
AsteroidField::deleteSubtreeStructure(Node* node)
{
    for (int i = 0; i < 8; i++)
    if (node->getChildren()[i])
        deleteSubtreeStructure(node->getChildren()[i]);
    delete node;
}

std::list<Asteroid*>
AsteroidField::getAsteroidsInBlock(Vector3 center, Vector3 width)
{
    Vector3 boxLowCorner = center - width;
    Vector3 boxHighCorner = center + width;
    std::list<Asteroid*> result;
    treeSearchAsteroids(m_root, boxLowCorner, boxHighCorner, result);
    return result;
}

void
AsteroidField::treeSearchAsteroids(Node *node, const Vector3& bottomCorner, const Vector3& topCorner, std::list<Asteroid*> &resultList)
{
    Vector3 selfPos = node->getAsteroid()->getPosition();
    char x, y, z;
    x = inLowerOrHigher(selfPos.x(), bottomCorner.x(), topCorner.x());
    y = inLowerOrHigher(selfPos.y(), bottomCorner.y(), topCorner.y());
    z = inLowerOrHigher(selfPos.z(), bottomCorner.z(), topCorner.z());

    if (x == y && y == z && z == 0)
        resultList.push_back(node->getAsteroid());
    for (char c = 0; c < 8; c++)
    {
        if (x == (c % 2) * 2 - 1)continue;
        if (y == ((c / 2) % 2) * 2 - 1)continue;
        if (z == ((c / 4) % 2) * 2 - 1)continue;
        if (node->getChildren()[c]) treeSearchAsteroids(node->getChildren()[c], bottomCorner, topCorner, resultList);
    }
}

char
AsteroidField::inLowerOrHigher(float a, float bottom, float top)
{
    if (a >= bottom)
    {
        if (a <= top)
            return 0;
        else
            return 1;
    }
    else
        return -1;
}
