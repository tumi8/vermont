/** @file
 * Filter a packet by checking if it is containing a predefined regEx string
 */

#ifndef REGEXFILTER_H
#define REGEXFILTER_H

#include <list>
#include <string>
#include <string.h>
#include "common/msg.h"
#include "PacketProcessor.h"
#include <sys/types.h>
#include <boost/regex.hpp>



class RegExFilter
	: public PacketProcessor
{

public:

  RegExFilter ()
  {
  };

  virtual ~RegExFilter ()
  {
  };

  void regcre()
  {
  	rexp.assign(match);
  };

  virtual bool processPacket (Packet * p);

  int filtertype;
  std::string match;
  boost::regex rexp;

protected:
  bool compare (char *data);

};

#endif
