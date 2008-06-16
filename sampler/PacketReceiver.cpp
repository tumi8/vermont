#include "PacketReceiver.h"

#include <sstream>


PacketReceiver::PacketReceiver(const std::string& ownerName)
        : name(ownerName)
{
        queue.setOwner(ownerName);
        StatisticsManager::getInstance().addModule(this);
}

PacketReceiver::~PacketReceiver()
{
        StatisticsManager::getInstance().removeModule(this);
}


/**
 * statistics function called by StatisticsManager
 */
std::string PacketReceiver::getStatistics()
{
        std::ostringstream oss;
        oss << "PacketReceiver(" << name << "): current queue size: " << queue.getCount();

        return oss.str();
}

