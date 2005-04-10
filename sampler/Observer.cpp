/*
 * PSAMP Reference Implementation
 *
 * Observer.cpp
 *
 * Implementation of the packet capturing thread
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
	 
#include <pcap.h>
#include <iostream>
#include <unistd.h>

#include "Observer.h"
#include "Globals.h"
#include "Thread.h"

using namespace std;

//
// void *Observer::observerThread(void *)
//
// This is the main observer loop. It graps packets from libpcap and
// dispatches them to the registered receivers.
//
void *Observer::observerThread(void *arg)
{
  // first we need to get the instance back from the void *arg
  Observer *obs = (Observer *)arg;
  Packet *p;
  int numReceivers = obs->receivers.size();
  const unsigned char *rawPacketData;
  void *myPacketData;
  struct pcap_pkthdr packetHeader;
  
  // start capturing packets
  LOG("Observer: Capturing started for device %s\n", obs->captureInterface);
  obs->captureDevice=pcap_open_live(obs->captureInterface, obs->capturelen, 1, 2000, obs->errorBuffer);
  // check for errors
  if(!obs->captureDevice)
  {
    LOG("Observer: Error initializing pcap interface: %s\n", obs->errorBuffer);
    return NULL;
  }
  
  while(!obs->exitFlag)
  {
    // get next packet (no zero-copy possible *sigh*)
    rawPacketData = pcap_next(obs->captureDevice, &packetHeader);
    if(!rawPacketData)
      continue; // no packet data was available
    
    if(!(myPacketData=malloc(packetHeader.caplen))) {
    	/*
	 FIXME!
	 ALARM - no more memory available
	 1) Start throwing away packets !
	 2) Notify user !
	 3) Try to resolve (?)
	 */
    }
    memcpy(myPacketData, rawPacketData, packetHeader.caplen);
    
    p = new Packet(myPacketData, packetHeader.caplen, numReceivers);
    p->timestamp = packetHeader.ts;
    //LOG("Observer: received packet at %d.%04d, len=%d\n", 
    //    p->timestamp.tv_sec, p->timestamp.tv_usec / 1000, packetHeader.caplen);
    
    // broadcast packet to all receivers
    for(vector<ConcurrentQueue<Packet> *>::iterator it = obs->receivers.begin();
      it != obs->receivers.end(); ++it)
    {
      (*it)->push(p);
    }
  }
}

