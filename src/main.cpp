#include <osgDB/ReadFile>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <CEGUI/CEGUI.h>
#include <chrono>

#include "CEGUIDrawable.h"

int main()
{
    // setup CEGUI as OSG drawable
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root = new osg::Group;

    osg::ref_ptr <osg::Node> cessnaNode = osgDB::readNodeFile("cessna.osg");
    osg::ref_ptr<osg::Geode> ceguiNode = new osg::Geode;
    ceguiNode->addDrawable(new CeguiDrawable);

    root->addChild(cessnaNode);
    root->addChild(ceguiNode);

    viewer.setSceneData(root.get());

    return viewer.run();
}