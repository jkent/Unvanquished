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

#include "../g_local.h"
#include "qfile.h"


static int mode_to_fsmode(fsMode_t *fm, const char *mode)
{
	switch (mode[0]) {
	case 'r':
		*fm = FS_READ;
		return 0;
	case 'w':
		*fm = FS_WRITE;
		return 0;
	case 'a':
		*fm = FS_APPEND;
		return 0;
	}

	return -1;
}

void q_clearerr(QFILE *f)
{
  f->eof = qfalse;
  f->error = qfalse;
}

int q_fclose(QFILE *f)
{
  trap_FS_FCloseFile(f->fh);
  free(f);
  return 0;
}

int q_feof(QFILE *f)
{
  return (f->eof) ? -1 : 0;
}

int q_ferror(QFILE *f)
{
  return (f->error) ? -1 : 0;
}

int q_fgetc(QFILE *f)
{
  char c;
  if (q_fread(&c, 1, 1, f) < 1) {
    f->eof = qtrue;
    return QEOF;
  }

  return c;
}

char *q_fgets(char *s, int size, QFILE *f)
{
  size_t readlen = size - 1;
  size_t n;
  char *p = s;
  char *nl;
  int rv;

  while (readlen > 0) {
    if (f->buflen > 0) {
      if ((nl = memchr(f->buf, '\n', f->buflen))) {
        n = MIN(nl - f->buf + 1, readlen);
        memcpy(p, f->buf, n);
        memmove(f->buf, f->buf + n, f->buflen - n);
        f->buflen -= n;
        p += n;
        *p = 0;
        return s;
      }

      n = MIN(f->buflen, readlen);
      memcpy(p, f->buf, n);
      memmove(f->buf, f->buf + n, f->buflen - n);
      f->buflen -= n;
      readlen -= n;
      p += n;
    }

    if (readlen > 0) {
      n = MIN(sizeof(f->buf), readlen);
      rv = trap_FS_Read(f->buf, n, f->fh);
      if (rv < 0) {
        f->error = qtrue;
      }
      else if (rv < n) {
        f->eof = qtrue;
      }
      if (rv <= 0) {
        if (p != s) {
          break;
        }
        return NULL;
      }
      f->buflen = rv;
    }
  }

  *p = 0;
  return s;
}

QFILE *q_fopen(const char *qpath, const char *mode)
{
  QFILE *f;
  fsMode_t fm;

  if (!(f = malloc(sizeof(QFILE)))) {
    return NULL;
  }
  f->buflen = 0;
  q_clearerr(f);

  if (mode_to_fsmode(&fm, mode)) {
    free(f);
    return NULL;
  }

  if (trap_FS_FOpenFile(qpath, &f->fh, fm) < 0) {
    free(f);
    return NULL;
  }

  return f;
}

size_t q_fread(void *ptr, size_t size, size_t nmemb, QFILE *f)
{
  size_t readlen = size * nmemb;
  size_t n;
  void *p = ptr;
  int rv;

  if (f->buflen > 0) {
    n = MIN(readlen, f->buflen);
    readlen -= n;
    memcpy(p, f->buf, n);
    memmove(f->buf, f->buf + n, f->buflen - n);
    f->buflen -= n;
    p += n;
  }

  if (readlen > 0) {
    rv = trap_FS_Read(p, readlen, f->fh);
    if (rv < 0) {
      f->error = qtrue;
    }
    else if (rv < readlen) {
      f->eof = qtrue;
    }
    if (rv > 0) {
      p += rv;
    }
  }

  return (size_t)(p - ptr);
}

QFILE *q_freopen(const char *qpath, const char *mode, QFILE *f)
{
  fsMode_t fm;

  trap_FS_FCloseFile(f->fh);
  f->buflen = 0;
  q_clearerr(f);

  if (mode_to_fsmode(&fm, mode)) {
    free(f);
    return NULL;
  }

  if (trap_FS_FOpenFile(qpath, &f->fh, fm) < 0) {
    free(f);
    return NULL;
  }

  return f;
}

int q_fseek(QFILE *f, long offset, int whence)
{
  int rv = -1;

  switch (whence) {
  case SEEK_SET:
    rv = trap_FS_Seek(f->fh, offset, FS_SEEK_SET);
    f->buflen = 0;
    break;
  case SEEK_CUR:
    rv = trap_FS_Seek(f->fh, offset - f->buflen, FS_SEEK_CUR);
    f->buflen = 0;
    break;
  case SEEK_END:
    rv = trap_FS_Seek(f->fh, offset, FS_SEEK_END);
    f->buflen = 0;
    break;
  }
  return rv;
}

long q_ftell(QFILE *f)
{
  return trap_FS_FTell(f->fh) - f->buflen;
}

size_t q_fwrite(const void *ptr, size_t size, size_t nmemb, QFILE *f)
{
  size_t writelen = size * nmemb;

  if (f->buflen) {
    trap_FS_Seek(f->fh, -(f->buflen), FS_SEEK_CUR);
    f->buflen = 0;
  }

  return trap_FS_Write(ptr, writelen, f->fh);
}

int q_ungetc(int c, QFILE *f)
{
  if (f->buflen < sizeof(f->buf)) {
    memmove(f->buf + 1, f->buf, f->buflen);
    f->buflen += 1;
    *f->buf = (char)c;
    return c;
  }

  return QEOF;
}
