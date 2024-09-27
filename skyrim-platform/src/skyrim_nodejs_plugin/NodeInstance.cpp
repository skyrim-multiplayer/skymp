#include "NodeInstance.h"

struct NodeInstance::Impl
{

};

NodeInstance::NodeInstance()
{
    pImpl = std::make_shared<Impl>();
}

void NodeInstance::Load()
{

}
