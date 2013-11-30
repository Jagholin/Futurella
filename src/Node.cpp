#include "Node.h"
#include "Asteroid.h"

Node::Node(Asteroid* a){
	for (int i = 0; i < 8; i++) child[i] = NULL;
	asteroid = a;
}


Node*
Node::getNextChild(osg::Vec3f *a){
	return child[getContainingChildIndex(a)];
}

Node**
Node::getChildren(){
	return child;
}

int
Node::getContainingChildIndex(osg::Vec3f *vector){
	int index = 0;
	if (asteroid->getPosition().x() < vector->x()) index += 1;
	if (asteroid->getPosition().y() < vector->y()) index += 2;
	if (asteroid->getPosition().z() < vector->z()) index += 4;
	return index;
}

Asteroid*
Node::getAsteroid(){
	return asteroid;
}

void
Node::addAsteroid(Asteroid* asteroid){
	int index = getContainingChildIndex(&asteroid->getPosition());
	if (child[index] == NULL)
		child[index] = new Node(asteroid);
	else
		child[index]->addAsteroid(asteroid);
}