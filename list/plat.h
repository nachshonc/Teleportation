// Requires : gcc
// Supports : ia32, x64, sparc-V9, sparc-V9
// Exposes  : CAS32, CAS64, MEMBAR, rdclock, Pause
//
// See also: gcc-atomics.h
//
// To report gcc prefined preprocessor values :
//   echo | gcc -dM -E -x c - | sort
//
// Consider: Pause() operators should mark "memory" as a side-effect



#ifndef _PLAT_HPP
#define _PLAT_HPP
#include <stdint.h>

typedef unsigned long long u64 ;
typedef long long i64 ;
typedef unsigned char byte ;
typedef unsigned int uns ;
typedef int (*Entry)() ;
typedef intptr_t (*IEntry)() ;
typedef uintptr_t Address ;

#define ADDR(x)     ((Address)(x))
#define U64(x)      ((uint64_t)(x))
#define BAD         ((void *) 0xBAD)
#define ALIGN8K     __attribute__ ((aligned(8192)))

#define __CTASSERT(x) { int tag[1-(2*!(x))]; printf ("Tag @%X\n", tag); }
#define CTASSERT(x) { typedef int tag [1-(2*!(x))]; }
#define DIM(A)      (sizeof(A)/sizeof((A)[0]))
#define UNS(a)      ((uintptr_t)(a))

// Consider also: weak, alias

#if __GNUC__
//#define _ALIGN(x)    __attribute__ ((aligned(x)))
#define NOINLINE     __attribute__ ((noinline))
#define ALWAYSINLINE __attribute__ ((always_inline))
#define ATTRIBUTE(x) __attribute__ ((x))
#define _FLATTEN     __attribute__ ((flatten))
#define _HOT         __attribute__ ((hot))
#define _COLD        __attribute__ ((cold))
#define _INTERNAL    __attribute__ ((visibility ("internal")))
#define _EXPORT      __attribute__ ((visibility ("default")))
#define _ANORETURN   __attribute__ ((__noreturn__))
#define _NORETURN    __attribute__ ((noreturn))
#define _SETJMP      __attribute__ ((returns_twice))
#define _ALIAS(x)    __attribute__ ((alias (#x)));
#define _PRAGMA(x)   _Pragma (#x)
#define TODO(x)      _PRAGMA(message ("TODO - " #x))
#else
//#define _ALIGN(x)
#define NOINLINE
#define ALWAYSINLINE
#define ATTRIBUTE(x)
#define _FLATTEN
#define _HOT
#define _COLD
#define _INTERNAL
#define _NORETURN
#define _SETJMP
#define TODO(x) (0)
#endif

#define _AI ALWAYSINLINE
#define _NI NOINLINE


#if !defined(_ASSERTS)
#define _ASSERTS 0
#endif

#if _ASSERTS
#define ASSERT(x)   ((x) || printf ("ASSERT %s:%d %s\n", __FILE__,__LINE__,#x))
#else
// implementation variants :
// #define ASSERT(x)   assert(x)
// #define ASSERT(x)   { if (!(x)) *((volatile int *) 0xBAD) = \
//                                                      __FILE__##__LINE__ ; }
// #define ASSERT(x) ((x) || __afail(__FILE__,__LINE__,#x))
#define ASSERT(x)   (0)
#endif

#define POW2(x)     (((x) & ((x)-1)) == 0)
#define BUMP(x)     {;}
// #define FREPORT(m)  {;}
#define FREPORT(m)  {static int ct ; ct++; if POW2(ct) printf (#m " %d\n", ct);}
#define ADR(x)      ((Address)(x))
#define ALIGN8K     __attribute__ ((aligned(8192)))
#define PT(x)       (__builtin_expect((x),1))       // predict taken (true)
#define PN(x)       (__builtin_expect((x),0))       // predict not-taken

#if defined(__sun)
#define sc_incrit sc_pad
#else
typedef struct { int _d ; } sc_shared_t ;
#endif

#if defined(__sun)
#define _IFSUN(x)   (x)
#define _IFLINUX(x)
#define _PlafLinux   0
#define _PlafSolaris 1
#endif

