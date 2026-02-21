/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2015 Liam Girdwood
 */

#include "local.h"
#include <stdlib.h>

#if defined(__i386__) || defined(__amd64__)
static void x86_cpuid(unsigned int *eax, unsigned int *ebx, unsigned int *ecx,
                      unsigned int *edx, unsigned int op) {
  __asm__ __volatile__("cpuid"
                       : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                       : "a"(op), "c"(0));
}

static unsigned int cpu_x86_flags(void) {
  unsigned int eax, ebx, ecx, edx, id;
  unsigned int cpu_flags = 0;

  x86_cpuid(&id, &ebx, &ecx, &edx, 0);

  if (id >= 1) {
    x86_cpuid(&eax, &ebx, &ecx, &edx, 1);

    if (ecx & (1 << 20))
      cpu_flags |= CPU_X86_SSE4_2;

    if (ecx & (1 << 28))
      cpu_flags |= CPU_X86_AVX;

    if (ecx & (1 << 12))
      cpu_flags |= CPU_X86_FMA;
  }

  if (id >= 7) {
    x86_cpuid(&eax, &ebx, &ecx, &edx, 7);

    if (ebx & (1 << 5))
      cpu_flags |= CPU_X86_AVX2;

    if (ebx & (1 << 16))
      cpu_flags |= CPU_X86_AVX512;
  }

  return cpu_flags;
}
#endif

int cpu_get_flags(void) {
#if defined(__i386__) || defined(__amd64__)
  return cpu_x86_flags();
#endif
  return 0;
}
