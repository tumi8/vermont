#include "IDMEFMessage.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

string IDMEFMessage::hostname;
string IDMEFMessage::domainname;
string IDMEFMessage::ipAddress;
uint32_t IDMEFMessage::idCounter = 0;
time_t IDMEFMessage::idCounterTime = time(0);

const char* IDMEFMessage::PAR_MESSAGE_ID = "MESSAGE_ID";
const char* IDMEFMessage::PAR_SOURCE_ADDRESS = "SOURCE_ADDRESS";
const char* IDMEFMessage::PAR_TARGET_ADDRESS = "TARGET_ADDRESS";
const char* IDMEFMessage::PAR_ANALYZER_ID = "ANALYZER_ID";
const char* IDMEFMessage::PAR_ANALYZER_HOST = "ANALYZER_HOST";
const char* IDMEFMessage::PAR_ANALYZER_IP = "ANALYZER_IP";
const char* IDMEFMessage::PAR_CREATE_TIME = "CREATE_TIME";
const char* IDMEFMessage::PAR_NTP_TIME = "NTP_TIME";



IDMEFMessage::IDMEFMessage(InstanceManager<IDMEFMessage>* im)
: ManagedInstance<IDMEFMessage>(im)
{

}

/**
 * initializes this instance of IDMEFMessage
 * @param tmplfilename filename which is to be used as IDMEF XML template
 * @param analyzerid IDMEF analyzer id of this message
 */
void IDMEFMessage::init(const string tmplfilename, string analyzerid)
{
	analyzerId = analyzerid;
	time_t t = time(0);

	if (this->hostname == "") {
		// retrieve default values
		char hostname[512];
		if (gethostname(hostname, 512) != 0) THROWEXCEPTION("failed to call gethostname, error: %s", strerror(errno));
		char domainname[512];
		if (getdomainname(domainname, sizeof(domainname)) != 0) THROWEXCEPTION("failed call getdomainname, error: %s", strerror(errno));
		struct hostent* he = gethostbyname(hostname);
		if (he == NULL) THROWEXCEPTION("failed to call gethostbyname, error: %s", strerror(errno));

		this->hostname = string(hostname) + "." + string(domainname);
		ipAddress = inet_ntoa(*((struct in_addr *)he->h_addr));
		msg(MSG_DIALOG, "using hostname %s and ip address %s", this->hostname.c_str(), ipAddress.c_str());
	}

	readTemplate(tmplfilename);

	// set idmef parameters
	setVariable(PAR_ANALYZER_ID, analyzerId);
	setVariable(PAR_ANALYZER_IP, ipAddress);
	setVariable(PAR_ANALYZER_HOST, hostname);
	setVariable(PAR_CREATE_TIME, getCreateTime(t));
	setVariable(PAR_NTP_TIME, getNtpStamp(t));
	setVariable(PAR_MESSAGE_ID, createMessageID());
}

IDMEFMessage::~IDMEFMessage()
{
}

/**
 * reads a template into the internal buffer
 * @param tmplfilename template to be used
 */
void IDMEFMessage::readTemplate(const string tmplfilename)
{
	FILE* f = fopen(tmplfilename.c_str(), "r");
	if (f == NULL) {
		THROWEXCEPTION("failed to open template file %s, error: %s", tmplfilename.c_str(), strerror(errno));
	}
	char temp[1025];
	message = "";
	uint32_t bytes;
	while ((bytes = fread(temp, 1, 1024, f)) > 0) {
		temp[bytes] = 0;
		message += temp;
	}
	if (fclose(f) != 0) THROWEXCEPTION("failed to close template file %s, error %s", tmplfilename.c_str(), strerror(errno));
}

/**
 * internally, a map of all possible variables inside the IDMEF template is stored
 * a variable is marked as %KEY% inside the template file and is replaced with the value
 * given in parameter 'value' in this function
 */
void IDMEFMessage::setVariable(const string key, const string value)
{
	parameterValues[key] = value;
}

/**
 * for documentation see setVariable(string, string)
 */
void IDMEFMessage::setVariable(const string key, const uint32_t value)
{
	char valtext[15];
	snprintf(valtext, 15, "%u", value);
	parameterValues[key] = valtext;
}
void IDMEFMessage::setVariable(const string key, const uint16_t value)
{
	char valtext[15];
	snprintf(valtext, 15, "%u", value);
	parameterValues[key] = valtext;
}
void IDMEFMessage::setVariable(const string key,const double value)
{
	char valtext[15];
	snprintf(valtext,15, "%f",value);
	parameterValues[key] = valtext;
}
/**
 * returns ntp time string for idmef message
 */
string IDMEFMessage::getNtpStamp(time_t t)
{
	char timestr[100];

	// 2208988800 is the amount of seconds between 1900-01-01 and 1970-01-01
	snprintf(timestr, sizeof(timestr), "0x%lX.0x0", t+2208988800UL);

	return timestr;
}

/**
 * return time string for idmef message
 */
string IDMEFMessage::getCreateTime(time_t t)
{
	char timestr[100];
	struct tm* tms;
	tms = localtime(&t);
	strftime(timestr, sizeof(timestr), "%F-T%TZ", tms);
	return timestr;
}

/**
 * creates a unique id for the IDMEF message
 * this function takes input from the current time, the processid, a random variable
 * and an internal counter
 */
string IDMEFMessage::createMessageID()
{
	char id[64];

	uint32_t randvar = rand()%10000000;
	if (idCounter > 0x1000000) {
		// idCounterTime contains time of last reset of idCounter
		if (idCounterTime == time(0)) {
			// this ensures, that all created ids are unique, but is pretty crude
			// usually we don't send more than 0x1000000 idmef messages per second
			sleep(1);
		}
		idCounterTime = time(0);
		idCounter = 0;
	}
	snprintf(id, 64, "%012X%X%08X%06X", static_cast<uint32_t>(time(0)), randvar, getpid(), idCounter++);

	return id;
}


/**
 * applies cached variables to template
 */
void IDMEFMessage::applyVariables()
{
	// replace all variable values inside the template
	map<string, string>::iterator iter = parameterValues.begin();
	while (iter != parameterValues.end()) {
		string key = string("%") + iter->first + "%";
		string val = iter->second;

		size_t pos;
		while ((pos = message.find(key)) != string::npos) {
			message.replace(pos, key.size(), val);
		}

		iter++;
	}
}

/**
 * @returns IDMEFMessage
 */
string IDMEFMessage::getMessage()
{
	return message;
}
