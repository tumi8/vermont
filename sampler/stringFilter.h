/** @file
 * Filter a packet by checking if it is containing a predefined string
 */

#ifndef STRINGFILTER_H
#define STRINGFILTER_H

#include <list>
#include <string>
#include "msg.h"
#include "PacketProcessor.h"




class stringFilter:public PacketProcessor
{

public:

  stringFilter ()
  {

  };

  virtual ~ stringFilter ()
  {

  };

  void addandFilter (char *string)
  {
    andFilters.push_back (string);
  };

  void addnotFilter (char *string)
  {
    notFilters.push_back (string);
  };

  char* hexparser(const char* input);

  virtual bool processPacket (const Packet * p);

//  int filtertype;


protected:


  std::list < char *> andFilters;
  std::list < char *> notFilters;

  bool compare (unsigned char *data, char *toMatch, unsigned int plength);


//  char *match;



};

#endif
