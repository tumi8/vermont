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
  switch(h){
  case    9 ...   11 :  Hertz =   10; break; /* S/390 (sometimes) */
  case   18 ...   22 :  Hertz =   20; break; /* user-mode Linux */
  case   30 ...   34 :  Hertz =   32; break; /* ia64 emulator */
  case   48 ...   52 :  Hertz =   50; break;
  case   58 ...   61 :  Hertz =   60; break;
  case   62 ...   65 :  Hertz =   64; break; /* StrongARM /Shark */
  case   95 ...  105 :  Hertz =  100; break; /* normal Linux */
  case  124 ...  132 :  Hertz =  128; break; /* MIPS, ARM */
  case  195 ...  204 :  Hertz =  200; break; /* normal << 1 */
  case  253 ...  260 :  Hertz =  256; break;
  case  393 ...  408 :  Hertz =  400; break; /* normal << 2 */
  case  790 ...  808 :  Hertz =  800; break; /* normal << 3 */
  case  990 ... 1010 :  Hertz = 1000; break; /* ARM */
  case 1015 ... 1035 :  Hertz = 1024; break; /* Alpha, ia64 */
  case 1180 ... 1220 :  Hertz = 1200; break; /* Alpha */
  default:
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

