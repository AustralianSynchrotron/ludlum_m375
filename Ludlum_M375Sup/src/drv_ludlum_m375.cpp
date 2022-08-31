// $File: //ASP/opa/acc/eqc/saf/ludlum_m375/trunk/Ludlum_M375Sup/src/drv_ludlum_m375.cpp $
// $Revision: #4 $
// $DateTime: 2020/05/12 15:24:01 $
// Last checked in by: $Author: starritt $
//
// Description
// LUDLUM_M375 Radiation Monitor driver, based on asynPortDriver.
//
// Copyright (c) 2019-2020 Australian Synchrotron
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// Licence as published by the Free Software Foundation; either
// version 2.1 of the Licence, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public Licence for more details.
//
// You should have received a copy of the GNU Lesser General Public
// Licence along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
//
// Contact details:
// andrew.starritt@synchrotron.org.au
// 800 Blackburn Road, Clayton, Victoria 3168, Australia.
//

#include "drv_ludlum_m375.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errlog.h>
#include <epicsExit.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>

#include <asynOctetSyncIO.h>


//==============================================================================
// Useful type neutral numerical macro fuctions.
//
#define ABS(a)             ((a) >= 0  ? (a) : -(a))
#define MIN(a, b)          ((a) <= (b) ? (a) : (b))
#define MAX(a, b)          ((a) >= (b) ? (a) : (b))
#define LIMIT(x,low,high)  (MAX(low, MIN(x, high)))

// Calculates number of items in an array
//
#define ARRAY_LENGTH(xx)   ((int) (sizeof (xx) /sizeof (xx [0])))


//==============================================================================
//
static const char* driverVersion = "1.1.3";

// MUST be consistent with enum Qualifiers type out of Driverludlum_M375 (in drvludlum_M375.h)
//
struct QualifierDefinitions {
   asynParamType type;
   const char* name;          // qualifier name
};

// This table must be consistent with the Qualifiers enum type defined in the header file.
//
static const QualifierDefinitions qualifierList [DriverLudlumM375::NUMBER_QUALIFIERS] = {
   {asynParamOctet,     "DRIVER_VERSION"  },
   {asynParamFloat64,   "DOSE",           },
   {asynParamFloat64,   "DOSERATE",       },
   {asynParamInt32,     "COUNT",          }
};

// Supported interrupts.
//
static const int interruptMask =  asynFloat64Mask;

// Any interrupt must also have an interface.
//
static const int interfaceMask = interruptMask |
                                 asynDrvUserMask | asynOctetMask | asynInt32Mask;

// Not a ASYN_MULTIDEVICE
//
static const int asynFlags = ASYN_CANBLOCK;

static int LudlumM375Debug = 0;    // Errors only

#define OBJECT_CHECK        0x0D375DAF

// The device sends an update every two seconds.
// We allow a bit of wiggle room
//
static const double  maxValidAgeLimit = 7.0;


//==============================================================================
// Local functions
//==============================================================================
//
static void devprintf (const int required_min_verbosity,
                       const char* function,
                       const int line_no,
                       const char* format, ...)
{
   if (LudlumM375Debug >= required_min_verbosity) {
      char message [200];
      va_list arguments;
      va_start (arguments, format);
      vsnprintf (message, sizeof (message), format, arguments);
      va_end (arguments);
      errlogPrintf ("DriverLudlumM375: %3d:%s %s\n", line_no, function, message);
   }
}

// Wrapper macros to devprintf.
//
#define ERROR(...)    devprintf (0, __FUNCTION__, __LINE__, __VA_ARGS__);
#define WARNING(...)  devprintf (1, __FUNCTION__, __LINE__, __VA_ARGS__);
#define INFO(...)     devprintf (2, __FUNCTION__, __LINE__, __VA_ARGS__);
#define DETAIL(...)   devprintf (3, __FUNCTION__, __LINE__, __VA_ARGS__);

#define ASSERT(condtion, ...)   {                                           \
   if (!(condtion)) {                                                       \
      INFO (__VA_ARGS__);                                                   \
      return asynError;                                                     \
   }                                                                        \
}


#define ASSERT_INITIALISED   {                                              \
   if (this->readyToGo != true) {                                           \
      ERROR ("Not initalised\n");                                           \
      return asynError;                                                     \
   }                                                                        \
}


