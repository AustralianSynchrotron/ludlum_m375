// $File: //ASP/opa/acc/eqc/saf/ludlum_m375/trunk/Ludlum_M375Sup/src/drv_ludlum_m375.h $
// $Revision: #3 $
// $DateTime: 2020/11/07 15:44:29 $
// Last checked in by: $Author: starritt $
//
// Description
// Ludlum M375 Digital Area Monitor driver, based on asynPortDriver.
// This driver supports a single Gamma/Neuton radiation monitor.
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
// andrews@ansto.gov.au
// 800 Blackburn Road, Clayton, Victoria 3168, Australia.
//

#ifndef DRV_LUDLUM_M375_H
#define DRV_LUDLUM_M375_H

#include <epicsTime.h>
#include <epicsThread.h>
#include <initHooks.h>
#include <shareLib.h>

#include <asynPortDriver.h>

class epicsShareClass DriverLudlumM375 : public asynPortDriver {
public:
   explicit DriverLudlumM375 (const char* portName,
                             const char *serverPort);
   ~DriverLudlumM375 ();

   enum Qualifiers { Version = 0,          // driver version
                     Dose,                 // accum. dose ai uSv
                     DoseRate,             // dose ai uSv/Hr
                     Count,                // update count from mM375
                     NUMBER_QUALIFIERS };  // must be last

   // Overide asynPortDriver functions needed for this driver.
   //
   asynStatus readOctet (asynUser* pasynUser, char* value, size_t maxChars,
                         size_t* nActual, int* eomReason);

   asynStatus readInt32 (asynUser* pasynUser, epicsInt32* value);

   asynStatus readFloat64 (asynUser* pasynUser, epicsFloat64* value);
   asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);

private:
   const int objectCheck;     // magic number
   const char* const serverPort;

   int indexList [NUMBER_QUALIFIERS];  // used by asynPortDriver

   asynUser* pOctetAsynUser;
   epicsThreadId processThread;
   char threadName [80];
   bool readyToGo;
   volatile bool shutdownRequested;

   // The actual device data
   //
   double dose;
   double doseRate;
   epicsTime lastReadTime;
   epicsInt32 count;

   asynStatus readDeviceData (double &doseRate);
   void threadFunction ();
   void shutdown ();

   // Returns the associated with the qualifer based on pasynUser->reason
   //
   Qualifiers getQualifier (const asynUser* pasynUser) const;

   static const char* qualifierImage (const Qualifiers qualifer);
   static void classThreadFunction (void* parm);
   static void classShutdown (void* arg);
};

#endif // DRV_LUDLUM_M375_H
