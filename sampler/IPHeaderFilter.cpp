/*
 * PSAMP Reference Implementation
 *
 * IPHeaderFilter.cpp
 *
 * Filter by data from IP packet given by (offset, size, comparison, targetvalue)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#include "IPHeaderFilter.h"

bool IPHeaderFilter::compareValues(int srcvalue, int dstvalue)
{
  switch(m_comparison)
  {
  case CMP_LT:
    return srcvalue < dstvalue;
  case CMP_LE:
    return srcvalue <= dstvalue;
  case CMP_EQ:
    return srcvalue == dstvalue;
  case CMP_GE:
    return srcvalue >= dstvalue;
  case CMP_GT:
    return srcvalue > dstvalue;
  case CMP_NE:
    return srcvalue != dstvalue;
  default:
    return 0;
  }
}

int IPHeaderFilter::getData(void *data, int size)
{
  switch(size)
  {
  case 1:
    return *((unsigned char *)data);
  case 2:
    return *((unsigned short *)data);
  case 4:
    return *((int *)data);
  default:
    LOG("Invalid Data Size %d\n", size);
    return 0;
  }
}

bool IPHeaderFilter::processPacket(const Packet *p)
{
  int srcvalue = getData(p->getPacketData(m_offset), m_size);
  return compareValues(srcvalue, m_value);
}