//==============================================================================
// DriverLudlumM375 methods
//==============================================================================
//
DriverLudlumM375::DriverLudlumM375 (const char* portNameIn,
                                    const char *serverPortIn) :
   asynPortDriver (portNameIn,          //
                   1,                   //
//                 NUMBER_QUALIFIERS,   //
                   interfaceMask,       //
                   interruptMask,       //
                   asynFlags,           //
                   1,                   // Autoconnect
                   0,                   // Default priority
                   0),                  // Default stack size
   objectCheck (OBJECT_CHECK),
   serverPort (epicsStrDup (serverPortIn))
{
   char octetPort [80];
   asynStatus status;

   // This is set true if and only if we reach on of constructor.
   //
   this->readyToGo = false;
   this->shutdownRequested  = false;

   // Initialize as effectively 120 second old - this will cause an
   // immediate stale indication until data is actually read.
   //
   this->lastReadTime = epicsTime::getCurrent () - 120.0;
   this->dose = 0.0;
   this->doseRate = 0.0;
   this->count = 0;

   // Set up asyn parameters.
   //
   for (int j = 0; j < ARRAY_LENGTH (qualifierList); j++) {
      status = this->createParam (qualifierList[j].name,
                                  qualifierList[j].type,
                                  &this->indexList[j]);
   }

   snprintf (octetPort, sizeof (octetPort), "%s", this->serverPort);

   // Connect to asyn port to which the controller has been connected.
   // We always use address 0
   //
   status = pasynOctetSyncIO->connect (this->serverPort, 0, &this->pOctetAsynUser, NULL);
   if (status != asynSuccess) {
      ERROR ("can't connect to server port %s, status=%d", octetPort, status);
      return;
   }

   // Create thread name - also used as error/info message qualifier.
   //
   snprintf (this->threadName, sizeof (this->threadName),
             "DriverLUDLUM_M375_%s", this->portName);

   // Start thread
   //
   this->processThread = epicsThreadMustCreate
         (this->threadName, epicsThreadPriorityMedium,
          epicsThreadGetStackSize (epicsThreadStackMedium),
          DriverLudlumM375::classThreadFunction, this);

   if (!this->processThread) {
      ERROR ("Failed to create thread %s", this->threadName);
      return;
   }

   // Register for epics exit callback.
   // Note: shutdown is a static function so we must pass this as a parameter.
   //
   epicsAtExit (DriverLudlumM375::classShutdown, this);

   // We got to the end - the port initialisation has been successful.
   //
   this->readyToGo = true;
   INFO ("%s setup complete", this->portName);
}

//------------------------------------------------------------------------------
//
DriverLudlumM375::~DriverLudlumM375 () { }


