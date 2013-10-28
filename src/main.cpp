#include <osgDB\ReadFile>
#include <osg\Group>
#include <osg\ShapeDrawable>
#include <osgViewer\Viewer>
#include <chrono>

#include <iostream>

int main()
{
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root = new osg::Group;

    osg::ref_ptr<osg::Geode> cubeNode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> cubeDrawable = new osg::ShapeDrawable(new osg::Box(osg::Vec3(), 10));
    cubeDrawable->setColor(osg::Vec4(1,0,0,1));
    cubeNode->addDrawable(cubeDrawable);
    root->addChild(cubeNode);

    osg::ref_ptr <osg::Node> cessnaNode = osgDB::readNodeFile("cessna.osg");

    viewer.setSceneData(cessnaNode.get());

    return viewer.run();
}