#if defined(__linux)
#define _IFSUN(x)
#define _IFLINUX(x)     (x)
#define _PlafLinux    1
#define _PlatSolaris  0
#endif

// Architecture-specific primitives
// Extracted from gcc-atomics.h -- See gcc-atomics.h for canonical forms
// TODO-FIXME: change CAS operators to be static inline methods for
// better type checking.
// Use: __SIZEOF_POINTER__ vs _LP64

#if !defined(__GNUC__)
#error "Must compile with GCC"
#endif

#define ASM         __asm__ __volatile__
#define CBAR()      ASM ("nop;" ::: "memory")           // Sequence point

#if defined(__sparc)

#define CAS32(ptr,cmp,set)                      \
  ({  typeof(set) tt = (set);                   \
      __asm__ __volatile__ (                    \
        "cas [%2],%3,%0"                        \
        : "=&r" (tt)                            \
        : "0" (tt), "r" (ptr), "r" (cmp)        \
        : "memory" );                           \
      tt ;                                      \
  })

#define _Refined_CAS32(ptr,cmp,set)             \
  ({  int tt = (set);                           \
      __asm__ __volatile__ (                    \
        "cas [%[ADR]],%[CMP],%[SET]"            \
        : [SET] "+r" (tt)                       \
        : [ADR] "r" (ptr), [CMP] "r" (cmp)      \
        : "memory" );                           \
      tt ;                                      \
  })

#define __CAS64(ptr,cmp,set)                    \
  ({  typeof(set) tt = (set);                   \
      __asm__ __volatile__ (                    \
        "casx [%2],%3,%0"                       \
        : "=&r" (tt)                            \
        : "0" (tt), "r" (ptr), "r" (cmp)        \
        : "memory" );                           \
      tt ;                                      \
  })


#define MEMBAR(Type)    ASM ("membar #" #Type ::: "memory")
#define MEMBARN(Type)   ASM ("membar #" #Type)
#define LDNF(a)         ({ int v;                                              \
                           ASM ("lda [%1]0x82, %0" : "=r" (v) : "r" (a)); v;})
#define _RawSelf        ({ char * rr ; ASM ( "mov %%g7,%0" : "=r" (rr)); rr ;})
#define Pause()         ({ ASM ("rd %ccr,%g0") ; 0; })
#define PauseN()        ({ ASM ("rd %ccr,%g0") ; 0; })
#define PauseM()        ({ ASM ("rd %ccr,%g0;" ::: "memory") ; 0; })


// Use either __SIZEOF_POINTER__ or _LP64
#if __SIZEOF_POINTER__ == 4
#define CASN  CAS32
#undef  CAS64
#endif

#if __SIZEOF_POINTER__ == 8
#define CASN  __CAS64
#define CAS64 __CAS64
#endif


#endif

// Synonyms : __amd64; __amd64__; __x86_64
//
// MFENCE yields worse results than "LOCK:ADDB TOS,0" or "XCHG TOSDummy,0" on
// NHM when measured via TLRW-ByteLock with "rb D10 d25 u25 r10000 t=1|2|4|8" on
// ubu.local Nehalem Dell XPS 730X. Beware too that MFENCE is about 35 clocks on
// AMD ShangHai whereas LOCK:CMPCHG is about 20.
//
// Consider:
// * Use "+a" vs "=&a" vs "=a" with "0"
// * Use %[NAME] vs %1
// * CMPXCHG r,m EQU eax = CAS m, eax, r
// * need "q" specifier for cmpxchgq ?
// * Beware: EAX/RAX is the implicit "cmp" (input) and "dst" (result) register
//   for CMPXCHG

#if defined(__x86_64)

#define CAS32(m,c,s)                                        \
  ({ int32_t _x = (c);                                      \
     __asm__ __volatile (                                   \
       "lock; cmpxchgl %1,%2;"                              \
       : "+a" (_x)                                          \
       : "r" ((int32_t)(s)), "m" (*(volatile int32_t *)(m)) \
       : "cc", "memory") ;                                  \
   _x; })

#define CAS64(m,c,s)                                            \
  ({ int64_t _x = (c);                                          \
     __asm__ __volatile (                                       \
       "lock; cmpxchgq %1,%2;"                                  \
       : "+a" (_x)                                              \
       : "r" ((int64_t)(s)), "m" (*(volatile int64_t *)(m))     \
       : "cc", "memory") ;                                      \
   _x; })


