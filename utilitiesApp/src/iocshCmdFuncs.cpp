///
/// @file iocshCmdFuncs.cpp Enhanced EPICS iocshCmd functions
/// @author Freddie Akeroyd, STFC ISIS Facility <freddie.akeroyd@stfc.ac.uk>
///
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ncurses.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <time.h>
#include <sstream>
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "epicsStdlib.h"
#include "epicsString.h"
#include "epicsStdio.h"
#include "dbDefs.h"
#include "epicsMutex.h"
#include "dbBase.h"
#include "dbStaticLib.h"
#include "dbFldTypes.h"
#include "dbCommon.h"
#include "dbAccessDefs.h"
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <iocsh.h>
#include "envDefs.h"
#include "macLib.h"
#include "errlog.h"

#include <string.h>
#include <registryFunction.h>

#include <epicsExport.h>

#include "utilities.h"

static char* ioc_getenv(const char* var)
{
	char* val = getenv(var);
	if (val == NULL)
	{
		return strdup("");
	}
	else
	{
		return strdup(val);
	}
}

static void loadMacEnviron(MAC_HANDLE* pmh)
{
	for(char** cp = environ; *cp != NULL; ++cp)
	{
		char* str_tmp = strdup(*cp);
		char* equals_loc = strchr(str_tmp, '='); // split   name=value   string
		if (equals_loc != NULL)
		{
		    *equals_loc = '\0';
		    macPutValue(pmh, str_tmp, equals_loc + 1);
		}
		free(str_tmp);
	}
}

/// look for e.g. \$(I) and replace with $(I) so we can substitute later with macEnvExpand()
static void subMacros(std::string& new_macros, const char* macros, const char* loopVar)
{
    char loopSubFrom[32], loopSubTo[32];
    epicsSnprintf(loopSubFrom, sizeof(loopSubFrom), "\\$(%s)", loopVar);
    epicsSnprintf(loopSubTo, sizeof(loopSubTo), "$(%s)", loopVar);
    new_macros = macros;
    size_t start_pos = 0, lf = strlen(loopSubFrom), lt = strlen(loopSubTo);
    while( (start_pos = new_macros.find(loopSubFrom, start_pos)) != std::string::npos ) 
    {
        new_macros.replace(start_pos, lf, loopSubTo);
        start_pos += lt;
    }
    epicsSnprintf(loopSubFrom, sizeof(loopSubFrom), "\\${%s}", loopVar);
    epicsSnprintf(loopSubTo, sizeof(loopSubTo), "${%s}", loopVar);
	start_pos = 0;
    while( (start_pos = new_macros.find(loopSubFrom, start_pos)) != std::string::npos ) 
    {
        new_macros.replace(start_pos, lf, loopSubTo);
        start_pos += lt;
    }
}

