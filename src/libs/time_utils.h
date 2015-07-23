#pragma once
#ifndef _TIME_UTILS_H
#define _TIME_UTILS_H



/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/**
  similar to timeradd, timercmp, etc... function but for timerspec
*/


#define timespec_cmp(a, b, CMP)                                              \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_nsec CMP (b)->tv_nsec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))



#define timespec_add(a, b, result)                                           \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;                          \
    if ((result)->tv_nsec > 1000000000) {                                     \
      ++(result)->tv_sec;                                                     \
      (result)->tv_nsec -= 1000000000;                                        \
    }                                                                         \
  } while (0)


#define timespec_sub(a, b, result)                                           \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                     \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000000;                                        \
    }                                                                         \
  } while (0)


#define timespec_clear(a)                                                    \
  do {                                                                        \
    (a)->tv_sec = 0;                                                          \
    (a)->tv_nsec = 0;                                                         \
  } while (0)




#define timespec_isset(a)                                                    \
  ( !((a)->tv_sec == 0 && (a)->tv_nsec == 0) )


#define timespec_copy(a,b)                                                    \
    do {                                                                      \
    (a)->tv_sec =  (b)->tv_sec;                                              \
    (a)->tv_nsec =  (b)->tv_nsec;                                            \
    } while (0)

#endif
