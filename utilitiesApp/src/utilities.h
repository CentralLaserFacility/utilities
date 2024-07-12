#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <list>
#include <map>

#include <shareLib.h>

epicsShareFunc std::string epicsShareAPI setIOCName(const char* iocName);
epicsShareFunc std::string epicsShareAPI getIOCName();
epicsShareFunc std::string epicsShareAPI getIOCGroup();

epicsShareFunc std::string epicsShareAPI getIOCName();
epicsShareFunc std::string epicsShareAPI getIOCGroup();

epicsShareFunc void dbLoadRecordsLoop(const char* dbFile, const char* macros, const char* loopVar, int start, int stop, int step);
epicsShareFunc void dbLoadRecordsList(const char* dbFile, const char* macros, const char* loopVar, const char* list, const char* sep);

epicsShareFunc std::string epicsShareAPI trimString(const std::string& str);

epicsShareFunc void iocshCmdLoop(const char* command, const char* macros, const char* loopVar, int start, int stop, int step);
epicsShareFunc void iocshCmdList(const char* command, const char* macros, const char* loopVar, const char* list, const char* sep);


#include <dirent.h>

#endif /* UTILITIES_H */
