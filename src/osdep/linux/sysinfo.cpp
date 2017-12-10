#ifdef __linux__
#include "sysinfo.h"

#include "version.c"

#include "common/msg.h"

#include <locale.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

// small hack to access environ variable conforming to the GNU way
#define environ __environ

#define STAT_FILE    "/proc/stat"
static int stat_fd = -1;
#define UPTIME_FILE  "/proc/uptime"
static int uptime_fd = -1;

long smp_num_cpus;     /* number of CPUs */
static char buf[1024];

/* This macro opens filename only if necessary and seeks to 0 so
 * that successive calls to the functions are more efficient.
 * It also reads the current contents of the file into the global buf.
 */
#define FILE_TO_BUF(filename, fd) do{				\
    static int local_n;						\
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {	\
	THROWEXCEPTION("error: /proc must be mounted!");		\
    }								\
    lseek(fd, 0L, SEEK_SET);					\
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {	\
	perror(filename);					\
	fflush(NULL);						\
	_exit(103);						\
    }								\
    buf[local_n] = '\0';					\
}while(0)

/***********************************************************************
 * Some values in /proc are expressed in units of 1/HZ seconds, where HZ
 * is the kernel clock tick rate. One of these units is called a jiffy.
 * The HZ value used in the kernel may vary according to hacker desire.
 * According to Linus Torvalds, this is not true. He considers the values
 * in /proc as being in architecture-dependant units that have no relation
 * to the kernel clock tick rate. Examination of the kernel source code
 * reveals that opinion as wishful thinking.
 *
 * In any case, we need the HZ constant as used in /proc. (the real HZ value
 * may differ, but we don't care) There are several ways we could get HZ:
 *
 * 1. Include the kernel header file. If it changes, recompile this library.
 * 2. Use the sysconf() function. When HZ changes, recompile the C library!
 * 3. Ask the kernel. This is obviously correct...
 *
 * Linus Torvalds won't let us ask the kernel, because he thinks we should
 * not know the HZ value. Oh well, we don't have to listen to him.
 * Someone smuggled out the HZ value. :-)
 *
 * This code should work fine, even if Linus fixes the kernel to match his
 * stated behavior. The code only fails in case of a partial conversion.
 *
 * Recent update: on some architectures, the 2.4 kernel provides an
 * ELF note to indicate HZ. This may be for ARM or user-mode Linux
 * support. This ought to be investigated. Note that sysconf() is still
 * unreliable, because it doesn't return an error code when it is
 * used with a kernel that doesn't support the ELF note. On some other
 * architectures there may be a system call or sysctl() that will work.
 */

unsigned long long Hertz;

