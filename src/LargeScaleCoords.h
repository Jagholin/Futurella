#pragma once

#include <osg/Vec3>
#include <osg/Vec3d>

// 2-Level 3d space coordinate
// This class defines a small-scale coordinate system within a large-scale one.
// "real" world coordinates are determined by:
//      realCoord = largeScale * factor + smallScale
// where factor is a constant scalar value.
class LargeScaleCoord
{
public:

    LargeScaleCoord();
    LargeScaleCoord(const osg::Vec3f& largeScale, const osg::Vec3f& smallScale);
    LargeScaleCoord(const osg::Vec3d& realCoord);

    osg::Vec3d realCoordinate() const;

    // This function tries to change m_largeScale and m_smallScale components 
    // of this object, so that it has the same large-scale offset as the argument.
    // This function doesn't change the "real" coordinate value on any significant scale,
    // (there are rounding errors of course)
    // Returns true if the change was successful; false otherwise.
    // It will return false, if the coordinates are too far away from each other.
    bool changeLargeScale(const LargeScaleCoord&);

    osg::Vec3f smallScale() const;
    osg::Vec3f largeScale() const;

    void smallScale(const osg::Vec3f&);
    void largeScale(const osg::Vec3f&);

    static const double sizeFactor;
protected:
    osg::Vec3f m_smallScale;
    osg::Vec3f m_largeScale;
};