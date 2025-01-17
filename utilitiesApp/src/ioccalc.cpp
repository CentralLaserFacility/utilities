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
#include <vector>
#include <memory>

#include "epicsStdlib.h"
#include "epicsString.h"
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

#include "postfix.h"
#include "cvtFast.h"

#include <boost/scoped_array.hpp>

#include <string.h>
#include <registryFunction.h>

#include <epicsExport.h>

#include "utilities.h"

// options
// default: perform calc and return integer in resultVar
// 0x1: verbose
// 0x2: zero pad to specified length
// 0x4: if true ( !=0 ) then return ' ', if false ( ==0 ) return '#'
static void ioccalc(const char* resultvar, const char* expression, int options, int length)
{
    if (resultvar == NULL || expression == NULL)
	{
	    errlogPrintf("ioccalc: ERROR: NULL args");
		return;
	}
	bool verbose = (options & 0x1);
	bool zero_pad = (options & 0x2);
	bool hash_mode = (options & 0x4);
    char* expr_expand = macEnvExpand(expression);
	if (expr_expand == NULL)
	{
	    errlogPrintf("ioccalc: ERROR: NULL expanded expression arg");
		return;
	}
	if (verbose)
	{
	    printf("ioccalc: expanded expression=\"%s\"\n", expr_expand);
	}
	short calc_error;
	static const int CALC_NARGS = 12; // named A to L
	std::vector<double> parg(CALC_NARGS);
	for(int i=0; i<CALC_NARGS; ++i)
	{
	    parg[i] = 0.0;
	}
	double result;
	// need at add extra space to INFIX_TO_POSTFIX_SIZE - bug?
	boost::scoped_array<char> ppostfix(new char[INFIX_TO_POSTFIX_SIZE(strlen(expr_expand)) + 100]);   // cannot use  std::unique_ptr<char[]>  yet
	if ( postfix(expr_expand, ppostfix.get(), &calc_error) != 0 )
	{
	    errlogPrintf("ioccalc: ERROR: postfix: %s\n", calcErrorStr(calc_error));
		return;
	}
	if ( calcPerform(&(parg[0]), &result, ppostfix.get()) != 0 )
	{
	    errlogPrintf("ioccalc: ERROR: calcPerform: %s\n", "");
		return;
	}
	long long_result = static_cast<long>(floor(result + 0.5));
	char result_str[32];
	std::ostringstream format_str;
	if (hash_mode)
	{
		if ( sprintf(result_str, "%s", (long_result != 0 ? " " : "#")) < 0 )
		{
			errlogPrintf("ioccalc: ERROR: sprintf (hash mode)\n");
			return;
		}
	}
	else
	{
		format_str << "%";
		if (zero_pad)
		{
			format_str << "0";
		}
		if (length > 0)
		{
			format_str << length;
		}
		format_str << "ld";
		if ( sprintf(result_str, format_str.str().c_str(), long_result) < 0 )
		{
			errlogPrintf("ioccalc: ERROR: sprintf: %s\n", format_str.str().c_str());
			return;
		}
	}
	if (verbose)
	{
	    printf("ioccalc: setting %s=\"%s\" (%g)\n", resultvar, result_str, result);
        // print out parg array too for info? (values assigned to A, B, C parameters in exporession) 		
	}
	epicsEnvSet(resultvar, result_str);
	free(expr_expand);
}

extern "C" {

// EPICS iocsh shell commands 

// calc "result" "expression"
static const iocshArg calcInitArg0 = { "resultvar", iocshArgString };
static const iocshArg calcInitArg1 = { "expression", iocshArgString };
static const iocshArg calcInitArg2 = { "options", iocshArgInt };
static const iocshArg calcInitArg3 = { "length", iocshArgInt };
static const iocshArg * const calcInitArgs[] = { &calcInitArg0, &calcInitArg1, &calcInitArg2, &calcInitArg3 };

static const iocshFuncDef calcInitFuncDef = {"calc", sizeof(calcInitArgs) / sizeof(iocshArg*), calcInitArgs};

static void calcInitCallFunc(const iocshArgBuf *args)
{
    ioccalc(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
}

static void ioccalcRegister(void)
{
    iocshRegister(&calcInitFuncDef, calcInitCallFunc);
}

epicsExportRegistrar(ioccalcRegister); // need to be declared via registrar() in utilities.dbd too

// asub callable functions - need to be in utilities.dbd as function() 

//epicsRegisterFunction(setIOCName); 
//epicsRegisterFunction(getIOCName); 
//epicsRegisterFunction(getIOCGroup); 

}
