#include "NodeCallbackService.h"

#include <osg/Node>
#include <osg/NodeVisitor>

NodeCallbackService::NodeCallbackService(boost::asio::io_service& aService) :
m_service(aService)
{
    // nop
}

void NodeCallbackService::operator ()(osg::Node* node, osg::NodeVisitor* nv)
{
    m_service.poll();
    m_service.reset();

    if (node->getNumChildrenRequiringUpdateTraversal() > 0)
        nv->traverse(*node);
}