static void old_Hertz_hack(void){
  unsigned long long user_j, nice_j, sys_j, other_j;  /* jiffies (clock ticks) */
  double up_1, up_2, seconds;
  unsigned long long jiffies;
  unsigned h;
  char* savelocale;

  savelocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");
  do{
    FILE_TO_BUF(UPTIME_FILE,uptime_fd);  sscanf(buf, "%lf", &up_1);
    /* uptime(&up_1, NULL); */
    FILE_TO_BUF(STAT_FILE,stat_fd);
    sscanf(buf, "cpu %Lu %Lu %Lu %Lu", &user_j, &nice_j, &sys_j, &other_j);
    FILE_TO_BUF(UPTIME_FILE,uptime_fd);  sscanf(buf, "%lf", &up_2);
    /* uptime(&up_2, NULL); */
  } while((long long)( (up_2-up_1)*1000.0/up_1 )); /* want under 0.1% error */
  setlocale(LC_NUMERIC, savelocale);
  jiffies = user_j + nice_j + sys_j + other_j;
  seconds = (up_1 + up_2) / 2;
  h = (unsigned)( (double)jiffies/seconds/smp_num_cpus );
  /* actual values used by 2.4 kernels: 32 64 100 128 1000 1024 1200 */
  if        ( (h >= 9   ) && (h <= 11  ) ){
    /* S/390 (sometimes) */
    Hertz = 10;
  } else if ( (h >= 18  ) && (h <= 22  ) ){
    /* user-mode Linux */
    Hertz = 20; /* S/390 (sometimes) */
  } else if ( (h >= 30  ) && (h <= 34  ) ){
    /* ia64 emulator */
    Hertz = 32;
  } else if ( (h >= 48  ) && (h <= 52  ) ){
    Hertz = 50;
  } else if ( (h >= 58  ) && (h <= 61  ) ){
    Hertz = 60;
  } else if ( (h >= 62  ) && (h <= 65  ) ){
    /* StrongARM /Shark */
    Hertz = 64;
  } else if ( (h >= 95  ) && (h <= 105 ) ){
    /* normal Linux */
    Hertz = 100;
  } else if ( (h >= 124 ) && (h <= 132 ) ){
    /* MIPS, ARM */
    Hertz = 128;
  } else if ( (h >= 195 ) && (h <= 204 ) ){
    /* normal << 1 */
    Hertz = 200;
  } else if ( (h >= 253 ) && (h <= 260 ) ){
    Hertz = 256;
  } else if ( (h >= 393 ) && (h <= 408 ) ){
    /* normal << 2 */
    Hertz = 400;
  } else if ( (h >= 790 ) && (h <= 808 ) ){
    /* normal << 3 */
    Hertz = 800;
  } else if ( (h >= 990 ) && (h <= 1010) ){
    /* ARM */
    Hertz = 1000;
  } else if ( (h >= 1015) && (h <= 1035) ){
    /* Alpha, ia64 */
    Hertz = 1024;
  } else if ( (h >= 1180) && (h <= 1220) ){
    /* Alpha */
    Hertz = 1200;
  } else {
#ifdef HZ
    Hertz = (unsigned long long)HZ;    /* <asm/param.h> */
#else
    /* If 32-bit or big-endian (not Alpha or ia64), assume HZ is 100. */
    Hertz = (sizeof(long)==sizeof(int) || htons(999)==999) ? 100UL : 1024UL;
#endif
    fprintf(stderr, "Unknown HZ value! (%d) Assume %Ld.\n", h, Hertz);
  }
}

#define NOTE_NOT_FOUND 42

//extern char** environ;

/* for ELF executables, notes are pushed before environment and args */
static unsigned long find_elf_note(unsigned long findme){
  unsigned long *ep = (unsigned long *)environ;
  while(*ep++);
  while(*ep){
    if(ep[0]==findme) return ep[1];
    ep+=2;
  }
  return NOTE_NOT_FOUND;
}

#define AT_CLKTCK       17    // frequency of times()
#define LINUX_VERSION(x,y,z)    (0x10000*(x) + 0x100*(y) + z)

static void init_libproc(void){
  // ought to count CPUs in /proc/stat instead of relying
  // on glibc, which foolishly tries to parse /proc/cpuinfo
  //
  // SourceForge has an old Alpha running Linux 2.2.20 that
  // appears to have a non-SMP kernel on a 2-way SMP box.
  // _SC_NPROCESSORS_CONF returns 2, resulting in HZ=512
  // _SC_NPROCESSORS_ONLN returns 1, which should work OK
  smp_num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  if(smp_num_cpus<1) smp_num_cpus=1; /* SPARC glibc is buggy */

  if(linux_version_code > LINUX_VERSION(2, 4, 0)){ 
    Hertz = find_elf_note(AT_CLKTCK);
    if(Hertz!=NOTE_NOT_FOUND) return;
    fputs("2.4+ kernel w/o ELF notes? -- report this\n", stderr);
  }
  old_Hertz_hack();
}


/**
 * main function which tries to determine jiffy time
 */
unsigned long long getHertzValue() 
{
	init_Linux_version();
	init_libproc();
	
	return Hertz; 
}

#endif // __linux__

