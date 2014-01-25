
#include "glincludes.h"
#include <osgDB/ReadFile>
#include <osg/Group>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <CEGUI/CEGUI.h>
#include <osg/MatrixTransform>
#include <chrono>
#include <map>
#include <osgGA/TrackballManipulator>
#include <iostream>

#include "CEGUIDrawable.h"
#include "GUIApplication.h"
#include "gameclient/LevelDrawable.h"
#include "Asteroid.h"
#include "SpaceShip.h"
#include "ChaseCam.h"

int main()
{

    std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    // setup CEGUI as OSG drawable
    osgViewer::Viewer *viewer = new osgViewer::Viewer;

    osg::ref_ptr<osg::Group> root = new osg::Group;
    GUIApplication guiApp(viewer, root);

    //osg::ref_ptr<osg::Group> asteroids = new osg::Group;
    //Level level(6, 0.5f, 1, asteroids.get());

    osg::ref_ptr<osg::Geode> ceguiNode = new osg::Geode;
    osg::ref_ptr<CeguiDrawable> guiSurface = new CeguiDrawable;
    guiSurface->setGuiApplication(&guiApp);
    ceguiNode->addDrawable(guiSurface);
    ceguiNode->setCullingActive(false);

    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setCullingActive(false);
    postCamera->setRenderOrder(osg::Camera::POST_RENDER);
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);

    //root->addChild(asteroids);
    postCamera->addChild(ceguiNode);

    //root->addChild(postCamera);

    viewer->setSceneData(root.get());
    //viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    //debug octahedron
    osg::ref_ptr<osg::Shader> octahedronVS = new osg::Shader(osg::Shader::VERTEX);
    osg::ref_ptr<osg::Shader> octahedronFS = new osg::Shader(osg::Shader::FRAGMENT);
    octahedronVS->loadShaderSourceFromFile("shader/vs_octahedron.txt");
    octahedronFS->loadShaderSourceFromFile("shader/fs_octahedron.txt");

    osg::ref_ptr<osg::Program> octahedronShader = new osg::Program();
    octahedronShader->addShader(octahedronVS);
    octahedronShader->addShader(octahedronFS);
    octahedronShader->addBindAttribLocation("position", 0);
    octahedronShader->addBindAttribLocation("offset", 1);

    osg::ref_ptr<osg::Drawable> octahedron = new LevelDrawable();
    osg::ref_ptr<osg::Geode> blabla = new osg::Geode();
    blabla->getOrCreateStateSet()->setAttributeAndModes(octahedronShader, osg::StateAttribute::ON);
    blabla->addDrawable(octahedron);
    root->addChild(blabla);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();
    //viewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);

    osgViewer::ViewerBase::Windows windowList;
    viewer->getWindows(windowList);
    windowList[0]->useCursor(false);

    //Camera setup: in SpaceShipClient
    //viewer->getCamera()->setProjectionMatrixAsPerspective(60, 16.0f / 9.0f, 0.1f, 1000); //TODO: use real aspect ratio
    
    std::chrono::duration<float> frameTime(0);
    std::chrono::steady_clock::time_point start;

    while (!viewer->done()){
        start = std::chrono::steady_clock::now();

        guiApp.timeTick(frameTime.count());
        viewer->frame();

        frameTime = std::chrono::steady_clock::now() - start;
    }
}
