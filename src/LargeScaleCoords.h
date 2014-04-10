#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
using namespace Magnum;

// 2-Level 3d space coordinate
// This class defines a small-scale coordinate system within a large-scale one.
// "real" world coordinates are determined by:
//      realCoord = largeScale * factor + smallScale
// where factor is a constant scalar value.
class LargeScaleCoord
{
public:

    LargeScaleCoord();
    LargeScaleCoord(const Vector3& largeScale, const Vector3& smallScale);
    LargeScaleCoord(const Vector3d& realCoord);

    Vector3d realCoordinate() const;

    // This function tries to change m_largeScale and m_smallScale components 
    // of this object, so that it has the same large-scale offset as the argument.
    // This function doesn't change the "real" coordinate value on any significant scale,
    // (there are rounding errors of course)
    // Returns true if the change was successful; false otherwise.
    // It will return false, if the coordinates are too far away from each other.
    bool changeLargeScale(const LargeScaleCoord&);

    Vector3 smallScale() const;
    Vector3 largeScale() const;

    void smallScale(const Vector3&);
    void largeScale(const Vector3&);

    static const double sizeFactor;
protected:
    Vector3 m_smallScale;
    Vector3 m_largeScale;
};