// The following variant specifies rax/eax as +r and the memory
// operand as +m.  "+" specifies both incoming and outgoing.
// By refining the specification we can then avoid the general "memory"
// constraint specified in the side-effects list, possibly allowing better
// optimization over invocations of the CAS() operator.
// Nota Bene : this swaps the %2,%1 parameters relative to the baseline form
// above.

#define _Refined__CAS64(m,c,s)                              \
  ({ int64_t _x = (c);                                      \
     __asm__ __volatile (                                   \
       "lock; cmpxchgq %2,%1;"                              \
       : "+a" (_x), "+m" (*(volatile int64_t *)(m))         \
       : "r" ((int64_t)(s))                                 \
       : "cc") ;                                            \
   _x; })


#define MEMBAR(Type) { ASM ("lock;addb $0,(%%rsp);" ::: "cc") ; }
#define Pause()      ({ ASM ("rep;nop;") ; 0 ; })
#define APICID   ({ intptr_t rawid = 0;                              \
   __asm__ __volatile ("push %%rbx; push %%rcx; push %%rdx; mov $1,%%rax;cpuid;mov %%rbx,%%rax;pop %%rdx;pop %%rcx;pop %%rbx;" \
     : "=a" (rawid)); rawid >> 24; })
#define Halt()      Pause()

#define CASN   CAS64

#endif

#if defined(__i386)

#define CAS32(m,c,s)                                        \
  ({ int32_t _x = (c);                                      \
     __asm__ __volatile (                                   \
       "lock; cmpxchgl %1,%2;"                              \
       : "+a" (_x)                                          \
       : "r" ((int32_t)(s)), "m" (*(volatile int32_t *)(m)) \
       : "cc", "memory") ;                                  \
   _x; })

#define MEMBAR(Type) { ASM ("lock;addb $0,(%%esp);" ::: "cc") ; }
#define Pause()      ({ ASM ("rep;nop;") ; 0 ; })
#define APICID   ({ intptr_t rawid = 0;                              \
   __asm__ __volatile ("push %%ebx; push %%ecx; push %%edx; mov $1,%%eax;cpuid;mov %%ebx,%%eax;pop %%edx;pop %%ecx;pop %%ebx;" \
     : "=a" (rawid)); rawid >> 24; })

// Beware that RDTSC reads into EDX:EAX in both 64- and 32-bit processor modes.
// Furthermore, the "=A" contraint specifier seems broken in X86_64 mode.
// Given that, we read out the lo and hi portions individually.
// We might consider incorporating the SHL and OR directly into the ASM
// directive.
#define rdclock() ({ int64_t _hi, _lo ;                                        \
                     ASM ("rdtsc;" : "=a" (_lo), "=d" (_hi)) ; (_hi << 32)|_lo;\
                   })

// rdtscp:
// -- returns tsc in edx:eax, but
// -- returns MSR_TSC_AUX in ecx
//    Linux initializes MSR_TSC_AUX  as (NUMANode << 12) | CPUID
// -- is serializing
// -- is 6x-8x faster than CPUID(1)
#define rdtscp_ecx() ({ int32_t _id ;                                          \
                        ASM ("rdtscp;" : "=c" (_id) :: "eax", "edx") ; _id; })

#define Halt()      Pause()

#define CASN   CAS32
#undef  CAS64

#endif

#if defined(__x86_64)
#define _x86 64
#endif

#if defined(__i386)
#define _x86 32
#endif

#if defined(__i386) || defined(__x86_64)

#define __XADDL(m,v)                                 \
  ({ int _x = (v) ;                                  \
     __asm__ __volatile__ (                          \
       "lock;xaddl %[SET],%[MEM]; "                  \
       : [SET] "+r" (_x)                             \
       : [MEM] "m" (*(volatile int *)(m))            \
       : "memory", "cc" ) ;                          \
   _x ; })

#define __XADDL_Refined(m,v)                         \
  ({ int _x = (v) ;                                  \
     __asm__ __volatile__ (                          \
       "lock;xaddl %[SET],%[MEM]; "                  \
       : [SET] "+r" (_x),                            \
         [MEM] "+m" (*(volatile int *)(m))           \
       : "cc" ) ;                                    \
   _x ; })