//==============================================================================
// Override functions
//==============================================================================
//
asynStatus DriverLudlumM375::readOctet (asynUser* pasynUser, char* value,
                                        size_t maxChars, size_t* nActual, int*)
{
   const Qualifiers qualifier = this->getQualifier (pasynUser);

   asynStatus status;
   size_t length;

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case Version:
         snprintf (value, maxChars, "%s", driverVersion);
         length = strlen (driverVersion);
         *nActual = MIN (length, maxChars);
         break;

      default:
         errlogPrintf ("%s: %s Unexpected qualifier (%s)\n",
                       __FUNCTION__, this->portName,
                       this->qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;

}

//------------------------------------------------------------------------------
//
asynStatus DriverLudlumM375::readInt32 (asynUser* pasynUser, epicsInt32* value)
{
   const Qualifiers qualifier = this->getQualifier (pasynUser);

   asynStatus status = asynError;

   // Did we successfully initialise?
   //
   ASSERT_INITIALISED;

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case Count:
         *value = this->count;
         break;

      default:
         errlogPrintf ("%s: %s Unexpected qualifier (%s)\n", __FUNCTION__,
                       this->portName, this->qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus DriverLudlumM375::readFloat64 (asynUser* pasynUser, epicsFloat64* value)
{
   const Qualifiers qualifier = this->getQualifier (pasynUser);

   asynStatus status = asynError;
   epicsTime timeNow;
   double age;

   // Did we successfully initialise?
   //
   ASSERT_INITIALISED;

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case Dose:
      case DoseRate:
         timeNow = epicsTime::getCurrent ();
         age = timeNow - this->lastReadTime;
         if (age >= maxValidAgeLimit) {
            status = asynTimeout;
            WARNING ("[%s] %s age: %f", this->portName, this->qualifierImage (qualifier), age);
            break;
         }

         // We successfully read this value less than maxValidAgeLimit
         // seconds ago. Just use as is.
         //
         *value = epicsFloat64 (qualifier == Dose ? this->dose : this->doseRate);
         break;

      default:
         errlogPrintf ("%s: %s Unexpected qualifier (%s)\n", __FUNCTION__,
                       this->portName, this->qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus DriverLudlumM375::writeFloat64 (asynUser *pasynUser, epicsFloat64 value)
{
   const Qualifiers qualifier = this->getQualifier (pasynUser);

   asynStatus status = asynError;

   // Did we successfully initialise?
   //
   ASSERT_INITIALISED;

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case Dose:
         // Re-set the accumulated dose - prob an auto saved value.
         //
         this->dose = value;

         // I/O interrupt
         //
         this->setDoubleParam (Dose, this->dose);
         this->setParamStatus (Dose, asynSuccess);
         this->callParamCallbacks ();
         break;

      default:
         errlogPrintf ("%s: %s Unexpected qualifier (%s)\n", __FUNCTION__,
                      this->portName, this->qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus DriverLudlumM375::readDeviceData (double& doseRate)
{
   static const size_t minimumResponseLength = 60;
   static const double timeout = 10.0;

   doseRate = 0.0;   // ensure not erroneous

   char responseBuffer [420];   // Large enough for any valid message 384 bytes.
   size_t nbytesIn;
   int eomReason;
   asynStatus status = asynError;

   status = pasynOctetSyncIO->read
         (this->pOctetAsynUser,
          responseBuffer, sizeof (responseBuffer) - 1,
          timeout, &nbytesIn, &eomReason);

   ASSERT (status == asynSuccess, "[%s] read failure", this->portName);

   // Check size of response - this shoud be at least xxx bytes.
   //
   ASSERT (nbytesIn >= minimumResponseLength, "[%s] response too short", this->portName);

   // Ensure zero terminated.
   //
   responseBuffer [nbytesIn] = '\0';   // ensure terminated

   // Decode the xml response
   // NOTE: We do not use a pukka xml paraser here - we just search for the
   // dose value tag in the responseBuffer with some basic error checks.
   // Array of start tags and end tags.
   //
   static const char* const tagNames [6] = {
      "<area_monitor",
      "<status>",
      "<rate>",
      "</rate>",
      "</status>",
      "</area_monitor>",
   };

   char* tags [ARRAY_LENGTH (tagNames)];
   char* input = responseBuffer;
   for (int j = 0; j < ARRAY_LENGTH (tags); j++) {
      tags [j] = strstr (input, tagNames [j]);
      ASSERT (tags [j], "[%s] missing %s tag", this->portName, tagNames [j]);
      input = tags [j];
   }

   // Value is between 2nd and 3rd (ZERO based) tags, zero terminate value.
   //
   char* value = tags [2] + strlen (tagNames [2]);
   *tags [3] = '\0';

   int n = sscanf (value, "%lf", &doseRate);
   ASSERT(n == 1, "[%s] cannot extract value from '%s'", this->portName, value);

   return asynSuccess;  // All checks okay.
}

//------------------------------------------------------------------------------
//
void DriverLudlumM375::threadFunction ()
{
   epicsThreadSleep (0.5);
   printf ("DriverLUDLUM_M375 thread starting\n");

   // Perform an initial flush
   //
   pasynOctetSyncIO->flush (this->pOctetAsynUser);

   bool firstTime = true;
   while (!this->shutdownRequested) {

      double tempDoseRate;

      const asynStatus status = this->readDeviceData (tempDoseRate);
      const epicsTime timeNow = epicsTime::getCurrent();
      if (this->shutdownRequested) break;

      if (status == asynSuccess) {

         // The value has been read and extracted
         //
         // On 2nd and subsequent input, we can start integtating the
         // dose rate to calculate a dose. Note: we use the previous does rate
         // here - it is the value in affect until we know better.
         //
         if (!firstTime) {
            const double interval = timeNow - this->lastReadTime;
            this->dose += this->doseRate * (interval / 3600.0);
         } else {
            firstTime = false;
         }

         this->doseRate = tempDoseRate;
         this->lastReadTime = timeNow;
         this->count = (this->count + 1) % 100000;

         // I/O interrupt
         //
         this->setDoubleParam (Dose, this->dose);
         this->setParamStatus (Dose, asynSuccess);

         this->setDoubleParam (DoseRate, this->doseRate);
         this->setParamStatus (DoseRate, asynSuccess);

         this->callParamCallbacks ();

         DETAIL ("[%s] dose rate: %.3f uSv/Hr  dose: %.3f uSv",
                 this->portName, this->doseRate, this->dose);

      } else {
         // We had a read error.
         // I/O interrupt if the current values are deemed too old.
         //
         const double theAge = timeNow - this->lastReadTime;
         if (theAge >= maxValidAgeLimit) {
            this->setParamStatus (DoseRate, status);
            this->setParamStatus (Dose, status);
            this->callParamCallbacks ();
            firstTime = true;  // restart integration
         }

         epicsThreadSleep (1.0);
      }
   }

   printf ("DriverLUDLUM_M375 thread complete\n");
}

//------------------------------------------------------------------------------
//
void DriverLudlumM375::shutdown ()
{
   this->shutdownRequested = true;
}

//------------------------------------------------------------------------------
//
DriverLudlumM375::Qualifiers DriverLudlumM375::getQualifier (const asynUser* pasynUser) const
{
   // As we inherit directly from asynPortDriver indexList [0] is zero.
   //
   return (Qualifiers) (pasynUser->reason - this->indexList[0]);
}

//------------------------------------------------------------------------------
// static
const char* DriverLudlumM375::qualifierImage (const Qualifiers q)
{
   static char result[40];

   if ((q >= 0) && (q < ARRAY_LENGTH (qualifierList))) {
      return qualifierList[q].name;
   } else {
      sprintf (result, "unknown (%d)", q);
      return result;
   }
}

//------------------------------------------------------------------------------
// static
void DriverLudlumM375::classThreadFunction (void* parm)
{
   if (parm) {
      // epicsThreadMustCreate expects a regular function, not a class instance function.
      // Convert and verify arg is a DriverLUDLUM_M375 object, then call class
      // instance function.
      //
      DriverLudlumM375* self = (DriverLudlumM375*) parm;
      if (self->objectCheck == OBJECT_CHECK) {
         self->threadFunction ();  // call actuall thread function method.
      } else {
         printf ("threadFunction - object check fail");
      }
   } else {
      printf ("threadFunction - null parameter");
   }
}

//------------------------------------------------------------------------------
// static
void DriverLudlumM375::classShutdown (void* arg)
{
   if (arg) {
      // epicsAtExit expects a regular function, not a class instance function.
      // Convert and verify parm is a DriverLUDLUM_M375 object, then call class
      // instance function.
      //
      DriverLudlumM375* self = (DriverLudlumM375*) arg;
      if (self->objectCheck == OBJECT_CHECK) {
         self->shutdown ();  // call actuall shutdown method.
      } else {
         printf ("shutdown - object check fail");
      }
   } else {
      printf ("shutdown - null parameter");
   }
}


//==============================================================================
// IOC shell commands
//==============================================================================
//
static const iocshArg ConfigureArg0 = { "Asyn port name", iocshArgString };
static const iocshArg ConfigureArg1 = { "Device octet port name", iocshArgString };

static const iocshArg *const LudlumM375ConfigureArgs[2] = {
   &ConfigureArg0,
   &ConfigureArg1
};

static const iocshFuncDef LudlumM375ConfigureFuncDef = {
   "Ludlum_M375_Configure", 2, LudlumM375ConfigureArgs
};

//------------------------------------------------------------------------------
//
static void callLudlumM375Configure (const iocshArgBuf* args)
{
   // Do a basic validation.
   //
   if ((args[0].sval == NULL) || (strlen (args[0].sval) == 0)) {
      errlogPrintf ("Ludlum_M375_Configure: Null/empty ASYN port name\n");
      return;
   }

   if ((args[1].sval == NULL) || (strlen (args[1].sval) == 0)) {
      errlogPrintf ("Ludlum_M375_Configure: port %s: Null/empty device octet port\n",
                    args[0].sval);
      return;
   }

   // Create the diver instance
   //
   new DriverLudlumM375 (args[0].sval, args[1].sval);
}

//------------------------------------------------------------------------------
//
static void LudlumM375Startup (void)
{
   printf ("DriverLudlumM375 startup version %s\n", driverVersion);
   iocshRegister (&LudlumM375ConfigureFuncDef, callLudlumM375Configure);
}


epicsExportAddress (int, LudlumM375Debug);
epicsExportRegistrar (LudlumM375Startup);

// end
