// Filename: trueClock.cxx
// Created by:  drose (04Jul00)
// 
////////////////////////////////////////////////////////////////////

#include "trueClock.h"
#include "config_express.h"
#include "numeric_types.h"

TrueClock *TrueClock::_global_ptr = NULL;


#if defined(WIN32_VC)

////////////////////////////////////////////////////////////////////
//
// The Win32 implementation.
//
////////////////////////////////////////////////////////////////////

#include <sys/timeb.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#undef WINDOWS_LEAN_AND_MEAN

static BOOL _has_high_res;
static PN_int64 _frequency;
static PN_int64 _init_count;
static long _init_sec;


double TrueClock::
get_real_time() const {
  if (_has_high_res) {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);

    if (_init_count > count.QuadPart)
      _init_count = count.QuadPart;

    if (_frequency < 0) {
      express_cat.error()
	<< "TrueClock::get_real_time() - frequency is negative!" << endl;
      QueryPerformanceFrequency((LARGE_INTEGER *)&_frequency);
    }
    return (double)(count.QuadPart - _init_count) / (double)_frequency;

  } else {
    // No high-resolution clock; return the best information we have.
    struct timeb tb;
    ftime(&tb);
    return (double)(tb.time - _init_sec) + (double)tb.millitm / 1000.0;
  }
}

TrueClock::
TrueClock() {
  _has_high_res = QueryPerformanceFrequency((LARGE_INTEGER *)&_frequency);

  if (_has_high_res) {
    QueryPerformanceCounter((LARGE_INTEGER *)&_init_count);

  } else {
    express_cat.warning()
      << "No high resolution clock available." << endl;

    struct timeb tb;
    ftime(&tb);
    _init_sec = tb.time;
  }
}



#elif defined(PENV_PS2)

////////////////////////////////////////////////////////////////////
//
// The PS2 implementation.
//
////////////////////////////////////////////////////////////////////

#include <eeregs.h>
#include <eekernel.h>

static unsigned int _msec;
static unsigned int _sec;

// PS2 timer interrupt, as the RTC routines don't exist unless you're
// using the .irx iop compiler, which scares us.  A lot.
static int
timer_handler(int) {
  _msec++;

  if (_msec >= 1000) {
    _msec = 0;
    _sec++;
  }

  return -1;
}

double TrueClock::
get_real_time() const {
  return (double) _sec + ((double) _msec / 1000.0);
}

TrueClock::
TrueClock() {
  _init_sec = 0;
  _msec = 0;
  _sec = 0;

  tT_MODE timer_mode;
  *(unsigned int *) &timer_mode = 0;

  timer_mode.cxxLKS = 1;
  timer_mode.ZRET = 1;
  timer_mode.cxxUE = 1;
  timer_mode.cxxMPE = 1;
  timer_mode.EQUF = 1;

  *T0_COMP = 9375;
  *T0_MODE = *(unsigned int *) &timer_mode;

  EnableIntc(INTC_TIM0);
  AddIntcHandler(INTC_TIM0, timer_handler, -1);
}


#elif !defined(WIN32)

////////////////////////////////////////////////////////////////////
//
// The Posix implementation.
//
////////////////////////////////////////////////////////////////////

#include <sys/time.h>
#include <stdio.h>  // for perror

static long _init_sec;


double TrueClock::
get_real_time() const {
  struct timeval tv;

  int result;

#ifdef GETTIMEOFDAY_ONE_PARAM
  result = gettimeofday(&tv);
#else
  result = gettimeofday(&tv, (struct timezone *)NULL);
#endif

  if (result < 0) {
    // Error in gettimeofday().
    return 0.0;
  }

  // We subtract out the time at which the clock was initialized,
  // because we don't care about the number of seconds all the way
  // back to 1970, and we want to leave the double with as much
  // precision as it can get.
  return (double)(tv.tv_sec - _init_sec) + (double)tv.tv_usec / 1000000.0;
}

TrueClock::
TrueClock() {
  struct timeval tv;

  int result;
#ifdef GETTIMEOFDAY_ONE_PARAM
  result = gettimeofday(&tv);
#else
  result = gettimeofday(&tv, (struct timezone *)NULL);
#endif

  if (result < 0) {
    perror("gettimeofday");
    _init_sec = 0;
  } else {
    _init_sec = tv.tv_sec;
  }
}

#endif
