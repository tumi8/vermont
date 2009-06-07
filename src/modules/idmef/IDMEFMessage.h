#ifndef IDMEFMESSAGE_H_
#define IDMEFMESSAGE_H_

#include "common/ManagedInstance.h"
#include "core/Emitable.h"

#include <string>
#include <map>

using namespace std;

class IDMEFMessage : public ManagedInstance<IDMEFMessage>, public Emitable
{
public:
	// default variable keys (should be set using setVariable)
	const static char* PAR_SOURCE_ADDRESS; // = "SOURCE_ADDRESS";
	const static char* PAR_TARGET_ADDRESS; // = "TARGET_ADDRESS";
	
	IDMEFMessage(InstanceManager<IDMEFMessage>* im);
	virtual ~IDMEFMessage();
	
	void init(const string tmplfilename, string analyzerid);
	void setVariable(const string key, const string value);
	void setVariable(const string key, const uint32_t value);
	void setVariable(const string key, const uint16_t value);
	void setVariable(const string key, const double value);
	void applyVariables();
	string getMessage();
	
private:
	// private variable keys which are set by IDMEFMessage
	const static char* PAR_ANALYZER_ID; // = "ANALYZER_ID";
	const static char* PAR_ANALYZER_HOST; // = "ANALYZER_HOST";
	const static char* PAR_ANALYZER_IP; // = "ANALYZER_IP";
	const static char* PAR_CREATE_TIME; // = "CREATE_TIME";
	const static char* PAR_MESSAGE_ID; // = "MESSAGE_ID";
	const static char* PAR_NTP_TIME; // = "NTP_TIME";
	
	static string hostname;
	static string domainname;
	static string ipAddress;
	static uint32_t idCounter;
	static time_t idCounterTime;
	
	string analyzerId;
	string message;
	map<string, string> parameterValues;
	
	void readTemplate(const string tmplfilename);
	string createMessageID();
	string getCreateTime(time_t t);
	string getNtpStamp(time_t t);
};

#endif /*IDMEFMESSAGE_H_*/
