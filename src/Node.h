#ifndef hNode_included
#define hNode_included

#include <osg\Vec3>

class Asteroid;

class Node
{
public:
	Node(Asteroid* asteroid);

	Node* getNextChild(osg::Vec3f *vector);
	Node** getChildren();
	int getContainingChildIndex(osg::Vec3f *vector);

	void addAsteroid(Asteroid *asteroid);
	Asteroid* getAsteroid();
	
private:
	Node *(child[8]);
	Asteroid* asteroid;

};

#endif//hNode_included ndef
