#pragma once

#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/AbstractGroupedFeature.h>
#include <Magnum/SceneGraph/FeatureGroup.h>
#include <Magnum/Platform/Sdl2Application.h>
#include "magnumdefs.h"

class UserInputFeature : public SceneGraph::AbstractGroupedFeature3D<UserInputFeature>
{
public:
    explicit UserInputFeature(SceneGraph::AbstractObject3D& obj) :
        SceneGraph::AbstractGroupedFeature3D<UserInputFeature>(obj, &globalFeatureList())
    {
        // nothing serious
    }

    virtual void handleMouseMove(Platform::Sdl2Application::MouseMoveEvent&){}
    virtual void handleMousePress(Platform::Sdl2Application::MouseEvent&){}
    virtual void handleMouseRelease(Platform::Sdl2Application::MouseEvent&){}
    virtual void handleKeyDown(Platform::Sdl2Application::KeyEvent&){}
    virtual void handleKeyUp(Platform::Sdl2Application::KeyEvent&){}

    typedef SceneGraph::FeatureGroup3D<UserInputFeature> Group;

    static Group& globalFeatureList()
    {
        return uiFeatures;
    }
private:

    static Group uiFeatures;
};
