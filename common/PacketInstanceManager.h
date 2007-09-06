#ifndef PACKETINSTANCEMANAGER_H_
#define PACKETINSTANCEMANAGER_H_

#include "InstanceManager.h"
#include "Packet.h"

/* This is here because I cant figure out how to make the template InstanceManager
 * a singleton. There should be only one InstanceManager<Packet>
 */
class PacketInstanceManager
{
private:
	static InstanceManager<Packet>* manager;
	PacketInstanceManager();
public:


	static InstanceManager<Packet>* getManager()
	{
		if (manager == NULL)
			manager = new InstanceManager<Packet>();
		return manager;
	}

	static void destroyManager()
	{
		if (manager)
			delete manager;
		manager = NULL;
	}
};


#endif /*PACKETINSTANCEMANAGER_H_*/
