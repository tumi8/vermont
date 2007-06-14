/** \file
 * This is the String Filter
 */


#include "stringFilter.h"



char* stringFilter::hexparser(const char* input) {

  unsigned int i;
  char *output = new char[sizeof(input)];
  int j=0;
  char hex[5], *stop;
  for (i = 0; i < strlen (input); i += 2)
    {
      hex[0] = '0';
      hex[1] = 'x';
      hex[2] = input[i];
      hex[3] = input[i + 1];
      hex[4] = 0;
      output[j] = strtol (hex, &stop, 16);
      printf("-%c", output[j]);
      getc(stdin);

      j+=1;

    }
  output[j] = '\0';
  //strcat(output, "\0");

/*  printf("%s", output);
  getc(stdin);*/

  return output;


}

/**
 * compare given String to packet data and check for a match
 * @param pdata Packet payload
 * @param toMatch the String to be matched against the packet
 * @param plength The packetlength
 * @return true on match false otherwise
 */
inline bool stringFilter::compare(unsigned char *pdata, char* toMatch, unsigned int plength)
{

	int counter=0;


	for(unsigned int i=0;i<plength;i++) {
//	putchar(pdata[i]);

		if ((char)pdata[i] == toMatch[0]) {
			counter = 0;
			for( unsigned int j=1; j<sizeof(toMatch); j++){
				if((char)pdata[i+j] == toMatch[j]) {
					counter++;
				}
				if(counter == sizeof(toMatch) - 1) {

						return true;
				}
			}
		}
	}


		return false;

};

/**
 * prepare the Packet for comparefucntion
 * set Pointer to Payload
 * @param p Packet data
 * @return true if packet contains string false otherwise
 */
bool stringFilter::processPacket(const Packet *p)
{
        unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;
	std::list<char *>::iterator iti;
	unsigned int andCounter=0, notCounter=0;

	payloadOffset = p->payloadOffset;
	if( payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if(pdata == NULL) return false;

	for(iti = andFilters.begin(); iti != andFilters.end(); ++iti) {
		if(compare(pdata, *iti, plength)) andCounter++;
	}
	for(iti = notFilters.begin(); iti != notFilters.end(); ++iti) {
		if(compare(pdata, *iti, plength)) notCounter++;
	}

	if((notCounter == 0) && (andCounter == andFilters.size())) {
		return true;
	} 
			


	return false;

};
