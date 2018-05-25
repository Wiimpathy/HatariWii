/*
  Hatari - version.h

  Name and version for window title (and other outputs)
*/

/* devel */
#if ENABLE_WINUAE_CPU
# define CPU_CORE_NAME "(WinUAE CPU core)"
#else
# define CPU_CORE_NAME "(OldUAE CPU core)"
#endif
//#define PROG_NAME "Hatari v1.9.0-devel (" __DATE__ ") " CPU_CORE_NAME

/* release */
#ifdef GEKKO
#define PROG_NAME "HatariWii v0.0.5"
#else
#define PROG_NAME "Hatari v1.9.0"
#endif
