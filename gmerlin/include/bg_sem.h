/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>

#ifdef HAVE_POSIX_SEMAPHORES /* Map original POSIX API with zero overhead */

#include <semaphore.h>

#else // Use BSD Implementation
/* See original copyright below. */
/* NOTE: On systems, which have non-working versions
   of these functions in their libc, only the -export-symbols-regex
   of libtool prevents clashes */


/* Begin thread_private.h kluge */
/*
 * These come out of (or should go into) thread_private.h - rather than have 
 * to copy (or symlink) the files from the source tree these definitions are 
 * inlined here.  Obviously these go away when this module is part of libc.
*/

struct sem {
#define SEM_MAGIC       ((uint32_t) 0x09fa4012)
        uint32_t       magic;
        pthread_mutex_t lock;
        pthread_cond_t  gtzero;
        uint32_t       count;
        uint32_t       nwaiters;
};

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

/*
 * $Id: bg_sem.h,v 1.1 2008-04-16 22:29:59 gmerlin Exp $
 *
 * semaphore.h: POSIX 1003.1b semaphores
*/

/*-
 * Copyright (c) 1996, 1997
 *	HD Associates, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by HD Associates, Inc
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HD ASSOCIATES AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL HD ASSOCIATES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/posix4/semaphore.h,v 1.6 2000/01/20 07:55:42 jasone Exp $
 */

#include <limits.h>

#include <sys/types.h>
#include <fcntl.h>

/* Opaque type definition. */
struct sem;
typedef struct sem *sem_t;

#define SEM_FAILED	((sem_t *)0)

#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX	UINT_MAX
#endif

#ifndef KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int	 sem_init __P((sem_t *, int, unsigned int));
int	 sem_destroy __P((sem_t *));
sem_t	*sem_open __P((const char *, int, ...));
int	 sem_close __P((sem_t *));
int	 sem_unlink __P((const char *));
int	 sem_wait __P((sem_t *));
int	 sem_trywait __P((sem_t *));
int	 sem_post __P((sem_t *));
int	 sem_getvalue __P((sem_t *, int *));
__END_DECLS
#endif /* KERNEL */

#endif /* _SEMAPHORE_H_ */

#endif // HAVE_POSIX_SEMAPHORES
