# $File: //ASP/opa/acc/eqc/saf/ludlum_m375/trunk/Ludlum_M375TestApp/src/Makefile $
# $Revision: #1 $
# $DateTime: 2019/10/24 17:42:53 $
# Last checked in by: $Author: starritt $

TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = Ludlum_m375Test
# Ludlum_m375Test.dbd will be created and installed
DBD += Ludlum_m375Test.dbd

# Ludlum_m375Test.dbd will be made up from these files:
Ludlum_m375Test_DBD += base.dbd

# Include dbd files from all support applications:
#
Ludlum_m375Test_DBD += asyn.dbd
Ludlum_m375Test_DBD += drvAsynIPPort.dbd
Ludlum_m375Test_DBD += drvAsynSerialPort.dbd
Ludlum_m375Test_DBD += drv_ludlum_m375.dbd

# Add all the support libraries needed by this IOC
#
Ludlum_m375Test_LIBS += asyn
Ludlum_m375Test_LIBS += drv_ludlum_m375

# Ludlum_m375Test_registerRecordDeviceDriver.cpp derives from Ludlum_m375Test.dbd
Ludlum_m375Test_SRCS += Ludlum_m375Test_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
Ludlum_m375Test_SRCS_DEFAULT += Ludlum_m375TestMain.cpp
Ludlum_m375Test_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#Ludlum_m375Test_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
Ludlum_m375Test_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
