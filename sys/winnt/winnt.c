/*	SCCS Id: @(#)winnt.c	 3.2	 95/09/06		  */
/* Copyright (c) NetHack PC Development Team 1993, 1994 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  Windows NT system functions.
 *
 *  Initial Creation: Michael Allison - January 31/93
 *
 */

#define NEED_VARARGS
#include "hack.h"
#include <dos.h>
#include <direct.h>
#include <ctype.h>
#include "win32api.h"

#ifdef WIN32


/*
 * The following WIN32 API routines are used in this file.
 *
 * GetDiskFreeSpace
 * GetVolumeInformation
 * FindFirstFile
 * FindNextFile
 * FindClose
 *
 */


/* globals required within here */
HANDLE ffhandle = (HANDLE)0;
WIN32_FIND_DATA ffd;


/* The function pointer nt_kbhit contains a kbhit() equivalent
 * which varies depending on which window port is active.
 * For the tty port it is tty_kbhit() [from nttty.c]
 * For the win32 port it is win32_kbhit() [from winmain.c]
 * It is initialized to point to def_kbhit [in here] for safety.
 */

int def_kbhit(void);
int (*nt_kbhit)() = def_kbhit;

char
switchar()
{
 /* Could not locate a WIN32 API call for this- MJA */
	return '-';
}

long
freediskspace(path)
char *path;
{
	char tmppath[4];
	DWORD SectorsPerCluster = 0;
	DWORD BytesPerSector = 0;
	DWORD FreeClusters = 0;
	DWORD TotalClusters = 0;

	tmppath[0] = *path;
	tmppath[1] = ':';
	tmppath[2] = '\\';
	tmppath[3] = '\0';
	GetDiskFreeSpace(tmppath, &SectorsPerCluster,
			&BytesPerSector,
			&FreeClusters,
			&TotalClusters);
	return (long)(SectorsPerCluster * BytesPerSector *
			FreeClusters);
}

/*
 * Functions to get filenames using wildcards
 */
int
findfirst(path)
char *path;
{
	if (ffhandle){
		 FindClose(ffhandle);
		 ffhandle = (HANDLE)0;
	}
	ffhandle = FindFirstFile(path,&ffd);
	return 
	  (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int
findnext() 
{
	return FindNextFile(ffhandle,&ffd) ? 1 : 0;
}

char *
foundfile_buffer()
{
	return &ffd.cFileName[0];
}

long
filesize(file)
char *file;
{
	if (findfirst(file)) {
		return ((long)ffd.nFileSizeLow);
	} else
		return -1L;
}

/*
 * Chdrive() changes the default drive.
 */
void
chdrive(str)
char *str;
{
	char *ptr;
	char drive;
	if ((ptr = index(str, ':')) != (char *)0) 
	{
		drive = toupper(*(ptr - 1));
		_chdrive((drive - 'A') + 1);
	}
}

static int
max_filename()
{
	DWORD maxflen;
	int status=0;
	
	status = GetVolumeInformation((LPTSTR)0,(LPTSTR)0, 0
			,(LPDWORD)0,&maxflen,(LPDWORD)0,(LPTSTR)0,0);
	if (status)
	{
	   return maxflen;
	}
}

int
def_kbhit()
{
	return 0;
}

/* 
 * Windows NT version >= 3.5 supports long file names,
 * even on FAT volumes, so no need for nt_regularize.  Windows NT
 * 3.1 could not do long file names except on NTFS, so nt_regularize
 * was required.
 */

void
nt_regularize(s)	/* normalize file name */
register char *s;
{
	register char *lp;

	for (lp = s; *lp; lp++)
	    if ( *lp == '?' || *lp == '"' || *lp == '\\' ||
		 *lp == '/' || *lp == '>' || *lp == '<'  ||
		 *lp == '*' || *lp == '|' || *lp == ':' || (*lp > 127))
			*lp = '_';
}


char *getusername()
{
	static char nbuf[30];
	int i = sizeof(nbuf);

	if (GetUserName(nbuf, &i)) {
		pline("User name is %s",nbuf);
		return nbuf;
	}
	return (char *)0;
}
# if 0
char *getxxx()
{
char     szFullPath[MAX_PATH] = "";
HMODULE  hInst = NULL;  	/* NULL gets the filename of this module */

GetModuleFileName(hInst, szFullPath, sizeof(szFullPath));
return &szFullPath[0];
}
# endif
#endif /* WIN32 */

/*winnt.c*/
