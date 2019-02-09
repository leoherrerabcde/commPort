#ifndef DETECT_SO_H_INCLUDED
#define DETECT_SO_H_INCLUDED

#ifdef __MINGW32__

#define WINDOW_OS

#else

#ifdef __CYGWIN32__

#define WINDOW_OS

#else

#define LINUX_SO

#endif // __CYGWIN32__

#endif // __MINGW32__



#endif // DETECT_SO_H_INCLUDED
