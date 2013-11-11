#include <algorithm>
#include "CEGUIDrawable.h"

#include <osg/Texture2D>

bool CeguiDrawable::m_exists = false;

void CeguiDrawable::drawImplementation( osg::RenderInfo& renderInfo ) const
{
    osg::State* myState = renderInfo.getState();

    if (!m_initDone)
    {
        m_initDone = true;
        CEGUI::OpenGL3Renderer::bootstrapSystem();
    }

    osg::StateSet* myStates = new osg::StateSet;
    myStates->setTextureAttributeAndModes(0, new osg::Texture2D(), osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);
    myStates->setAttributeAndModes(new osg::Program(), osg::StateAttribute::PROTECTED | osg::StateAttribute::OFF);

    myState->pushStateSet(myStates);
    myState->apply();
    myState->setActiveTextureUnit(0);
    CEGUI::System::getSingleton().renderAllGUIContexts();
    myState->popStateSet();
}

