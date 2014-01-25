#pragma once 

#include "../glIncludes.h"

#include <osg/Drawable>
#include <osg/RenderInfo>


class LevelDrawable : public osg::Drawable
{
public:
    META_Object(futurella, LevelDrawable);

    LevelDrawable(const LevelDrawable & rhs, const osg::CopyOp& op){};
	LevelDrawable();

	virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

private: 
	mutable GLuint basis, instanceInfo, vao;
    void createVertexBuffers() const;
    mutable bool m_initialized;
};