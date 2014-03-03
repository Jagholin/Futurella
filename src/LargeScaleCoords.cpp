#include "LargeScaleCoords.h"
#include <algorithm>
#include <cmath>

const double LargeScaleCoord::sizeFactor = 10000.;

LargeScaleCoord::LargeScaleCoord():
m_smallScale(), m_largeScale()
{

}

LargeScaleCoord::LargeScaleCoord(const osg::Vec3f& largeScale, const osg::Vec3f& smallScale):
m_smallScale(smallScale), m_largeScale(largeScale)
{

}

LargeScaleCoord::LargeScaleCoord(const osg::Vec3d& realCoord)
{
    m_smallScale.set(0, 0, 0);
    m_largeScale.set(realCoord.x() / sizeFactor, realCoord.y() / sizeFactor, realCoord.z() / sizeFactor);
}

osg::Vec3d LargeScaleCoord::realCoordinate() const
{
    return m_largeScale * sizeFactor + m_smallScale;
}

bool LargeScaleCoord::changeLargeScale(const LargeScaleCoord& rhs)
{
    osg::Vec3d realR = rhs.realCoordinate();
    osg::Vec3d real = realCoordinate();
    if (std::max({ std::abs(realR.x() - real.x()), std::abs(realR.y() - real.y()), std::abs(realR.z() - real.z())
        }) > sizeFactor)
        return false;

    osg::Vec3f dLarge = rhs.largeScale() - largeScale();
    smallScale(smallScale() + dLarge * sizeFactor);
    largeScale(rhs.largeScale());
    return true;
}

osg::Vec3f LargeScaleCoord::smallScale() const
{
    return m_smallScale;
}

osg::Vec3f LargeScaleCoord::largeScale() const
{
    return m_largeScale;
}

void LargeScaleCoord::smallScale(const osg::Vec3f& rhs)
{
    m_smallScale = rhs;
}

void LargeScaleCoord::largeScale(const osg::Vec3f& rhs)
{
    m_largeScale = rhs;
}
