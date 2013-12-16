#ifndef hAsteroidField_included
#define hAsteroidField_included

#include <osg\Vec3>
#include <vector>
#include <list>

class Asteroid;

class Node;

class AsteroidField
{
public:
	AsteroidField();
	~AsteroidField();
	Asteroid* addAsteroid(osg::Vec3f pos, float radius);
	Asteroid* getAsteroid(int id);
	int getLength();
	std::list<Asteroid*> getAsteroidsInBlock(osg::Vec3f blockCenter, osg::Vec3f blockWidth);
private:
	Node* treeAddAsteroid(Asteroid *asteroid, Node* node);
	char inLowerOrHigher(float a, float bottom, float top);
	void deleteSubtreeStructure(Node* node);
	void treeSearchAsteroids(Node* node, osg::Vec3f* boundingBoxBottomCorner, osg::Vec3f* boundingBoxTopCorner, std::list<Asteroid*> *resultList);
	Node *root;
	std::vector<Asteroid*> asteroidIndex;
};

#endif//hAsteroidField_included ndef
