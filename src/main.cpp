//#include "glincludes.h"
//#include <osgDB/ReadFile>
//#include <osg/Group>
//#include <osg/ShapeDrawable>
//#include <osgViewer/Viewer>
//#include <CEGUI/CEGUI.h>
//#include <osg/MatrixTransform>
//#include <chrono>
//#include <map>
//#include <osgGA/TrackballManipulator>
//#include <iostream>
//#include <OpenThreads/Thread>

//#include "CEGUIDrawable.h"
#include "GUIApplication.h"
//#include "gameclient/LevelDrawable.h"
//#include "Asteroid.h"
//#include "ChaseCam.h"

MAGNUM_APPLICATION_MAIN(GUIApplication)

// int main()
// {
//     std::srand(std::chrono::system_clock::now().time_since_epoch().count());
//     // setup CEGUI as OSG drawable
//     osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
// 
//     osg::ref_ptr<osg::Group> root = new osg::Group;
//     GUIApplication guiApp(viewer, root);
// 
//     osg::ref_ptr<osg::Geode> ceguiNode = new osg::Geode;
//     osg::ref_ptr<CeguiDrawable> guiSurface = new CeguiDrawable;
//     guiSurface->setGuiApplication(&guiApp);
//     ceguiNode->addDrawable(guiSurface);
//     ceguiNode->setCullingActive(false);
// 
//     osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
//     postCamera->setCullingActive(false);
//     postCamera->setRenderOrder(osg::Camera::POST_RENDER);
//     postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
//     postCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
// 
//     postCamera->addChild(ceguiNode);
// 
//     root->addChild(postCamera);
// 
//     viewer->setSceneData(root.get());
//     viewer->realize();
//     viewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
// 
//     osgViewer::ViewerBase::Windows windowList;
//     viewer->getWindows(windowList);
//     windowList[0]->useCursor(false);
// 
//     //Camera setup: in SpaceShipClient
//     //viewer->getCamera()->setProjectionMatrixAsPerspective(60, 16.0f / 9.0f, 0.1f, 1000); //TODO: use real aspect ratio
// 
//     while (!viewer->done()){
//         viewer->frame();
//     }
// }
