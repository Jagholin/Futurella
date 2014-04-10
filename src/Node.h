#pragma once
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
using namespace Magnum;

class Asteroid;

class Node
{
public:
    Node(Asteroid* asteroid);

    Node* getNextChild(Vector3 *vector);
    Node** getChildren();
    int getContainingChildIndex(Vector3 *vector);

    void addAsteroid(Asteroid *asteroid);
    Asteroid* getAsteroid();
    
private:
    Node* m_child[8];
    Asteroid* m_asteroid;
};
