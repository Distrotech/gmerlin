/*****************************************************************

  cputest.c

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <gavl/gavl.h>
#include <config.h>

/*
 *   Test for available CPU extensions
 *   Shamelessly stolen from ffmpeg (ffmpeg.sf.net)
 */

/*
 *  Wrappers for foreign flag constants
 */

#define MM_MMX    GAVL_ACCEL_MMX
#define MM_MMXEXT GAVL_ACCEL_MMXEXT
#define MM_SSE    GAVL_ACCEL_SSE
#define MM_SSE2   GAVL_ACCEL_SSE2
#define MM_3DNOW  GAVL_ACCEL_3DNOW

#ifdef ARCH_X86
/* ebx saving is necessary for PIC. gcc seems unable to see it alone */
#define cpuid(index,eax,ebx,ecx,edx)\
    __asm __volatile\
        ("movl %%ebx, %%esi\n\t"\
         "cpuid\n\t"\
         "xchgl %%ebx, %%esi"\
         : "=a" (eax), "=S" (ebx),\
           "=c" (ecx), "=d" (edx)\
         : "0" (index));
#endif

int gavl_accel_supported()
  {
#ifdef ARCH_X86
  int rval = 0;

  int eax, ebx, ecx, edx;
  
  __asm__ __volatile__ (
                        /* See if CPUID instruction is supported ... */
                        /* ... Get copies of EFLAGS into eax and ecx */
                        "pushf\n\t"
                        "popl %0\n\t"
                        "movl %0, %1\n\t"
                        
                        /* ... Toggle the ID bit in one copy and store */
                        /*     to the EFLAGS reg */
                        "xorl $0x200000, %0\n\t"
                        "push %0\n\t"
                        "popf\n\t"
                        
                        /* ... Get the (hopefully modified) EFLAGS */
                        "pushf\n\t"
                        "popl %0\n\t"
                        : "=a" (eax), "=c" (ecx)
                        :
                        : "cc" 
                        );
  
  if (eax == ecx)
    return 0; /* CPUID not supported */
  
  cpuid(0, eax, ebx, ecx, edx);
  
  if (ebx == 0x756e6547 &&
      edx == 0x49656e69 &&
      ecx == 0x6c65746e)
    {
    /* intel */
    inteltest:
    cpuid(1, eax, ebx, ecx, edx);
    if ((edx & 0x00800000) == 0)
      return 0;
    rval = MM_MMX;
    if (edx & 0x02000000) 
      rval |= MM_MMXEXT | MM_SSE;
    if (edx & 0x04000000) 
      rval |= MM_SSE2;
    return rval;
    }
  else if (ebx == 0x68747541 &&
           edx == 0x69746e65 &&
           ecx == 0x444d4163)
    {
    /* AMD */
    cpuid(0x80000000, eax, ebx, ecx, edx);
    if ((unsigned)eax < 0x80000001)
      goto inteltest;
    cpuid(0x80000001, eax, ebx, ecx, edx);
    if ((edx & 0x00800000) == 0)
      return 0;
    rval = MM_MMX;
    if (edx & 0x80000000)
      rval |= MM_3DNOW;
    if (edx & 0x00400000)
      rval |= MM_MMXEXT;
    return rval;
    }
  else if
    (ebx == 0x69727943 &&
     edx == 0x736e4978 &&
     ecx == 0x64616574)
    {
    /* Cyrix Section */
    /* See if extended CPUID level 80000001 is supported */
    /* The value of CPUID/80000001 for the 6x86MX is undefined
       according to the Cyrix CPU Detection Guide (Preliminary
       Rev. 1.01 table 1), so we'll check the value of eax for
       CPUID/0 to see if standard CPUID level 2 is supported.
       According to the table, the only CPU which supports level
       2 is also the only one which supports extended CPUID levels.
    */
    if (eax != 2) 
      goto inteltest;
    cpuid(0x80000001, eax, ebx, ecx, edx);
    if ((eax & 0x00800000) == 0)
      return 0;
    rval = MM_MMX;
    if (eax & 0x01000000)
      rval |= MM_MMXEXT;
    return rval;
    }
  else
    {
    return 0;
    }
#else
  return 0;
#endif
  }

int gavl_real_accel_flags(int wanted_flags)
  {
  int ret;
  ret = wanted_flags & (gavl_accel_supported()|GAVL_ACCEL_C|GAVL_ACCEL_C_HQ);
  return ret;
  }
