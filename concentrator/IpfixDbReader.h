#ifdef DB_SUPPORT_ENABLED

#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "IpfixDbCommon.h"
#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#include <mysql.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TABLES      10     // count of tables that will be send
#define MAX_COL         10     // max count of columns in the table

typedef struct {
	char* cname;       /**column name*/
	uint16_t ipfixId;  /**IPFIX_TYPEID*/
	uint8_t length;    /**IPFIX length*/
} columnDB;

typedef struct {
        char* tableNames[MAX_TABLES];
        int tableCount;
        columnDB* columns[MAX_COL];
        int colCount;
} DbData;


typedef struct {
        CallbackInfo* callbackInfo;
        int callbackCount;
        DbData* dbData;
} DbReader;

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
typedef struct {
        const char* hostName;    /** Hostname*/
        const char* dbName;      /**Name of the database*/
        const char* userName;    /**Username (default: Standarduser) */
        const char* password;    /** Password (default: none) */
        unsigned int portNum;    /** Portnumber (use default) */
        const char* socketName ;      /** Socketname (use default) */
        unsigned int flags;      /** Connectionflags (none) */
        MYSQL* conn;             /** pointer to connection handle */    
        SourceID srcId;
        DbReader* dbReader;
        pthread_mutex_t mutex;   /** start/stop mutex for db replaying process */
        pthread_t thread;
} IpfixDbReader;

        

int initializeIpfixDbReaders();
int deinitializeIpfixDbReaders();
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader);

int startIpfixDbReader(IpfixDbReader* ipfixDbReader);
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader);

IpfixDbReader* createIpfixDbReader(const char* hostname, const char* dbName,
                                   const char* username, const char* password,
                                   unsigned int port, uint16_t observationDomainId);

void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles);


#ifdef __cplusplus
}
#endif

#endif


#endif
