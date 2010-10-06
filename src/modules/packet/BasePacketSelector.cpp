
#include "BasePacketSelector.hpp"

BasePacketSelector::BasePacketSelector() : numberOfQueues(1)
{
}
BasePacketSelector::~BasePacketSelector()
{
}
	
void BasePacketSelector::setNumberOfQueues(int n)
{
	numberOfQueues = n;
}
