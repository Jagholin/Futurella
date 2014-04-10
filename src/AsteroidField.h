#pragma once
#include <Magnum/Magnum.h>
#include <vector>
#include <list>

class Asteroid;

class Node;

class AsteroidField
{
public:
    AsteroidField();
    ~AsteroidField();
    Asteroid* addAsteroid(Vector3 pos, float radius);
    Asteroid* getAsteroid(int id);
    unsigned int getLength();
    std::list<Asteroid*> getAsteroidsInBlock(Vector3 blockCenter, Vector3 blockWidth);
private:
    Node* treeAddAsteroid(Asteroid *asteroid, Node* node);
    char inLowerOrHigher(float a, float bottom, float top);
    void deleteSubtreeStructure(Node* node);
    void treeSearchAsteroids(Node* node, const Vector3& boundingBoxBottomCorner, const Vector3& boundingBoxTopCorner, std::list<Asteroid*> &resultList);
    Node *m_root;
    std::vector<Asteroid*> m_asteroidIndex;
};
