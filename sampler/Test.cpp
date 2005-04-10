/*
 * PSAMP Reference Implementation
 *
 * Test.cpp
 *
 * Test software
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include "Globals.h"
#include "Packet.h"
#include "Observer.h"
#include "PacketSink.h"
#include "ExporterSink.h"
#include "Filter.h"
#include "Template.h"
#include "IPHeaderFilter.h"

#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
  Observer *obs = new Observer();
  Filter *filter = new Filter();
  Template *tmpl = Template::readFromFile("template.cfg");
  //PacketProcessor *samp = new SystematicSampler(SystematicSampler::CountBasedSampler, 10, 10);
  PacketProcessor *samp = new RandomSampler(10, 20);
  IPHeaderFilter *flt1 = new IPHeaderFilter(9, 1, CMP_EQ, 1); // ICMP
  //PacketSink *sink = new PacketSink();
  ExporterSink *sink = new ExporterSink(tmpl, 2342);
  
  sink->AddCollector("127.0.0.1", 4711, "UDP");

  // connect the output of the observer to the packet sink
  //obs->addReceiver(sink->getQueue());
  
  obs->addReceiver(filter);
  filter->setReceiver(sink);
  //filter->addProcessor(samp);
  filter->addProcessor(flt1);
  
  sink->runSink();
  filter->startFilter();
  obs->startCapture("lo");
  
  sleep(10);
  
  cerr << "terminating...";
  obs->terminateCapture();
  filter->terminate();
  sink->terminateSink();
  cerr << "done." << endl;
  LOG("%ld of %ld packets passed (%05.2f%%)\n", filter->pktOut, filter->pktIn,
            (float)(100.0*filter->pktOut)/(float)(filter->pktIn));
  
  delete obs;  
  return 0;
}

