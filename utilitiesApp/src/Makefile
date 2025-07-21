TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

USR_INCLUDES += -I"/usr/local/lib" -I"/usr/local/epics/support/boost"

LIBRARY_IOC += utilities

# install icpconfig.dbd into <top>/dbd
DBD += utilities.dbd
INC += utilities.h

# specify all source files to be compiled and added to the library
utilities_SRCS += trimString.cpp mkdir.cpp iocshHelpers.cpp ioccalc.cpp iocdcalc.cpp iocstringtest.cpp
utilities_SRCS += dbLoadRecordsFuncs.cpp iocshCmdFuncs.cpp iocname.cpp

utilities_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
