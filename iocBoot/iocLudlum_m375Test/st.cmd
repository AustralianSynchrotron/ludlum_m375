#!../../bin/linux-x86_64/Ludlum_m375Test

# $File: //ASP/opa/acc/eqc/saf/ludlum_m375/trunk/iocBoot/iocLudlum_m375Test/st.cmd $
# $Revision: #1 $
# $DateTime: 2019/10/24 17:42:53 $
# Last checked in by: $Author: starritt $
#

## You may have to change Ludlum_m375Test to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/Ludlum_m375Test.dbd",0,0)
Ludlum_m375Test_registerRecordDeviceDriver(pdbbase)

# NOTE: Unlike most device drivers we use at the 'tron, in this case the IOC is
# the TCP/IP server and the device is the client.
#
# Aguments
# 1 - port name
# 2 - localhost:port [proto]
# 3 - max clients
# 4 - priority
# 5 - disable auto-connect
# 6 - noProcessEos  - must to set to true.
#
# NOTE: as each probe/controller pair is cycled in/out for calibration, the
# controller must be configured (via web page) to connect to the tcp port 
# correcponding to its new location.
#  
epicsEnvSet("SERVER_NAME", "SR00HOST04")
drvAsynIPServerPortConfigure ("SR15GRM01_SERVER", "${SERVER_NAME}:50000", 1, 1, 0, 1)
drvAsynIPServerPortConfigure ("SR15NRM01_SERVER", "${SERVER_NAME}:50001", 1, 1, 0, 1)

# Fire up the IP Asyn driver and connect to device
# Portname is same as device name
#
# Aguments
# 1 - port name
# 2 - the associated IP Server port name - we exclude the ":0" suffix here,
#     this is appended by the driver
#
Ludlum_M375_Configure ("SR15GRM01", "SR15GRM01_SERVER")
Ludlum_M375_Configure ("SR15NRM01", "SR15NRM01_SERVER")

## Load record instances
#
dbLoadTemplate ("db/ludlum_m375_test.substitutions")

cd ${TOP}/iocBoot/${IOC}
iocInit()

# Turn on low level trace like this
#
# asynSetTraceMask   ("SR15GRM01_SERVER:0",-1,0x9)
# asynSetTraceIOMask ("SR15GRM01_SERVER:0",-1,0x2)
#

var LudlumM375Debug 2

# end