/// Execute command multiple times substituting a specified macro according to a number range.
///
/// The \a command argument is executed, as per the usual EPICS iocshCmd(), but with additional 
/// environment variables set according to the \a macros arguments; however at the end of execution
/// these macros are reset back to their original values.
/// It is possible to embed a changing macro \a loopVar within this list whose value follows the range \a start to \a stop.
///
/// The name of the macro to be used for substitution is contained in \a loopVar and needs to be
/// referenced in an \\ escaped way to make sure EPICS does not try to substitute it too soon.
/// As well as the \a macros the \a command is also passed the \a loopVar macro value
/// @code
///     iocshCmdLoop("< st\$(I).cmd", "P=1,Q=Hello\$(I)", "I", 1, 4)
/// @endcode 
///
/// @param[in] command @copydoc iocshCmdLoopInitArg0
/// @param[in] macros @copydoc iocshCmdLoopInitArg1
/// @param[in] loopVar @copydoc iocshCmdLoopInitArg2
/// @param[in] start @copydoc iocshCmdLoopInitArg3
/// @param[in] stop @copydoc iocshCmdLoopInitArg4
/// @param[in] step @copydoc iocshCmdLoopInitArg5
epicsShareFunc void iocshCmdLoop(const char* command, const char* macros, const char* loopVar, int start, int stop, int step)
{
    char loopVal[32];
    if (loopVar == NULL)
    {
        iocshCmd(command);
        return;
    }
    if (step <= 0)
    {
        step = 1;
    }
    std::string macros_s, command_s;
    subMacros(command_s, command, loopVar);
    subMacros(macros_s, macros, loopVar);
    MAC_HANDLE* mh = NULL;
    char** pairs;
	char macros_exp[1024], command_exp[1024];
    for(int i = start; i <= stop; i += step)
    {
		std::map<std::string,char*> old_values;
        macCreateHandle(&mh, NULL);
		loadMacEnviron(mh);
        epicsSnprintf(loopVal, sizeof(loopVal), "%d", i);
        macPutValue(mh, loopVar, loopVal);   
        macExpandString(mh, command_s.c_str(), command_exp, sizeof(command_exp));
        macExpandString(mh, macros_s.c_str(), macros_exp, sizeof(macros_exp));
		pairs = NULL;
        macParseDefns(mh, macros_exp, &pairs);
		// save old environment and set new values for command
		for(int j=0; pairs[j] != NULL; j += 2)
		{
			old_values[pairs[j]] = ioc_getenv(pairs[j]);
			epicsEnvSet(pairs[j], pairs[j+1]);
		}
		free(pairs);
		old_values[loopVar] = ioc_getenv(loopVar);
		epicsEnvSet(loopVar, loopVal);
        std::ostringstream new_macros;
        new_macros << macros_exp << (strlen(macros_exp) > 0 ? "," : "") << loopVar << "=" << loopVal;
        std::cout << "--> (" << i << ") iocshCmd(\"" << command_exp << "\") with: " << new_macros.str() << std::endl;
        iocshCmd(command_exp);
		// restore old environment
		for(std::map<std::string,char*>::iterator it = old_values.begin(); it != old_values.end(); ++it)
		{
			epicsEnvSet(it->first.c_str(), it->second);
			free(it->second);
		}
		macDeleteHandle(mh);		
    }
}

/// Execute a command multiple times according to a list of items separated by known separator(s).
///
/// The \a command argument is executed, as per the usual EPICS iocshCmd(), but with additional 
/// environment variables set according to the \a macros arguments; however at the end of execution
/// these macros are reset back to their original values.
/// It is possible to embed a changing macro \a loopVar within this list whose value follows items in \a list.
///
/// The name of the macro to be used for substitution is contained in \a loopVar and needs to be
/// referenced in an \\ escaped way to make sure EPICS does not try to substitute it too soon.
/// As well as the \a macros the \a command is also passed the \a loopVar macro value
/// @code
///     iocshCmdList("< st\$(S).cmd", "P=1,Q=Hello\$(S)", "S", "A;B;C", ";")
/// @endcode 
///
/// @param[in] command @copydoc iocshCmdListInitArg0
/// @param[in] macros @copydoc iocshCmdListInitArg1
/// @param[in] loopVar @copydoc iocshCmdListInitArg2
/// @param[in] list @copydoc iocshCmdListInitArg3
/// @param[in] sep @copydoc iocshCmdListInitArg4
epicsShareFunc void iocshCmdList(const char* command, const char* macros, const char* loopVar, const char* list, const char* sep)
{
    static const char* default_sep = ";";
    if (loopVar == NULL || list == NULL)
    {
        iocshCmd(command);
        return;
    }
	if (sep == NULL)
	{
		sep = default_sep;
	}
    std::string macros_s, command_s;
    subMacros(command_s, command, loopVar);
    subMacros(macros_s, macros, loopVar);
    char* saveptr = NULL;
    char* list_tmp = strdup(list);
    char* list_item = epicsStrtok_r(list_tmp, sep, &saveptr);
    MAC_HANDLE* mh = NULL;
    char** pairs;
	char macros_exp[1024], command_exp[1024];
    while(list_item != NULL)
    {
		std::map<std::string,char*> old_values;
        macCreateHandle(&mh, NULL);
		loadMacEnviron(mh);
        macPutValue(mh, loopVar, list_item);   
        macExpandString(mh, command_s.c_str(), command_exp, sizeof(command_exp));
        macExpandString(mh, macros_s.c_str(), macros_exp, sizeof(macros_exp));
		pairs = NULL;
        macParseDefns(mh, macros_exp, &pairs);
		for(int j=0; pairs[j] != NULL; j += 2)
		{
			old_values[pairs[j]] = ioc_getenv(pairs[j]);
			epicsEnvSet(pairs[j], pairs[j+1]);
		}
		free(pairs);
		old_values[loopVar] = ioc_getenv(loopVar);
        epicsEnvSet(loopVar, list_item);   
        std::ostringstream new_macros;
        new_macros << macros_exp << (strlen(macros_exp) > 0 ? "," : "") << loopVar << "=" << list_item;
        std::cout << "--> (" << list_item << ") iocshCmd(\"" << command_exp << "\") with: " << new_macros.str() << std::endl;
        iocshCmd(command_exp);
		for(std::map<std::string,char*>::iterator it = old_values.begin(); it != old_values.end(); ++it)
		{
			epicsEnvSet(it->first.c_str(), it->second);
			free(it->second);
		}
		macDeleteHandle(mh);		
        list_item = epicsStrtok_r(NULL, sep, &saveptr);
    }
    free(list_tmp);
}

