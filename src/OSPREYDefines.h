
          /*\      __   __   __   __   __
          shs-    |  | |__  |__| |__| |__  \ /
         dM_d:    |__|  __| |    |  \ |__   |  0.1
        dL:KM     Arduino compatible configuration-less, plug-and-play
       dM56Mh     networking over PJON. Giovanni Blu Mitolo 2016
      yM87MM:     gioscarab@gmail.com
      dgfi3h-
       NM*(Mm          /|  Copyright (c) 2014-2019,
   ___yM(U*MMo        /j|  Giovanni Blu Mitolo All rights reserved.
 _/OF/sMQWewrMNhfmmNNMN:|  Licensed under the Apache License, Version 2.0
|\_\+sMM":{rMNddmmNNMN:_|
       yMMMMso         \|  You may obtain a copy of the License at
       gtMfgm              http://www.apache.org/licenses/LICENSE-2.0
      mMA@Mf
      MMp';M
      ysM1MM:
       sMM3Mh     EXPERIMENTAL,
        dM6MN     USE AT YOUR OWN RISK.
         dMtd:
          \*/

#pragma once

// Master device id
#ifndef OSPREY_MASTER_ID
  #define OSPREY_MASTER_ID              254
#endif

// Maximum devices handled by master
#ifndef OSPREY_MAX_SLAVES
  #define OSPREY_MAX_SLAVES              25
#endif

// Dynamic addressing
#define OSPREY_ID_REQUEST               200
#define OSPREY_ID_CONFIRM               201
#define OSPREY_ID_NEGATE                203
#define OSPREY_ID_LIST                  204
#define OSPREY_ID_REFRESH               205

// Errors
#define OSPREY_ID_ACQUISITION_FAIL      105
#define OSPREY_DEVICES_BUFFER_FULL      254

// Dynamic addressing port number
#define OSPREY_DYNAMIC_ADDRESSING_PORT    1

/* Maximum number of device id collisions during auto-addressing */
#define OSPREY_MAX_ACQUIRE_ID_COLLISIONS 10
/* Delay between device id acquisition and self request (1000 milliseconds) */
#define OSPREY_ACQUIRE_ID_DELAY        1000
/* Master free id broadcast response interval (100 milliseconds) */
#define OSPREY_ID_REQUEST_INTERVAL   100000
/* Master ID_REQUEST and ID_NEGATE timeout */
#define OSPREY_ADDRESSING_TIMEOUT   4000000
/* Master reception time during LIST_ID broadcast (250 milliseconds) */
#define OSPREY_LIST_IDS_TIME         250000
