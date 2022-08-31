# $File: //ASP/opa/acc/eqc/saf/ludlum_m375/trunk/Ludlum_M375Sup/src/Makefile $
# $Revision: #5 $
# $DateTime: 2020/05/23 13:49:42 $
# Last checked in by: $Author: starritt $
#

TOP=../..

include $(TOP)/configure/CONFIG

#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================

# Build drv_ludlum_m375 as a library for an IOC:
LIBRARY_IOC += drv_ludlum_m375
 
# Library Source files
#
drv_ludlum_m375_SRCS += drv_ludlum_m375.cpp

# Link with the asyn and base libraries
#
drv_ludlum_m375_LIBS += asyn
drv_ludlum_m375_LIBS += $(EPICS_BASE_IOC_LIBS)

# Install .dbd and .db files
#
DBD += drv_ludlum_m375.dbd

#========================================
# Service support scripts
#
SCRIPTS += ludlum_m375_comms_reset
SCRIPTS += ludlum_m375_c2c
SCRIPTS += ludlum_m375_manage
SCRIPTS += ludlum_m375_manage.service
SCRIPTS += ludlum_set_controller_function

#========================================
include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