extern "C" {

// EPICS iocsh shell commands 

static const iocshArg iocshCmdLoopInitArg0 = { "command", iocshArgString };			///< command string
static const iocshArg iocshCmdLoopInitArg1 = { "macros", iocshArgString };			///< macros to pass to \a command
static const iocshArg iocshCmdLoopInitArg2 = { "loopVar", iocshArgString };			///< loop macro variable name
static const iocshArg iocshCmdLoopInitArg3 = { "start", iocshArgInt };			///< Loop start value
static const iocshArg iocshCmdLoopInitArg4 = { "stop", iocshArgInt };			///< Loop end value
static const iocshArg iocshCmdLoopInitArg5 = { "step", iocshArgInt };			///< Loop step (default: 1)
static const iocshArg * const iocshCmdLoopInitArgs[] = { &iocshCmdLoopInitArg0, &iocshCmdLoopInitArg1,
     &iocshCmdLoopInitArg2, &iocshCmdLoopInitArg3, &iocshCmdLoopInitArg4, &iocshCmdLoopInitArg5 };

static const iocshArg iocshCmdListInitArg0 = { "command", iocshArgString };			///< command string
static const iocshArg iocshCmdListInitArg1 = { "macros", iocshArgString };			///< macros to pass to \a command
static const iocshArg iocshCmdListInitArg2 = { "loopVar", iocshArgString };			///< list macro variable name
static const iocshArg iocshCmdListInitArg3 = { "list", iocshArgString };			///< list of values to substitute
static const iocshArg iocshCmdListInitArg4 = { "sep", iocshArgString };			///< list value separator character
static const iocshArg * const iocshCmdListInitArgs[] = { &iocshCmdListInitArg0, &iocshCmdListInitArg1,
     &iocshCmdListInitArg2, &iocshCmdListInitArg3,  &iocshCmdListInitArg4 };

static const iocshFuncDef iocshCmdLoopDef = {"iocshCmdLoop", sizeof(iocshCmdLoopInitArgs) / sizeof(iocshArg*), iocshCmdLoopInitArgs};

static const iocshFuncDef iocshCmdListDef = {"iocshCmdList", sizeof(iocshCmdListInitArgs) / sizeof(iocshArg*), iocshCmdListInitArgs};

static void iocshCmdLoopInitCallFunc(const iocshArgBuf *args)
{
    iocshCmdLoop(args[0].sval, args[1].sval, args[2].sval, args[3].ival, args[4].ival, args[5].ival);
}

static void iocshCmdListInitCallFunc(const iocshArgBuf *args)
{
    iocshCmdList(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].sval);
}

static void iocshCmdFuncsRegister(void)
{
    iocshRegister(&iocshCmdLoopDef, iocshCmdLoopInitCallFunc);
    iocshRegister(&iocshCmdListDef, iocshCmdListInitCallFunc);
}

epicsExportRegistrar(iocshCmdFuncsRegister); // need to be declared via registrar() in utilities.dbd too

}