#define __XADDQ(m,v)                                 \
  ({ int64_t _x = (v) ;                              \
     __asm__ __volatile__ (                          \
       "lock;xaddq %[SET],%[MEM]; "                  \
       : [SET] "+r" (_x)                             \
       : [MEM] "m" (*(volatile int64_t *)(m))        \
       : "memory", "cc" ) ;                          \
   _x ; })

#define __XCHGL(m,v)                                 \
  ({  int32_t _x = (v) ;                             \
      __asm__ __volatile__ (                         \
       "xchgl %[SET],%[MEM]; "                       \
       : [SET] "+r" (_x)                             \
       : [MEM] "m" (*(volatile int32_t *)(m))        \
       : "memory" ) ;                                \
    _x; })

#define __XCHGL_Refined(m,v)                                        \
  ({  int32_t _x = (v) ;                                            \
      __asm__ __volatile__ (                                        \
       "xchgl %[SET],%[MEM]; "                                      \
       : [SET] "+r" (_x), [MEM] "+m" (*(volatile int32_t *)(m))     \
       ) ;                                                          \
    _x; })

#define __XCHGQ(m,v)                                 \
  ({  int64_t _x = (v) ;                             \
      __asm__ __volatile__ (                         \
       "xchgq %[SET],%[MEM]; "                       \
       : [SET] "+r" (_x)                             \
       : [MEM] "m" (*(volatile int64_t *)(m))        \
       : "memory" ) ;                                \
    _x; })

#define __XCHGQ_Refined(m,v)                                        \
  ({  int64_t _x = (v) ;                                            \
      __asm__ __volatile__ (                                        \
       "xchgq %[SET],%[MEM]; "                                      \
       : [SET] "+r" (_x), [MEM] "+m" (*(volatile int64_t *)(m))     \
       ) ;                                                          \
    _x; })

#if defined(__i386)
#define __XCHGN __XCHGL
#endif

#if defined(__x86_64)
#define __XCHGN __XCHGQ
#endif

#endif

#if defined(__i386)

STATIC_INLINE uint64_t
__cmpxchg8b(volatile uint64_t *ptr, uint64_t old, uint64_t new) {
  uint64_t prev;
  __asm__ __volatile__("lock; cmpxchg8b %1"
        : "=A" (prev),
          "+m" (*ptr)
        : "b" ((uint32_t)new),
          "c" ((uint32_t)(new >> 32)),
          "0" (old)
        : "memory", "cc");
  return prev;
}

#endif

#if defined(__sparc)

STATIC_INLINE int __XCHGL (volatile int * ptr, int v) {
  __asm__ __volatile ("swap [%[ADR]],%[RV]"
     : [RV] "+r" (v)
     : [ADR] "r" (ptr)
     : "memory" );
  return v ;
}

#if defined(_LP64)

STATIC_INLINE int64_t __XCHGN (volatile int64_t * a, int64_t v) {
  int64_t k = *a ;
  for (;;) {
    const int64_t r = CAS64 (a, k, v) ;
    if (r == k) return r ;
    k = r ;
  }
}

#else
#define __XCHGN __XCHGL
#endif

// ASR24 = %STICK
#define rdclock()    ({ int64_t ts; ASM ("rd %%asr24,%0;" : "=r" (ts)) ; ts; })

#endif

#if defined(__sun)
#if defined(__i386)
// Solaris IA32 x86_32 i386 32-bit
#define _RawSelf      ({ char * rr ; ASM ( "movl %%gs:0,%0" : "=r" (rr)); rr ;})
#else
#if defined(__x86_64)
// Solaris x86_64 amd64 64-bit
#define _RawSelf      ({ char * rr ; ASM ( "movq %%fs:0,%0" : "=r" (rr)); rr ;})
#endif
#endif
#endif

// Add aliases -- synonyms
// CASN, CASInt, CAS32, CAS64, CAS4b, CAS8b; CASRef; CASR; CASPtr;
// Add polymophic CAS() that uses typeof/sizeof target




#endif /* _PLAT_HPP */


