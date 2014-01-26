#pragma once 


#include <osg/Drawable>
#include <osg/RenderInfo>


class LevelDrawable : public osg::Drawable
{
public:
    META_Object(futurella, LevelDrawable);

    LevelDrawable(const LevelDrawable & rhs, const osg::CopyOp& op){};
    LevelDrawable();
    virtual ~LevelDrawable();

    virtual osg::BoundingBox computeBound() const;
    virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

    void addAsteroid(osg::Vec3f position, float scale);

private:
    mutable GLuint basis, instanceInfo, vao;
    std::vector<float> m_instanceRawData;
    mutable bool m_geometryDirty;
    mutable bool m_aabbDirty;
    mutable osg::BoundingBox m_aabb;
    int m_asteroidCount;
    virtual void initGeometry() const;
};