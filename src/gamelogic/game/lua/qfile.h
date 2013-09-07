/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Jeff Kent <jeff@jkent.net>

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef _QFILE_H
#define _QFILE_H

#include "../g_local.h"


#define QFILE_BUFSIZE 8192

typedef struct {
	fileHandle_t fh;
	char buf[QFILE_BUFSIZE];
	size_t buflen;
	qboolean eof;
	qboolean error;
} QFILE;


#define QEOF (-1)
#define q_getc(f) q_fgetc(f)

void   q_clearerr(QFILE *f);
int    q_fclose(QFILE *f);
int    q_feof(QFILE *f);
int    q_ferror(QFILE *f);
int    q_fgetc(QFILE *f);
char  *q_fgets(char *s, int size, QFILE *f);
QFILE *q_fopen(const char *qpath, const char *mode);
size_t q_fread(void *ptr, size_t size, size_t nmemb, QFILE *f);
QFILE *q_freopen(const char *qpath, const char *mode, QFILE *f);
int    q_fseek(QFILE *f, long offset, int whence);
long   q_ftell(QFILE *f);
size_t q_fwrite(const void *ptr, size_t size, size_t nmemb, QFILE *f);
int    q_ungetc(int c, QFILE *f);

#endif /* _QFILE_H */
