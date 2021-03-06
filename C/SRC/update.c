﻿/*****************************************************************************\
*                                                                             *
*   FILENAME:	    update.c						      *
*									      *
*   PURPOSE:	    Copy files only if the destination file is older	      *
*									      *
*   DESCRIPTION:    Get time of both files. Copy if the destination file      *
*		    does not exist or if it is older than the source.	      *
*									      *
*   Notes:	    Uses our custom debugging macros in debugm.h.	      *
*		    							      *
*		    To build in Unix/Linux, copy debugm.h into your ~/include *
*		    directory or equivalent, then run commands like:	      *
*		    export C_INCLUDE_PATH=~/include			      *
*		    gcc dirsize.c -o dirsize	# Release mode version	      *
*		    gcc -D_DEBUG dirsize.c -o dirsize.debug   # Debug version *
*		    							      *
*   		    This program is based on the "deplace" program. Some      *
*		    comments may still refer to this old program.	      *
*									      *
*		    The target directory name can be specified with a         *
*		    trailing backslash. It is actually a good practice to     *
*		    specify it, because this makes it clear that this is a    *
*                   directory name.                                           *
*		    Note however that if the name is enclosed in "quotes",    *
*		    that trailing backslash will be considered as an escape   *
*                   character for the following quote. Ex:                    *
*		    Command line input		C argument strings	      *
*		    C:\Windows			"C:\\Windows"		      *
*		    C:\Windows\			"C:\\Windows\\"		      *
*		    "C:\Windows"		"C:\\Windows"		      *
*		    "C:\Windows\"		"C:\\Windows\""		      *
*		    "C:\Windows\\"		"C:\\Windows\\"		      *
*		    C:\Program Files		"C:\\Program" "Files"	      *
*		    C:\Program Files\		"C:\\Program" "Files\\"	      *
*		    "C:\Program Files"		"C:\\Program Files"	      *
*		    "C:\Program Files\"		"C:\\Program Files\""	      *
*		    "C:\Program Files\\"	"C:\\Program Files\\"	      *
*		    Version 2.2 of this program includes a workaround for     *
*		    this common mistake, and changes back a trailing " into   *
*                   a trailing \.                                             *
*		    							      *
*		    TO DO: Change copy(), update(), etc, return type to a     *
*		    string pointer, and return NULL for success or a          *
*		    dynamically created error message in case of error.	      *
*		    							      *
*   History:								      *
*    1986-04-01 JFL jf.larvoire@hp.com created this program.		      *
*    1987-05-07 JFL Adapted to Lattice 3.10				      *
*    1992-05-20 JFL Adapted to Microsoft C. Adapted to OS/2. Switch /?.       *
*    1994-10-11 GB  Add -nologo and -noempty options			      *
*		    Version 1.2.					      *
*    1996-10-14 JFL Adapted to Win32. Version 2.0.			      *
*    2001-01-07 JFL Added -v option; Display list of files copied.	      *
*		    Version 2.1.					      *
*    2010-03-04 JFL Changed pathname buffers sizes to PATHNAME_SIZE.          *
*                   Cleaned up obsolete C syntax and library calls.           *
*                   Added a workaround for the trailing \ issue.              *
*                   Output just the file names. Use -v to get the old details.*
*		    Version 2.2.					      *
*    2010-03-10 JFL Added option -p. Version 2.2a.                            *
*    2010-04-09 JFL Removed debugging options. Version 2.2b.                  *
*    2011-09-06 JFL Added the ability to update to a file with a != name.     *
*		    Version 2.3.					      *
*    2012-10-01 JFL Added support for a Win64 version. No new features.       *
*		    Version 2.3.1.					      *
*    2012-10-18 JFL Added my name in the help. Version 2.3.2.                 *
*    2013-02-21 JFL Use the standard directory access functions.              *
*                   Side bug fix: Updating *.c will not copy *.c~ anymore.    *
*                   Added a Linux version.                                    *
*    2013-03-10 JFL Changed the command line syntax to be compatible with     *
*                   wildcards on Linux shells.                                *
*                   Added support for files > 4GB.                            *
*                   Version 3.0.0.					      *
*    2013-03-15 JFL Copy file permissions under Linux.                        *
*                   Added resiliency:                                         *
*                   When reading fails to start, avoid deleting the target.   *
*                   In case of error later on, delete incomplete copies.      *
*                   Version 3.0.1.					      *
*    2014-02-12 JFL Added support for symlinks in all operating systems,      *
*                   and symlinkds and junctions in Windows.                   *
*                   Improved the debugging output.                            *
*                   Version 3.1.                                              *
*    2014-02-21 JFL Added option -r for recursive updates.                    *
*    2014-02-26 JFL Use MsvcLibX LocalFileTime() to display local file times  *
*		    in Windows the same way cmd.exe and explorer.exe do.      *
*    2014-02-28 JFL Added support for UTF-8 pathnames, and output them in     *
*                   the current code page.                                    *
*                   Version 3.2.                                              *
*    2014-06-04 JFL Rebuilt in Windows with support for utimes() in MsvcLibX. *
*                   Version 3.2.1.                                            *
*    2014-06-13 JFL Fixed bug copying files > 2GB in Windows.		      *
*                   Version 3.2.2.                                            *
*    2014-07-01 JFL Fixed bug when copying files from the root directory.     *
*                   Report non ASCII file names correctly in error messages.  *
*    2014-07-02 JFL Added support for pathnames > 260 bytes in Windows.	      *
*                   Changed macro RETURN() to RETURN_CONST(), etc.	      *
*    2014-07-04 JFL Copy the date of directories too.                         *
*    2014-07-09 JFL Fixed a bug when updating existing links.                 *
*                   Version 3.3.                                              *
*    2014-12-04 JFL Rebuilt with MsvcLibX support for WIN32 paths > 260 chars.*
*                   Realigned help. Version 3.3.1.                            *
*    2015-01-06 JFL Bug fix: Delete the target if it's not the same type as   *
*                   the source file; But don't in test mode. Version 3.3.2.   *
*    2015-01-08 JFL Work around issue with old versions of Linux that define  *
*                   lchmod and lutimes, but implement only stubs that always  *
*                   fail. Version 3.3.3.                                      *
*    2015-01-12 JFL Bug fix: In recursive mode, an incorrect directory name   *
*                   was sometimes displayed. (But the correct directory was   *
*                   copied.) Version 3.3.4.                                   *
*    2015-12-14 JFL Bug fix: References to D: actually accessed D:\ in Windows.
*                   Bug fix: Failed to start in Windows XP due to missing fct.*
*                   Bug fix: Writing to disconnected drive returned many errs.*
*		    Bug fix: DOS version failed to read root directories.     *
*                   Version 3.3.5.                                            *
*    2016-01-07 JFL Fixed all warnings in Linux, and a few real bugs.         *
*		    Version 3.3.6.  					      *
*    2016-04-12 JFL Added option -S to show destination names.                *
*		    Version 3.4.    					      *
*    2016-05-10 JFL Added option -F/--force to overwrite read-only files.     *
*		    Version 3.5.    					      *
*    2016-09-13 JFL Minor tweaks to fix compilation in Linux.                 *
*    2017-01-30 JFL Improved mkdirp(), to avoid useless error messages.       *
*                   Added a workaround for the WIN32 _fullpath() bug.         *
*		    Version 3.5.1.    					      *
*    2017-05-11 JFL Display MsvcLibX library version in DOS & Windows.        *
*		    Version 3.5.2.    					      *
*    2017-05-31 JFL But don't display it in help, to limit the 1st line size. *
*		    Version 3.5.3.    					      *
*    2017-10-06 JFL Fixed a conditional compilation bug in MSDOS.	      *
*		    Fixed support for pathnames >= 260 characters. 	      *
*                   Improved mkdirp() speed and error management.             *
*		    Version 3.5.4.    					      *
*    2018-02-27 JFL All updateall() returns done via :cleanup_and_return.     *
*		    Version 3.5.5.    					      *
*    2018-04-24 JFL Use PATH_MAX and NAME_MAX from limits.h. Version 3.5.6.   *
*    2018-05-31 JFL Use the new zapFile() and zapDir() from zap.c.            *
*		    Added option -e to erase target files not in the source.  *
*                   Bug fix: The force option did corrupt the mode flag.      *
*                   Copy empty directories if the copyempty flag is set.      *
*                   Bug fix: Avoid a crash in update_link() on invalid links. *
*                   Prefix all error messages with the program name.          *
*                   Bug fix: mkdirp() worked, but returned an error, if the   *
*		     path contained a trailing [back]slash.		      *
*		    Version 3.6.    					      *
*    2018-12-18 JFL Added option -P to show the file copy progress.           *
*		    Added option -- to force ending switches.                 *
*		    Version 3.7.    					      *
*                                                                             *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_VERSION "3.7"
#define PROGRAM_DATE    "2018-12-18"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#define _ISOC99_SOURCE      /* Tell the GNU library that we support C99 syntax */
#define __STDC_LIMIT_MACROS /* Make sure C99 macros are defined in C++ */
#define __STDC_CONSTANT_MACROS

#define FALSE 0
#define TRUE 1

#define _BSD_SOURCE    		/* Include extra BSD-specific functions. Implied by _GNU_SOURCE. */
#define _LARGEFILE_SOURCE	/* Force using 64-bits file sizes if possible */
#define _GNU_SOURCE		/* Replaces nicely all the above */
#define _FILE_OFFSET_BITS 64	/* Force using 64-bits file sizes if possible */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
/* The following include files are not available in the Microsoft C libraries */
/* Use JFL's MsvcLibX library extensions if needed */
#include <sys/time.h>		/* For lutimes() */
#include <utime.h>		/* For struct utimbuf */
#include <dirent.h>		/* We use the DIR type and the dirent structure */
#include <unistd.h>		/* For the access function */
#include <fnmatch.h>
#include <iconv.h>
#include <inttypes.h>

/* MsvcLibX debugging macros */
#include "debugm.h"

DEBUG_GLOBALS	/* Define global variables used by debugging macros. (Necessary for Unix builds) */

#ifdef _MSC_VER
#pragma warning(disable:4001)	/* Ignore the // C++ comment warning */
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#pragma warning(disable:4996)	/* Ignore the deprecated name warning */

#define _filelength(hFile) _filelengthi64(hFile)

/* Front-end to _fullpath, with work around for trail spaces bug */
char *fullpath(char *absPath, const char *relPath, size_t maxLength);  

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#define fullpath _fullpath

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define DIRSEPARATOR_CHAR '\\'
#define DIRSEPARATOR_STRING "\\"

#define PATTERN_ALL "*.*"     		/* Pattern matching all files */

#define fullpath _fullpath

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#ifdef __unix__		/* Automatically defined when targeting a Unix app. */

#define DIRSEPARATOR_CHAR '/'
#define DIRSEPARATOR_STRING "/"

#define PATTERN_ALL "*"     		/* Pattern matching all files */

/*
#define _MAX_PATH  FILENAME_MAX
#define _MAX_DRIVE 3
#define _MAX_DIR   FILENAME_MAX
#define _MAX_FNAME FILENAME_MAX
#define _MAX_EXT   FILENAME_MAX
*/

#ifndef _S_IREAD
#define _S_IREAD __S_IREAD
#endif
#ifndef _S_IWRITE
#define _S_IWRITE __S_IWRITE
#endif
#ifndef _S_IEXEC
#define _S_IEXEC __S_IEXEC
#endif

#define _stricmp strcasecmp

/* Redefine Microsoft-specific routines */
off_t _filelength(int hFile);
#define fullpath(absPath, relPath, maxLength) realpath(relPath, absPath)
#define LocalFileTime localtime

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#if (!defined(DIRSEPARATOR_CHAR)) || (!defined(EXE_OS_NAME))
#error "Unidentified OS. Please define OS-specific settings for it."
#endif

#define PATHNAME_SIZE PATH_MAX
#define NODENAME_SIZE (NAME_MAX+1)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define strncpyz(to, from, l) {strncpy(to, from, l); (to)[(l)-1] = '\0';}

#ifndef min
#define min(a,b) ( ((a)<(b)) ? (a) : (b) )
#endif

#define TRUE 1
#define FALSE 0

#define streq(string1, string2) (strcmp(string1, string2) == 0)

#ifdef _MSDOS
#define BUFFERSIZE 16384
#else
#define BUFFERSIZE (1024L * 1024L)
#endif
char *buffer;       /* Pointer on the intermediate copy buffer */

#define isConsole(iFile) isatty(iFile)

static int test = 0;			/* Flag indicating Test mode */
static int show = 0;			/* 0=Show source; 1=Show dest */
static int fresh = 0;			/* Flag indicating freshen mode */
static int force = 0;			/* Flag indicating force mode */
static int iVerbose = FALSE;		/* Flag for displaying verbose information */
static int copyempty = TRUE;		/* Flag for copying empty file */
static int iPause = 0;			/* Flag for stop before exit */
static int iProgress = 0;		/* Flag for showing a progress bar */
#ifdef __unix__
static int iFnmFlag = 0;		/* Case-sensitive pattern matching */
#else
static int iFnmFlag = FNM_CASEFOLD;	/* Case-insensitive pattern matching */
#endif
static int iRecur = 0;			/* Recursive update */
#ifdef _WIN32
// UINT cp = 0;				/* Initial console code page */
#define cp codePage			/* Initial console code page in iconv.c */
#endif
static int iErase = 0;			/* Flag indicating Erase mode */

/* Forward references */

char *version(int iVerbose);		/* Build the version string. If verbose, append library versions */
void usage(void);			/* Display usage */
int IsSwitch(char *pszArg);		/* Is this a command-line switch? */
int updateall(char *, char *);		/* Copy a set of files if newer */
int update(char *, char *);		/* Copy a file if newer */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK)/* In DOS it's defined, but always returns 0 */
int update_link(char *, char *);	/* Copy a link if newer */
#endif
int copyf(char *, char *);		/* Copy a file silently */
int copy(char *, char *);		/* Copy a file and display messages */
int mkdirp(const char *path, mode_t mode); /* Same as mkdir -p */

int exists(char *name);			/* Does this pathname exist? (TRUE/FALSE) */
int exist_file(char *); 		/* Does this file exist? (TRUE/FALSE) */
int file_empty(char *); 		/* Is this file empty? (TRUE/FALSE). */
					/* File must exist (else return FALSE)  */
int is_directory(char *);		/* Is name a directory? -> TRUE/FALSE */
int older(char *, char *);		/* Is file 1 older than file 2? */
time_t getmodified(char *);		/* Get time of file modification */
int copydate(char *to, char *from);	/* Copy the file date & time */

char *strgfn(const char *);		/* Get file name position */
void stcgfn(char *, const char *);	/* Get file name */
void stcgfp(char *, const char *);	/* Get file path */
void strmfp(char *, const char *, const char *);    /* Make file pathname */
void strsfp(const char *, char *, char *);          /* Split file pathname */
char *NewPathName(const char *path, const char *name); /* Create a new pathname */
/* zap functions options */
typedef struct zapOpts {
  int iFlags;
  char *pszPrefix;
} zapOpts;
/* zapOpts iFlags */
#define FLAG_VERBOSE	0x0001		/* Display the pathname operated on */
#define FLAG_NOEXEC	0x0002		/* Do not actually execute */
#define FLAG_RECURSE	0x0004		/* Recursive operation */
#define FLAG_NOCASE	0x0008		/* Ignore case */
#define FLAG_FORCE	0x0010		/* Force operation on read-only files */
int zapFile(const char *path, zapOpts *pzo); /* Delete a file */
int zapFileM(const char *path, int iMode, zapOpts *pzo); /* Faster */
int zapDir(const char *path, zapOpts *pzo);  /* Delete a directory */
int zapDirM(const char *path, int iMode, zapOpts *pzo); /* Faster */

/* Global program name variables */
char *program;	/* This program basename, with extension in Windows */
char *progcmd;	/* This program invokation name, without extension in Windows */
int GetProgramNames(char *argv0);	/* Initialize the above two */
int printError(char *pszFormat, ...);	/* Print errors in a consistent format */

/* Exit front end, with support for the optional final pause */
void do_exit(int n) {
  if (iPause) {
    fflush(stdin);
    printf("Press Enter to continue... ");
    fflush(stdout);
    getchar();
  }
  exit(n);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    main						      |
|									      |
|   Description:    EXE program main initialization routine		      |
|									      |
|   Parameters:     int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns:	    The return code to pass to the OS.			      |
|									      |
|   History:								      |
|									      |
|    1986-04-01 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  int iArg;			/* Argument index */
  char *arg;
#if defined(_MSDOS) || defined(_WIN32)
  size_t len;
#endif
  char *target;
  int nErrors = 0;
  int iExit = 0;

  /* Extract the program names from argv[0] */
  GetProgramNames(argv[0]);

  for (iArg = 1; iArg<argc; iArg += 1) {
    arg = argv[iArg];
    if (IsSwitch(arg)) {
      char *opt = arg + 1;
      DEBUG_PRINTF(("Switch = %s\n", arg));
      if (streq(arg, "--")) {	    /* Force end of switches */
	iArg += 1;
	break;
      }
      if (   streq(opt, "h")	    /* Display usage */
	  || streq(opt, "help")	    /* The historical name of that switch */
	  || streq(opt, "-help")
	  || streq(opt, "?")) {
	usage();
      }
  #ifdef _WIN32
      if (   streq(opt, "A")
	  || streq(opt, "-ansi")) {   /* Force encoding output with the ANSI code page */
	cp = CP_ACP;
	continue;
      }
  #endif
      DEBUG_CODE(
      if (   streq(opt, "d")	    /* Debug mode on */
	  || streq(opt, "debug")	    /* The historical name of that switch */
	  || streq(opt, "-debug")) {
	DEBUG_MORE();
	iVerbose = TRUE;
	if (iVerbose) printf("Debug mode on.\n");
	continue;
      }
      )
      if (   streq(opt, "e")	    /* Erase mode on */
	  || streq(opt, "-erase")) {
	iErase = TRUE;
	if (iVerbose) printf("Erase mode on.\n");
	continue;
      }
      if (   streq(opt, "E")	    /* NoEmpty mode on */
	  || streq(opt, "noempty")    /* The historical name of that switch */
	  || streq(opt, "-noempty")) {
	copyempty = FALSE;
	if (iVerbose) printf("NoEmpty mode on.\n");
	continue;
      }
      if (   streq(opt, "f")	    /* Freshen mode on */
	  || streq(opt, "-freshen")) {
	fresh = 1;
	if (iVerbose) printf("Freshen mode on.\n");
	continue;
      }
      if (   streq(opt, "F")	    /* Force mode on */
	  || streq(opt, "-force")) {
	force = 1;
	if (iVerbose) printf("Force mode on.\n");
	continue;
      }
      if (   streq(opt, "i")	    /* Case-insensitive pattern matching */
	  || streq(opt, "-ignorecase")) {
	iFnmFlag |= FNM_CASEFOLD;
	if (iVerbose) printf("Case-insensitive pattern matching.\n");
	continue;
      }
      if (   streq(opt, "k")	    /* Case-sensitive pattern matching */
	  || streq(opt, "-casesensitive")) {
	iFnmFlag &= ~FNM_CASEFOLD;
	if (iVerbose) printf("Case-sensitive pattern matching.\n");
	continue;
      }
  #ifdef _WIN32
      if (   streq(opt, "O")
	  || streq(opt, "-oem")) {    /* Force encoding output with the OEM code page */
	cp = CP_OEMCP;
	continue;
      }
  #endif
      if (   streq(opt, "p")	    /* Final Pause on */
	  || streq(opt, "-pause")) {
	iPause = 1;
	if (iVerbose) printf("Final Pause on.\n");
	continue;
      }
      if (   streq(opt, "P")	    /* Show file copy progress */
	  || streq(opt, "-progress")) {
	if (isConsole(fileno(stdout))) { /* Only do it when outputing to the console */
	  iProgress = 1;
	  if (iVerbose) printf("Show file copy progress.\n");
	}
	continue;
      }
      if (   streq(opt, "q")	    /* Quiet/Nologo mode on */
	  || streq(opt, "-quiet")
	  || streq(opt, "nologo")) {  /* The historical name of that switch */
	iVerbose = FALSE;
	continue;
      }
      if (   streq(opt, "r")	    /* Recursive update */
	  || streq(opt, "-recurse")) {
	iRecur = 1;
	if (iVerbose) printf("Recursive update.\n");
	continue;
      }
      if (   streq(opt, "S")     /* Show dest instead of source */
	  || streq(opt, "-showdest")) {
	show = 1;
	if (iVerbose) printf("Show destination files names.\n");
	continue;
      }
  #ifdef _WIN32
      if (   streq(opt, "U")
	  || streq(opt, "-utf8")) {   /* Force encoding output with the UTF-8 code page */
	cp = CP_UTF8;
	continue;
      }
  #endif
      if (   streq(opt, "v")	    /* Verbose mode on */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	continue;
      }
      if (   streq(opt, "V")	    /* -V: Display the version */
	  || streq(opt, "-version")) {
	printf("%s\n", version(1));
	exit(0);
      }
      if (   streq(opt, "X")	    /* NoExec/Test mode on */
	  || streq(opt, "-noexec")
	  || streq(opt, "t")) {	    /* The historical name of that switch */
	test = 1;
	if (iVerbose) printf("NoExec mode on.\n");
	continue;
      }
      fprintf(stderr, "Warning: Unrecognized switch %s ignored.\n", arg);
    }
  }

  if ( (argc - iArg) < 1 ) {
    fprintf(stderr, "Error: Not enough arguments.\n");
    do_exit(1);
  }

  buffer = malloc(BUFFERSIZE);	/* Allocate memory for copying */
  if (!buffer) {
    fprintf(stderr, "Error: Not enough memory.\n");
    do_exit(1);
  }

  DEBUG_PRINTF(("Size of size_t = %d bits\n", (int)(8*sizeof(size_t))));
  DEBUG_PRINTF(("Size of off_t = %d bits\n", (int)(8*sizeof(off_t))));

  target = argv[--argc];	/* The last argument is the target */
  DEBUG_PRINTF(("Target = %s\n", target));
#if defined(_MSDOS) || defined(_WIN32)
  /* Workaround for a command.com or cmd.exe bug */
  len = strlen(target);
  if (len && (target[len-1] == '"')) {
    target[len-1] = DIRSEPARATOR_CHAR;
    DEBUG_PRINTF(("Changing the trailing quote to a backslash: %s\n", target));
  }
  /* Avoid multiple errors when writing to an inexistant or disconnected drive */
  if (target[0] && (target[1] == ':')) {
    struct stat s;
    char szDrive[4];
    int iErr;
    sprintf(szDrive, "%c:\\", target[0]);
    iErr = stat(szDrive, &s);
    if (iErr) {
      printError("Error: Cannot access drive %c: %s", target[0], strerror(errno));
      do_exit(1);
    }
  }
#endif

  for ( ; iArg < argc; iArg++) { /* For every source file before that */
    arg = argv[iArg];
    nErrors += updateall(arg, target);
  }

  if (nErrors) { /* Display a final summary, as the errors may have scrolled up beyond view */
    printError("Error: %d file(s) failed to be updated", nErrors);
    iExit = 1;
  }

  do_exit(iExit);
  return iExit;
}

/* Get the program version string, optionally with libraries versions */
char *version(int iLibsVer) {
  char *pszMainVer = PROGRAM_VERSION " " PROGRAM_DATE " " EXE_OS_NAME DEBUG_VERSION;
  char *pszVer = NULL;
  if (iLibsVer) {
    char *pszLibVer = ""
#if defined(_MSVCLIBX_H_)	/* If used MsvcLibX */
#include "msvclibx_version.h"
	  " ; MsvcLibX " MSVCLIBX_VERSION
#endif
#if defined(__SYSLIB_H__)	/* If used SysLib */
#include "syslib_version.h"
	  " ; SysLib " SYSLIB_VERSION
#endif
    ;
    pszVer = (char *)malloc(strlen(pszMainVer) + strlen(pszLibVer) + 1);
    if (pszVer) sprintf(pszVer, "%s%s", pszMainVer, pszLibVer);
  }
  if (!pszVer) pszVer = pszMainVer;
  return pszVer;
}

void usage(void)
    {
    printf("update version %s - Update files based on their time stamps\n\
\n\
Usage: update [SWITCHES] FILES DIRECTORY\n\
       update [SWITCHES] FILES DIRECTORY" DIRSEPARATOR_STRING "NEWDIR" DIRSEPARATOR_STRING "\n\
       update [SWITCHES] FILE  DIRECTORY[" DIRSEPARATOR_STRING "NEWNAME]\n\
\n\
Files:          FILE1 [FILE2 ...]\n\
                Wildcards are allowed in source files pathnames.\n\
\n\
Switches:\n\
  --            End of switches\n"
#ifdef _WIN32
"\
  -A|--ansi     Force encoding the output using the ANSI character set.\n"
#endif
#ifdef _DEBUG
"\
  -d|--debug    Output debug information.\n"
#endif
"\
  -e|--erase    Erase mode. Delete destination files not in the source.\n\
  -E|--noempty  Noempty mode. Don't copy empty file.\n\
", version(0));

    printf("\
  -f|--freshen  Freshen mode. Update only files that exist in both directories.\n\
  -F|--force    Force mode. Overwrite read-only files.\n\
  -h|--help|-?  Display this help screen.\n\
  -i|--ignorecase    Case-insensitive pattern matching. Default for DOS/Windows.\n\
  -k|--casesensitive Case-sensitive pattern matching. Default for Unix.\n"
#ifdef _WIN32
"\
  -O|--oem      Force encoding the output using the OEM character set.\n"
#endif
"\
  -p|--pause    Pause before exit.\n\
  -P|--progress Display the file copy progress. Useful with very large files.\n\
  -q|--nologo   Quiet mode. Don't display anything.\n\
  -r|--recurse  Recursively update all subdirectories.\n\
  -S|--showdest Show the destination files names. Default: The sources names.\n"
#ifdef _WIN32
"\
  -U|--utf8     Force encoding the output using the UTF-8 character encoding.\n"
#endif
"\
  -v|--verbose  Verbose node. Display extra status information.\n\
  -V|--version  Display this program version and exit.\n\
  -X|-t         Noexec mode. Display the files that need to be copied.\n\
\n"
#ifdef _MSDOS
"Author: Jean-Francois Larvoire"
#else
"Author: Jean-François Larvoire"
#endif
" - jf.larvoire@hpe.com or jf.larvoire@free.fr\n"
#ifdef __unix__
"\n"
#endif
);

    do_exit(0);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsSwitch						      |
|                                                                             |
|   Description:    Test if an argument is a command-line switch.             |
|                                                                             |
|   Parameters:     char *pszArg	    Would-be argument		      |
|                                                                             |
|   Return value:   TRUE or FALSE					      |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int IsSwitch(char *pszArg)
    {
    return (   (*pszArg == '-')
#ifndef __unix__
            || (*pszArg == '/')
#endif
           ); /* It's a switch */
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    updateall						      |
|                                                                             |
|   Description:    Update all files from source dir to dest dir              |
|                                                                             |
|   Parameters:     char *p1	    Source path. Wildcards allowed for files. |
|                   char *p2	    Destination directory		      |
|                                                                             |
|   Return value:   The number of errors encountered. 0=Success               |
|                                                                             |
|   Notes:	    Copy files, except if a newer version is already there.   |
|                                                                             |
|   History:								      |
|    2011-09-06 JFL Added the ability to update to a file with a differ. name.|
*                                                                             *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#endif
int updateall(char *p1,             /* Wildcard * and ? are interpreted */
	      char *p2)
    {
#if _MSDOS /* In DOS, the pathname size is very small, it can be auto-allocated on the stack */
    char path0[PATHNAME_SIZE], path1[PATHNAME_SIZE], path2[PATHNAME_SIZE];
    char path[PATHNAME_SIZE], name[PATHNAME_SIZE];
    char fullpathname[PATHNAME_SIZE], path3[PATHNAME_SIZE];
#else /* In all other OSs, pathname size is rather large. It'll be dynamically allocated from the heap */
    char *path0, *path1, *path2;
    char *path, *name;
    char *fullpathname, *path3;
#endif
    char *ppath, *pname;
    DIR *pDir;
    struct dirent *pDE;
    char *pattern;
    int err;
    int nErrors = 0;
    int iTargetDirExisted;
    zapOpts zo = {FLAG_VERBOSE, "- "};
    if (iRecur) zo.iFlags |= FLAG_RECURSE;
    if (test) zo.iFlags |= FLAG_NOEXEC;
    if (force) zo.iFlags |= FLAG_FORCE;

    DEBUG_ENTER(("updateall(\"%s\", \"%s\");\n", p1, p2));

#ifndef _MSDOS
    path0 = malloc(PATHNAME_SIZE);
    path1 = malloc(PATHNAME_SIZE);
    path2 = malloc(PATHNAME_SIZE);
    path3 = malloc(PATHNAME_SIZE);
    path = malloc(PATHNAME_SIZE);
    name = malloc(PATHNAME_SIZE);
    fullpathname = malloc(PATHNAME_SIZE);
    if ((!path0) || (!path1) || (!path2) || (!path3) || (!path) || (!name) || (!fullpathname)) {
      printError("Error: Not enough memory");
      nErrors += 1;
      goto cleanup_and_return;
    }
#endif

    strcpy(path0, p1);
    if (is_directory(p1)) {          /* If yes, assume "\*.*"  */
      pattern = PATTERN_ALL;
    } else {
      char *pSlash;
      pSlash = strrchr(path0, DIRSEPARATOR_CHAR);
      if (pSlash) {
      	pattern = pSlash + 1;
      	if (!*pattern) {
      	  pattern = PATTERN_ALL; /* There was a trailing / */
      	} else {
      	  pattern = p1 + (pattern - path0); /* Use the original in p1, as the copy in path0 may be overwritten below */
      	}
      	while ((pSlash > path0) && (*(pSlash-1) == DIRSEPARATOR_CHAR)) pSlash -= 1; /* Remove extra consecutive / */
      	*pSlash++ = '\0';
      	if (   (!path0[0])
#if defined(_MSDOS) || defined(_WIN32)
      	    || ((path0[1] == ':') && (!path0[2]) && (p1[2])) /* p1 was like "D:\", and path0 like "D:" */
#endif /* defined(_MSDOS) || defined(_WIN32) */
      	   ) { /* If this was the root directory (possibly on another drive in DOS/Windows) */
      	  /* Then restore that trailing / indicating the root dir */
      	  *(pSlash-1) = DIRSEPARATOR_CHAR;
	  *pSlash++ = '\0';
	}
      } else {
      	pattern = p1;
      	strcpy(path0, ".");
      }
    }

    if (iVerbose) {
      DEBUG_PRINTF(("// ")); /* If debug is on, print the debug indent, then a comment marker without a linefeed */
      printf("Update %s from %s to %s\n", pattern, path0, p2);
    }

    /* Check if the target is a file or directory name */
    ppath = p2;
    pname = NULL; /* Implies using pDE->d_name */
    strsfp(p2, path, name);
    if (name[0] && is_directory(path) && (!is_directory(p2)) && (!strpbrk(p1, "*?"))) {
      ppath = path;
      pname = name;
      DEBUG_PRINTF(("// The target is file %s in directory %s\n", pname, ppath));
    } else {
      DEBUG_PRINTF(("// The target is directory %s\n", ppath));
    }
    iTargetDirExisted = is_directory(ppath);

    /* Note: Scan the source directory even in the absence of wildcards.
       In Windows, this makes sure that the copy has the same case as
       the source, even if the command-line argument has a different case. */

    /* Scan all files that match the wild cards */
    pDir = opendir(path0);
    if (!pDir) {
      printError("Error: can't open directory \"%s\": %s", path0, strerror(errno));
      nErrors += 1;
      goto cleanup_and_return;
    }
    while ((pDE = readdir(pDir))) {
      DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
      if (   (pDE->d_type != DT_REG)
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      	  && (pDE->d_type != DT_LNK)
#endif
      	 ) continue;	/* We want only files or links */
      if (fnmatch(pattern, pDE->d_name, iFnmFlag) == FNM_NOMATCH) continue;
      strmfp(path1, path0, pDE->d_name);  /* Compute source path */
      DEBUG_PRINTF(("// Found %s\n", path1));
      strmfp(path2, ppath, pname?pname:pDE->d_name); /* Append it to directory p2 too */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      if (pDE->d_type == DT_LNK) {
	err = update_link(path1, path2); /* Displays error messages on stderr */
      }
      else
#endif
      {
      	err = update(path1, path2); /* Does not display error messages on stderr */
	if (err) {
	  printError("Error: Failed to create \"%s\". %s", path2, strerror(errno));
	}
      }
      if (err) {
      	nErrors += 1;
      	/* Continue the directory scan, looking for other files to update */
      }
    }
    closedir(pDir);

    /* Scan target files that might be erased */
    if (iErase) {
      fullpath(path2, p2, PATHNAME_SIZE); /* Build absolute pathname of source */
      pDir = opendir(p2);
      if (pDir) {
	while ((pDE = readdir(pDir))) {
	  struct stat sStat;
	  if (streq(pDE->d_name, ".")) continue;    /* Skip the . directory */
	  if (streq(pDE->d_name, "..")) continue;   /* Skip the .. directory */
	  DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
	  if (fnmatch(pattern, pDE->d_name, iFnmFlag) == FNM_NOMATCH) continue;
	  strmfp(path3, path2, pDE->d_name);  /* Compute the target file pathname */
	  DEBUG_PRINTF(("// Found %s\n", path3));
	  strmfp(path1, path0, pDE->d_name); /* Compute the corresponding source file pathname */
	  if (access(path1, F_OK) == -1) { /* If that source file does not exist */
	    char *pszType = "file";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
	    err = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
	    err = -lstat(path3, &sStat); /* If error, iErr = 1 = # of errors */
#endif
	    if (err) {
	      printError("Error: Can't stat \"%s\"", path1);
	      nErrors += 1;
	      continue;
	    }
	    switch (pDE->d_type) {
	      case DT_DIR:
		err = zapDirM(path3, sStat.st_mode, &zo);
		nErrors += err;
		break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
	      case (DT_LNK):
	      	pszType = "link";
		// Fall through
#endif
	      case DT_REG:
	      	err = zapFileM(path3, sStat.st_mode, &zo);
		if (err) {
		  printError("Error: Failed to remove %s \"%s\"", pszType, path3);
		  nErrors += 1;
		}
		break;
	      default:
		printError("Error: Can't delete \"%s\"", path3);
		nErrors += 1;
		break;
	    }
	  }
	}
      }
    }

    if (iRecur) { /* Parse the directory again, looking for actual directories (not junctions nor symlinkds) */
      pDir = opendir(path0);
      if (!pDir) {
	printError("Error: Can't open directory \"%s\": %s", path0, strerror(errno));
      	nErrors += 1;
        goto cleanup_and_return;
      }
      while ((pDE = readdir(pDir))) {
      	int p2_exists, p2_is_dir;

	DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
	if (pDE->d_type != DT_DIR) continue;	/* We want only directories */
	if (streq(pDE->d_name, ".") || streq(pDE->d_name, "..")) continue; /* These are not real subdirs */

	strmfp(path3, path0, pDE->d_name); /* Source subdirectory path: path3 = path0/d_name */
	fullpath(fullpathname, path3, PATHNAME_SIZE); /* Build absolute pathname of source dir */
	strmfp(path1, path3, pattern);	   /* Search pattern: path1 = path3/pattern */
	strmfp(path2, ppath, pDE->d_name); /* Destination subdirectory path: path2 = ppath/dname */
	strcat(path2, DIRSEPARATOR_STRING);/* Make sure the target path gets created if needed */

	p2_exists = exists(path2);
	p2_is_dir = is_directory(path2);
	if ((!p2_exists) || (!p2_is_dir)) {
	  if (test == 1) {
	    if (iVerbose) {
	      DEBUG_PRINTF(("// "));
	      printf("Would copy directory %s\\\n", fullpathname);
	    }
	    /* printf("%s\\\n", fullpathname); // 2015-01-12 JFL Don't display the directory name,
		    as we're interested only in its inner files, and there may be none to copy */
	  } else {
	    if (p2_exists && !p2_is_dir) {
	      err = zapFile(path2, &zo); /* Delete the conflicting file/link */
	      if (err) {
		printError("Error: Failed to remove \"%s\"", path2);
		nErrors += 1;
		continue;	/* Try updating something else */
	      }
	      p2_exists = FALSE;
	    }
	    /* 2015-01-12 JFL Don't create the directory now, as it may never be needed,
				if there are no files that match the input pattern */
	    /* 2018-05-31 JFL Actually do it, but only if the copyempty flag is set */
	    if (copyempty && !p2_exists) { /* Create the missing target directory */
	      printf("%s\\\n", fullpathname);
	      if (!test) {
	      	err = mkdirp(path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (err) {
		  printError("Error: Failed to create directory \"%s\". %s", path2, strerror(errno));
		  nErrors += 1;
		  continue;	/* Try updating something else */
		}
	      }
	    }
	  }
	}

	err = updateall(path1, path2);
	if (err) nErrors += err;

	if (!p2_exists) { /* If we did create the target subdir */
	  copydate(path2, path3); /* Make sure the directory date matches too */
	}
      }
      closedir(pDir);
    }

    if ((!iTargetDirExisted) && is_directory(ppath)) { /* If we did create the target dir */
      copydate(ppath, path0); /* Make sure the directory date matches too */
    }

cleanup_and_return:
#ifndef _MSDOS
    free(path0); free(path1); free(path2); free(path3); free(path); free(name); free(fullpathname);
#endif
    RETURN_INT(nErrors);
    }
#ifdef _MSC_VER
#pragma warning(default:4706)
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    update						      |
|                                                                             |
|   Description:    Update one file					      |
|                                                                             |
|   Parameters:     char *p1	    Source file pathname                      |
|                   char *p2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success, else Error				      |
|                                                                             |
|   Notes:	    Copy the file, except if a newer version is already there.|
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Updated the test mode support, and fixed a bug when       |
|                   using both the test mode and the showdest mode.           |
*                                                                             *
\*---------------------------------------------------------------------------*/

int update(char *p1,	/* Both names must be complete, without wildcards */
           char *p2)
    {
    int e;
    char name[PATHNAME_SIZE];
    struct stat sP2stat;
    char *p;

    DEBUG_ENTER(("update(\"%s\", \"%s\");\n", p1, p2));

    /* Get the pathname to display, before p2 is possibly modified by the test mode */
    p = p1;		/* By default, show the source file name */
    if (show) p = p2;	/* But in showdest mode, show the destination file name */

    /* In freshen mode, don't copy if the destination does not exist. */
    if (fresh && !exist_file(p2)) RETURN_CONST(0);

    /* In Noempty mode, don't copy empty file */
    if ( (copyempty == FALSE) && (file_empty(p1)) ) RETURN_CONST(0);

    /* If the target exists, make sure it's a file */
    e = lstat(p2, &sP2stat); /* Use lstat to avoid following links */
    if (e == 0) {
      zapOpts zo = {FLAG_VERBOSE | FLAG_RECURSE, "- "};
      if (test) zo.iFlags |= FLAG_NOEXEC;
      if (force) zo.iFlags |= FLAG_FORCE;
      if (S_ISDIR(sP2stat.st_mode)) {	/* If the target is a directory */
      	zo.iFlags |= FLAG_VERBOSE; /* Show what's deleted, beyond the obvious target itself */
      	e = zapDirM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
	if (test) p2 = ""; /* Trick older() to think the target is deleted */
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      } else if (S_ISLNK(sP2stat.st_mode)) { /* Else if it's a link */
      	zo.iFlags &= ~FLAG_VERBOSE; /* No need to show that that target is deleted */
	e = zapFileM(p2, sP2stat.st_mode, &zo);	/* Deletes the link, not its target. */
	if (test) p2 = ""; /* Trick older() to think the target is deleted */
#endif /* defined(S_ISLNK) */
      } /* Else the target is a plain file */
      if (e) {
      	printError("Failed to remove \"%s\"", p2);
      	RETURN_INT(e);
      }
    }

    /* In any mode, don't copy if the destination is newer than the source. */
    if (older(p1, p2)) RETURN_CONST(0);

    fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of source */
    if (test == 1)
        {
	if (iVerbose) {
	  DEBUG_PRINTF(("// "));
	  printf("Would copy file ");
	}
	printf("%s\n", name);
        RETURN_CONST(0);
        }

    if (!iVerbose) printf("%s\n", name);

    e = copy(p1, p2);

    RETURN_INT_COMMENT(e, (e?"Error\n":"Success\n"));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    update_link						      |
|                                                                             |
|   Description:    Update one symbolic link				      |
|                                                                             |
|   Parameters:     char *p1	    Source link pathname                      |
|                   char *p2	    Destination link pathname		      |
|                                                                             |
|   Return value:   0 = Success, else Error				      |
|                                                                             |
|   Notes:	    Copy the link, except if a newer version is already there.|
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Added support for the --force option.                     |
*                                                                             *
\*---------------------------------------------------------------------------*/

int exists(char *name) {
    int result;
    struct stat sstat;

    DEBUG_ENTER(("exists(\"%s\");\n", name));

    result = !lstat(name, &sstat); // Use lstat, as stat does not detect SYMLINKDs.

    RETURN_BOOL(result);
}

#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */

int is_link(char *name) {
    int result;
    int err;
    struct stat sstat;

    DEBUG_ENTER(("is_link(\"%s\");\n", name));

    err = lstat(name, &sstat); // Use lstat, as stat does not set S_IFLNK.
    result = ((err == 0) && (S_ISLNK(sstat.st_mode)));

    RETURN_BOOL(result);
}

/* Copy link p1 onto link p2, if and only if p1 is newer. */
int update_link(char *p1,	/* Both names must be complete, without wildcards */
                char *p2)
    {
    int err;
    char name[PATHNAME_SIZE];
    char target1[PATHNAME_SIZE];
    int iSize;
#if _MSVCLIBX_STAT_DEFINED
    struct stat sP1stat;
#endif
    struct stat sP2stat;
    int bP2Exists;
    int bP2IsLink;
    char path[PATHNAME_SIZE];
    char *p;

    DEBUG_ENTER(("update_link(\"%s\", \"%s\");\n", p1, p2));

    err = lstat(p2, &sP2stat); // Use lstat to avoid following links
    bP2Exists = (err == 0);
    bP2IsLink = (bP2Exists && (S_ISLNK(sP2stat.st_mode)));

    /* In freshen mode, don't copy if the destination does not exist. */
    if (fresh && !bP2IsLink) RETURN_CONST(0);

    /* In any mode, don't copy if the destination is newer than the source. */
    if (bP2IsLink && older(p1, p2)) RETURN_CONST(0);

    p = p1;		/* By default, show the source file name */
    if (show) p = p2;	/* But in showdest mode, show the destination file name */
    fullpath(name, p, PATHNAME_SIZE); /* Build absolute pathname of source */
    if (test == 1)
        {
	if (iVerbose) {
	  DEBUG_PRINTF(("// "));
	  printf("Would copy link ");
	}
	printf("%s\n", name);
        RETURN_CONST(0);
        }

    printf("%s\n", name);

    if (bP2Exists) { // Then the target has to be removed, even if it's a link
      zapOpts zo = {FLAG_VERBOSE | FLAG_RECURSE, "- "};
      if (test) zo.iFlags |= FLAG_NOEXEC;
      if (force) zo.iFlags |= FLAG_FORCE;
      // First, in force mode, prevent failures if the target is read-only
      if (force && !(sP2stat.st_mode & S_IWRITE)) {
      	int iMode = sP2stat.st_mode | S_IWRITE;
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", p2, iMode));
      	err = chmod(p2, iMode); /* Try making the target file writable */
      	DEBUG_PRINTF(("  return %d; // errno = %d\n", err, errno));
      }
      if (S_ISDIR(sP2stat.st_mode)) {
      	zo.iFlags |= FLAG_VERBOSE; /* Show what's deleted, beyond the obvious target itself */
	err = zapDirM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
      } else { // It's a file or a link
      	zo.iFlags &= ~FLAG_VERBOSE; /* No need to show that that target is deleted */
	err = zapFileM(p2, sP2stat.st_mode, &zo);	/* Then remove it */
      	if (err) printError("Error: Failed to remove \"%s\"", p2);
      }
      if (err) RETURN_INT(err);
    }

    strsfp(p2, path, NULL);
    if (!exists(path)) {
      err = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (err) {
      	printError("Error: Failed to create directory \"%s\". %s", path, strerror(errno));
	RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
      }
    }

    iSize = (int)readlink(p1, target1, sizeof(target1));
    if (iSize < 0) { /* This may fail for Linux Sub-System Symbolic Links */
      printError("Error: Failed to read link \"%s\"", p1);
      RETURN_INT(1);
    }
    DEBUG_PRINTF(("// Target1=\"%s\", iSize=%d\n", target1, iSize));

    // e = copy(p1, p2);
#if _MSVCLIBX_STAT_DEFINED
    err = lstat(p1, &sP1stat); // Use lstat to avoid following links
    if (sP1stat.st_ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
      err = junction(target1, p2);
    } else if (sP1stat.st_Win32Attrs & FILE_ATTRIBUTE_DIRECTORY) {
      err = symlinkd(target1, p2);
    } else
#endif
    err = symlink(target1, p2);
    if (!err) copydate(p2, p1);
    if (err) {
      printError("Error: Failed to create link \"%s\". %s", p2, strerror(errno));
    }

    RETURN_INT_COMMENT(err, (err?"Error\n":"Success\n"));
    }

#endif // !defined(S_ISLNK)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copyf						      |
|                                                                             |
|   Description:    Copy one file					      |
|                                                                             |
|   Parameters:     char *name1	    Source file pathname                      |
|                   char *name2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success						      |
|                   1 = Read error					      |
|                   2 = Write error					      |
|                                                                             |
|   Notes:	    Both names must be correct, and paths must exist.	      |
|                                                                             |
|   History:								      |
|    2013-03-15 JFL Added resiliency:					      |
|                   When reading fails to start, avoid deleting the target.   |
|                   In case of error later on, delete incomplete copies.      |
|    2016-05-10 JFL Added support for the --force option.                     |
*                                                                             *
\*---------------------------------------------------------------------------*/

int copyf(char *name1,		    /* Source file to copy from */
          char *name2)		    /* Destination file to copy to */
    {
    FILE *pfs, *pfd;	    /* Source & destination file pointers */
    int hsource;	    /* Source handle */
    off_t filelen;	    /* File length */
    size_t tocopy;	    /* Number of bytes to copy in one pass */
    int iShowCopying = FALSE;
    int nAttempt = 1;	    /* Force mode allows retrying a second time */
    off_t offset;
    int iWidth = 0;	    /* Number of characters in the iProgress output */
    char *pszUnit = "B";    /* Unit used for iProgress output */
    long lUnit = 1;	    /* Number of bytes for 1 iProgress unit */

    DEBUG_ENTER(("copyf(\"%s\", \"%s\");\n", name1, name2));
    if (iVerbose
#ifdef _DEBUG
        && !iDebug
#endif
	) {
	  iShowCopying = TRUE;
	  printf("\tCopying %s", name1);
	}

    pfs = fopen(name1, "rb");
    if (!pfs) {
      if (iShowCopying) printf("\n");
      RETURN_INT_COMMENT(1, ("Can't open input file\n"));
    }
    hsource = fileno(pfs);

    filelen = _filelength(hsource);
    /* Read 1 byte to test access rights. This avoids destroying the target
       if we don't have the right to read the source. */
    if (filelen && !fread(buffer, 1, 1, pfs)) {
      if (iShowCopying) printf("\n");
      RETURN_INT_COMMENT(1, ("Can't read the input file\n"));
    }
    fseek(pfs, 0, SEEK_SET);
retry_open_targetfile:
    pfd = fopen(name2, "wb");
    if (!pfd) {
      if ((errno == EACCES) && (nAttempt == 1) && force) {
      	struct stat sStat = {0};
      	int iErr = stat(name2, &sStat);
      	int iMode = sStat.st_mode | _S_IWRITE;
      	DEBUG_PRINTF(("chmod(%p, 0x%X);\n", name2, iMode));
      	iErr = chmod(name2, iMode); /* Try making the target file writable */
      	DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
      	if (!iErr) {
	  nAttempt += 1;
	  goto retry_open_targetfile;
	}
      }
      if (iShowCopying) printf("\n");
      fclose(pfs);
      RETURN_INT_COMMENT(2, ("Can't open the output file\n"));
    }
    /* hdest = fileno(pfd); */

    if (iShowCopying) printf(" : %"PRIdPTR" bytes\n", filelen);

    if (iProgress) {
      if (filelen > (100*1024L*1024L)) {
      	lUnit = 1024L*1024L;
      	pszUnit = "MB";
      } else if (filelen > (100*1024L)) {
      	lUnit = 1024L;
      	pszUnit = "KB";
      }
    }

    for (offset = 0; offset < filelen; offset += tocopy) {
      off_t remainder = filelen - offset;
      tocopy = (size_t)min(BUFFERSIZE, remainder);
      
      if (iProgress) {
      	int pc = (int)((offset * 100) / filelen);
      	iWidth = printf("%3d%% (%"PRIdPTR"%s/%"PRIdPTR"%s)\r", pc, (offset/lUnit), pszUnit, (filelen/lUnit), pszUnit);
      }
      
      XDEBUG_PRINTF(("fread(%p, %"PRIuPTR", 1, %p);\n", buffer, tocopy, pfs));
      if (!fread(buffer, tocopy, 1, pfs)) {
	if (iProgress && iWidth) printf("\n");
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(1, ("Can't read the input file. Deleted the partial copy.\n"));
      }
      if (!fwrite(buffer, tocopy, 1, pfd)) {
	if (iProgress && iWidth) printf("\n");
	fclose(pfs);
	fclose(pfd);
	unlink(name2); /* Avoid leaving an incomplete file on the target */
        RETURN_INT_COMMENT(2, ("Can't write the output file. Deleted the partial copy.\n"));
      }
    }
    if (iProgress && iWidth) printf("%*s\r", iWidth, "");

    fclose(pfs);
    fclose(pfd);

    copydate(name2, name1);	/* & give the same date than the source file */

    DEBUG_PRINTF(("// File %s mode is read%s\n", name2,
			access(name2, 6) ? "-only" : "/write"));

    RETURN_INT_COMMENT(0, ("File copy complete.\n"));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copy						      |
|                                                                             |
|   Description:    Copy one file, creating the target directory if needed    |
|                                                                             |
|   Parameters:     char *name1	    Source file pathname                      |
|                   char *name2	    Destination file pathname		      |
|                                                                             |
|   Return value:   0 = Success, else error and errno set		      |
|                                                                             |
|   Notes:	    Both names must be correct, and paths must exist.	      |
|                                                                             |
|   History:								      |
|    2016-05-10 JFL Compiled-out the error messages output. Another error     |
|                   message is displayed by the caller, and having both is    |
|                   confusing. To do: Build an error message string, and      |
|                   pass it back to the caller.                               |
*                                                                             *
\*---------------------------------------------------------------------------*/

int copy(char *name1, char *name2)
    {
    int e;
    char path[PATHNAME_SIZE];

    strsfp(name2, path, NULL);
    if (!exists(path)) {
      e = mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (e) {
      	printError("Error: Failed to create directory \"%s\". %s", path, strerror(errno));
      	return e;
      }
    }

    e = copyf(name1, name2);
#if NEEDED
    switch (e)
        {
        case 0:
            break;
        case 1:
	    printError("Error reading from file \"%s\"", name1);
            break;
        case 2:
	    printError("Error writing to file \"%s\"", name2);
            break;
        default:
            break;
        }
#endif
    return(e);
    }

/******************************************************************************
*									      *
*	File information						      *
*									      *
******************************************************************************/

int exist_file(char *name)	/* Does this file exist? (TRUE/FALSE) */
    {
    FILE *pf;

    DEBUG_ENTER(("exist_file(\"%s\");\n", name));

    pf = fopen(name, "r");
    if (pf)
	{
	fclose(pf);
	RETURN_CONST(TRUE);
	}

    RETURN_CONST(FALSE);
    }

int file_empty(char *name)	/* Is this file empty? (TRUE/FALSE) */
    {
    FILE *pf;
    int	 hfile;
    off_t lon;

    DEBUG_ENTER(("file_empty(\"%s\");\n", name));

    pf = fopen(name, "r");
    if (pf)
	{
	DEBUG_PRINTF(("// File %s exists\n", name));
	hfile = fileno(pf);
	lon = _filelength(hfile);
	fclose(pf);
	RETURN_BOOL(lon == 0);
	}

    RETURN_BOOL_COMMENT(FALSE, ("File %s does not exist\n", name));
    }

int is_directory(char *name)	/* Is name a directory? -> TRUE/FALSE */
    {				/* Les carateres * et ? ne sont pas acceptes */
    int result;
    int err;
    struct stat sstat;

    DEBUG_ENTER(("is_directory(\"%s\");\n", name));

    if (strchr(name, '?') || strchr(name, '*'))
	{ /* Wild cards not allowed */
	RETURN_CONST_COMMENT(FALSE, ("Directory %s does not exist\n", name));
	}

    err = lstat(name, &sstat); // Use lstat, as stat does not detect SYMLINKDs.

    result = ((err == 0) && (sstat.st_mode & S_IFDIR));

    RETURN_BOOL_COMMENT(result, ("Directory %s %s\n", name, result ? "exists"
								   : "does not exist"));
    }

int older(char *p1, char *p2)	/* Is file p1 older than file p2? */
    {
    time_t l1, l2;

    DEBUG_ENTER(("older(\"%s\", \"%s\");\n", p1, p2));

    l2 = getmodified(p2);
    if (l2 == 0L)
	{	      /* p2 does not exist */
	RETURN_BOOL_COMMENT(FALSE, ("File %s is newer than missing %s\n", p1, p2));
	}

    l1 = getmodified(p1);

    RETURN_BOOL_COMMENT(l1 <= l2, ("File %s is %s than file %s\n",
				   p1,
				   (l1 <= l2) ? "older" : "newer",
				   p2));
    }

#define CAST_WORD(u) (*(WORD *)&(u))

time_t getmodified(char *name) {
  int err;
  struct stat sstat;
  time_t result = 0L; /* Return 0 = invalid time for missing file */

  if (name && *name) {
    err = lstat(name, &sstat);
    if (!err) result = sstat.st_mtime;
  }

  DEBUG_PRINTF(("// File \"%s\" date/time = %lX\n", name, (long)result));

  return result;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    mkdirp						      |
|									      |
|   Description     Create a directory, and all parent directories as needed. |
|									      |
|   Parameters      Same as mkdir					      |
|									      |
|   Returns	    Same as mkdir					      |
|									      |
|   Notes	    Same as mkdir -p					      |
|		    							      |
|   History								      |
|    1990s      JFL Created this routine in update.c			      |
|    2017-10-04 JFL Improved the error handling, stopping at the first error. |
|		    Avoid testing access repeatedly if we know it'll fail.    |
|    2017-10-06 JFL Added the iVerbose arguement.			      |
|    2018-05-31 JFL Bug fix: This worked, but returned an error, if the path  |
|		     contained a trailing [back]slash.			      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS
#pragma warning(disable:4100) /* Ignore the "unreferenced formal parameter" warning */
#endif

int isdir(const char *pszPath) {
  struct stat sstat;
  int iErr = lstat(pszPath, &sstat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr) return 0;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(sstat.st_mode)) {
    char *pszReal = realpath(pszPath, NULL);
    int iRet = 0; /* If realpath failed, this is a dangling link, so not a directory */
    if (pszReal) iRet = isdir(pszReal);
    return iRet;
  }
#endif
  return S_ISDIR(sstat.st_mode);
}

/* Create one directory */
int mkdir1(const char *pszPath, mode_t pszMode) {
#ifndef HAS_MSVCLIBX
  DEBUG_PRINTF(("mkdir(\"%s\", 0x%X);\n", pszPath, pszMode));
#endif
  return mkdir(pszPath, pszMode);
}

/* Create all parent directories */
int mkdirp(const char *pszPath0, mode_t pszMode) {
  char *pszPath = strdup(pszPath0);
  int iErr = 0; /* Assume success */
  int iSkipTest = FALSE;
  DEBUG_ENTER(("mkdirp(\"%s\", 0x%X);\n", pszPath, pszMode));
  if (pszPath) {
    char c;
    char *pc = pszPath;
    if (pc[0] && (pc[1] == ':') && (pc[2] == DIRSEPARATOR_CHAR)) pc += 2; /* Skip the drive if absolute path */
    for (c = pc[0]; c; ) { /* Repeat for all components in the path */
      while (*pc == DIRSEPARATOR_CHAR) pc++; ; /* Skip leading slashes if absolute path */
      while (*pc && (*pc != DIRSEPARATOR_CHAR)) pc++; /* Skip the file or dir name */
      c = *pc; /* Either NUL or / or \ */
      *pc = '\0'; /* Trim pszPath */
      if (iSkipTest || !isdir(pszPath)) { /* If the intermediate path does not exist */
	iErr = mkdir1(pszPath, pszMode); /* Then create it. */
	if (iErr) break; /* No need to go further if this failed */
	iSkipTest = TRUE; /* We know future existence tests will fail */
      }
      *pc = c; /* Restore pszPath */
      if (c && !pc[1]) break; /* This was the trailing [back]slash */
    }
  }
  free(pszPath);
  RETURN_INT_COMMENT(iErr, (iErr ? "Failed. errno=%d - %s\n" : "Success\n", errno, strerror(errno)));
}

#ifdef _MSDOS
#pragma warning(default:4100)
#endif

/******************************************************************************
*									      *
*	Lattice C emulation						      *
*									      *
******************************************************************************/

char *strgfn(const char *pathname)		    /* Get file name position */
    {
    char *pc;

    /* Search for the end of the path */

    pc = strrchr(pathname, DIRSEPARATOR_CHAR);
    if (pc)
	{
	pc += 1;    /* Skip the \ */
	}
    else
	{
	pc = strrchr(pathname, ':');
	if (pc)
	    {
	    pc += 1;	/* Skip the : */
	    }
	else
	    {
	    pc = (char *)pathname;  /* There is just no path */
	    }
	}

    return pc;
    }

void stcgfn(char *name, const char *pathname)	    /* Get file name */
    {
    strcpy(name, strgfn(pathname)); /* Copy the name part of the pathname */

    return;
    }

void stcgfp(char *path, const char *pathname)	    /* Get file path */
    {
    char *pc;
    size_t n;

    pc = strgfn(pathname);
    n = pc - pathname;		    /* Size of the pathname */
    if (n && (pathname[n-1]==DIRSEPARATOR_CHAR))
	n -= 1; 		    /* Skip back the trailing \ if any */

    strncpy(path, pathname, n);     /* Copy the path part of the pathname */
    path[n] = '\0';

    return;
    }

void strmfp(char *pathname, const char *path, const char *name)   /* Make file pathname */
    {
    size_t l;
    DEBUG_ENTER(("strmfp(%p, \"%s\", \"%s\");\n", pathname, path, name));

    strcpy(pathname, path);
    l = strlen(path);
    if (   (l > 0)
	&& (path[l-1] != DIRSEPARATOR_CHAR)
	&& (path[l-1] != ':')
       )
	{
	  strcat(pathname, DIRSEPARATOR_STRING);
	}
    strcat(pathname, name);
    RETURN_COMMENT(("\"%s\"\n", pathname));
    }

/* 2014-02-21 JFL Made name optional */
void strsfp(const char *pathname, char *path, char *name)   /* Split file pathname */
    {
    const char *p;
    DEBUG_ENTER(("strsfp(\"%s\", %p, %p);\n", pathname, path, name));
    p = strrchr(pathname, DIRSEPARATOR_CHAR);
    if (!p) p = strrchr(pathname, ':');
    if (p) {
      size_t n = p-pathname;
      strncpy(path, pathname, n);
      path[n] = '\0';
      p += 1; /* The name following the last \ or : */
      /* Correct the path if it is a root directory or a pure drive */
      if (!path[0]) {
      	strcpy(path, DIRSEPARATOR_STRING);
      } else if ((*p == ':') && !path[1]) {
      	strcpy(path+1, ":");
      } else if ((path[1] == ':') && !path[2]) {
      	strcpy(path+2, DIRSEPARATOR_STRING);
      }
    } else {
      *path = '\0';
      p = pathname;
    }
    if (name) strcpy(name, p);
    RETURN_COMMENT(("path=\"%s\", name=\"%s\"\n", path, p));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    copydate						      |
|									      |
|   Description:    Copy the Date/time stamp from one file to another	      |
|									      |
|   Parameters:     char *pszToFile	Destination file		      |
|		    char *pszFromFile	Source file			      |
|									      |
|   Returns:	    0 = Success, else error and errno set		      |
|									      |
|   Notes:	    This operation is useless if the destination file is      |
|		    written to again. So it's necessary to flush it before    |
|		    calling this function.				      |
|									      |
|   History:								      |
|									      |
|    1996-10-14 JFL Made a clean, application-independant, version.	      |
|    2011-05-12 JFL Rewrote in an OS-independant way.			      |
|    2015-01-08 JFL Fallback to using chmod and utimes if lchmod and lutimes  |
|                   are not implemented. This will cause minor problems if the|
|                   target is a link, but will work well in all other cases.  |
*									      *
\*---------------------------------------------------------------------------*/

/* Old Linux versions define lchmod, but only implement a stub that always fails */
#ifdef __stub_lchmod
// #pragma message "The C Library does not implement lchmod. Using our own replacement."
#define lchmod lchmod1 /* Then use our own replacement for lchmod */
int lchmod1(const char *path, mode_t mode) {
  struct stat st = {0};
  int err;
  DEBUG_PRINTF(("lchmod1(\"%s\", %X);\n", path, mode));
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lchmod all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = chmod(path, mode);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif

#ifndef _STRUCT_TIMEVAL
/* No support for micro-second file time resolution. Use utime(). */
int copydate(char *pszToFile, char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct utimbuf utbTo = {0};
  int err;
  err = lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  utbTo.actime = stFrom.st_atime;
  utbTo.modtime = stFrom.st_mtime;
  err = lutime(pszToFile, &utbTo);
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(utbTo.modtime)); // Time of last data modification
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
    DEBUG_PRINTF(("utime(\"%s\", %s) = %d %s\n", pszToFile, buf, err,
      		  err?strerror(errno):""));
  });
  return err;                       /* Success */
}
#else /* defined(_STRUCT_TIMEVAL) */

#ifdef __stub_lutimes
// #pragma message "The C Library does not implement lutimes. Using our own replacement."
#define lutimes lutimes1 /* Then use our own replacement for lutimes */
int lutimes1(const char *path, const struct timeval times[2]) {
  struct stat st = {0};
  int err;
  // DEBUG_PRINTF(("lutimes1(\"%s\", %p);\n", path, &times)); // No need for this as VALUEIZE(lutimes) duplicates this below.
  err = lstat(path, &st);
  if (err) return err;
  /* Assume that libs that bother defining __stub_lutimes all define S_ISLNK */
  if (!S_ISLNK(st.st_mode)) { /* If it's anything but a link */
    err = utimes(path, times);	/* Then use the plain function supported by all OSs */
  } else { /* Else don't do it for a link, as it's the target that would be modified */
    err = -1;
    errno = ENOSYS;
  }
  return err;
}
#endif

/* Micro-second file time resolution supported. Use utimes(). */
int copydate(char *pszToFile, char *pszFromFile) { /* Copy the file dates */
  /* Note: "struct _stat" and "struct _utimbuf" don't compile under Linux */
  struct stat stFrom = {0};
  struct timeval tvTo[2] = {{0}, {0}};
  int err;
  DEBUG_PRINTF(("copydate(\"%s\", \"%s\")\n", pszToFile, pszFromFile));
  lstat(pszFromFile, &stFrom);
  /* Copy file permissions too */
  err = lchmod(pszToFile, stFrom.st_mode);
  /* And copy file times */
  TIMESPEC_TO_TIMEVAL(&tvTo[0], &stFrom.st_atim);
  TIMESPEC_TO_TIMEVAL(&tvTo[1], &stFrom.st_mtim);
  err = lutimes(pszToFile, tvTo);
#ifndef _MSVCLIBX_H_ /* Trace lutimes() call and return in Linux too */
  DEBUG_CODE({
    struct tm *pTime;
    char buf[40];
    pTime = LocalFileTime(&(stFrom.st_mtime)); // Time of last data modification
    sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d.%06ld",
	    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
	    pTime->tm_hour, pTime->tm_min, pTime->tm_sec, (long)stFrom.st_mtim.tv_nsec / 1000);
    DEBUG_PRINTF((VALUEIZE(lutimes) "(\"%s\", %s) = %d\n", pszToFile, buf, err));
  });
#endif
  return err;                       /* Success */
}
#endif /* !defined(_STRUCT_TIMEVAL) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    _filelength						      |
|									      |
|   Description:    Get the length of an open file			      |
|									      |
|   Parameters:     int hFile		File handle			      |
|									      |
|   Returns:	    The file length					      |
|									      |
|   Notes:	    Unix port of a Microsoft function			      |
|									      |
|   History:								      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef __unix__

#define _tell(hFile) lseek(hFile, 0, SEEK_CUR);

off_t _filelength(int hFile) {
  off_t curpos = _tell(hFile);			/* Save the current position */
  off_t length = lseek(hFile, 0, SEEK_END);	/* Move to the end of the file */
  lseek(hFile, curpos, SEEK_SET);		/* Return to the initial position */
  return length;
}

#endif /* defined(__unix__) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function	    fullpath						      |
|									      |
|  Description      Front-end to _fullpath, fixing the trail spaces bug	      |
|									      |
|  Parameters       Same as _fullpath					      |
|									      |
|  Returns	    Same as _fullpath					      |
|									      |
|  Notes	    _fullpath uses WIN32's GetFullPathName, which trims the   |
|		    trailing dots and spaces from path names.		      |
|		    							      |
|  History	    							      |
|    2017-01-30 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _WIN32

char *fullpath(char *absPath, const char *relPath, size_t maxLength) {
  int i;
  int l = (int)strlen(relPath);
  char *pszRet;
  DEBUG_ENTER(("fullpath(%p, \"%s\", %lu);\n", absPath, relPath, (unsigned long)maxLength));
  pszRet = _fullpath(absPath, relPath, maxLength);
  if (pszRet) {
    size_t  m = strlen(pszRet);  /* Current length of the output string */
    maxLength -= 1;		 /* Maximum length of the output string */
    /* If absPath is NULL, then _fullpath() allocated a new buffer with malloc() */
    if (!absPath) maxLength = m; /* _fullpath allocated a buffer */
    for (i=l; i && strchr(". \t", relPath[i-1]); i--) ; /* Search the first trailing dot or space */
    for ( ; i < l; i++) { /* Append the trailing dots or spaces to the output string */
      if (!absPath) {		/* Extend the buffer allocated by _fullpath */
      	char *pszRet2 = realloc(pszRet, ++maxLength + 1);
      	if (!pszRet2) {
      	  free(pszRet);
      	  RETURN_STRING(NULL);
      	}
      	pszRet = pszRet2;
      }
      if (m < maxLength) {
      	pszRet[m++] = relPath[i];
      	pszRet[m] = '\0';
      } else { /* The user-provided buffer is too small */
      	errno = ENOMEM;
      	RETURN_STRING(NULL);
      }
    }
  }
  RETURN_STRING(pszRet);
}

#endif /* _WIN32 */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetProgramNames					      |
|									      |
|   Description     Extract the program names from argv[0]		      |
|									      |
|   Parameters      char *argv[0]					      |
|									      |
|   Returns	    0							      |
|									      |
|   Notes	    Sets global variables program and progcmd.		      |
|		    Designed to work independantly of MsvcLibX.		      |
|		    							      |
|   History								      |
|    2018-03-23 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int GetProgramNames(char *argv0) {
#if defined(_MSDOS) || defined(_WIN32)
#if defined(_MSC_VER) /* Building with Microsoft tools */
#define strlwr _strlwr
#endif
  int lBase;
  char *pBase;
  char *p;
  pBase = strrchr(argv0, '\\');
  if ((p = strrchr(argv0, '/')) > pBase) pBase = p;
  if ((p = strrchr(argv0, ':')) > pBase) pBase = p;
  if (!(pBase++)) pBase = argv0;
  lBase = (int)strlen(pBase);
  program = strdup(pBase);
  strlwr(program);
  progcmd = strdup(program);
  if ((lBase > 4) && !strcmp(program+lBase-4, ".exe")) {
    progcmd[lBase-4] = '\0';
  } else {
    program = realloc(strdup(program), lBase+4+1);
    strcpy(program+lBase, ".exe");
  }
#else /* Build for Unix */
#include <libgen.h>	/* For basename() */
  program = basename(strdup(argv0)); /* basename() modifies its argument */
  progcmd = program;
#endif
  return 0;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    printError						      |
|									      |
|   Description     Print error messages with a consistent format	      |
|									      |
|   Parameters      char *pszFormat					      |
|		    ...							      |
|		    							      |
|   Returns	    The number of characters written			      |
|									      |
|   Notes	    Uses global variables program and progcmd,		      |
|		    set by GetProgramNames().				      |
|		    							      |
|   History								      |
|    2018-05-31 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int printError(char *pszFormat, ...) {
  va_list vl;
  int n;

  n = fprintf(stderr, "%s: ", program);
  va_start(vl, pszFormat);
  n += vfprintf(stderr, pszFormat, vl);
  n += fprintf(stderr, ".\n");
  va_end(vl);

  return n;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    NewPathName						      |
|									      |
|   Description     Join a directory name and a file name into a new pathname |
|									      |
|   Parameters      const char *path		The directory name, or NULL   |
|		    const char *name		The file name		      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    Wildcards allowed only in the name part of the pathname.  |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2017-10-09 JFL Allow the path pointer to be NULL. If so, dup. the name.  |
*									      *
\*---------------------------------------------------------------------------*/

char *NewPathName(const char *path, const char *name) {
  size_t lPath = path ? strlen(path) : 0;
  size_t lName = strlen(name);
  char *buf = malloc(lPath + lName + 2);
  if (!buf) return NULL;
  if (lPath) strcpy(buf, path);
  if (lPath && (buf[lPath-1] != DIRSEPARATOR_CHAR)) buf [lPath++] = DIRSEPARATOR_CHAR;
  strcpy(buf+lPath, name);
  return buf;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    zapDir						      |
|									      |
|   Description     Remove a directory, and all its files and subdirectories. |
|									      |
|   Parameters      const char *path		The directory pathname	      |
|		    int iFlags			Verbose & NoExec flags	      |
|		    							      |
|   Returns	    0 = Success, else # of failures encountered.	      |
|		    							      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    2017-10-05 JFL Created this routine				      |
|    2018-05-31 JFL Changed the iFlags argument to zapOpts *pzo.	      |
|		    The FLAG_FORCE flag now deletes read-only files.	      |
|		    Split zapFile() off of zapDir().			      |
|		    Added zapXxxM routines, with an additional iMode argument,|
|		     to avoid unnecessary slow calls to lstat() in Windows.   |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable:4706) /* Ignore the "assignment within conditional expression" warning */
#pragma warning(disable:4459) /* Ignore the "declaration of 'VARIABLE' hides global declaration" warning */
#endif

int zapFileM(const char *path, int iMode, zapOpts *pzo) {
  int iFlags = pzo->iFlags;
  char *pszSuffix = "";
  int iErr = 0;

  DEBUG_ENTER(("zapFileM(\"%s\", 0x%04X);\n", path, iMode));

  if (S_ISDIR(iMode)) {
    errno = EISDIR;
    RETURN_INT(1);
  }
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
  if (S_ISLNK(iMode)) {
    pszSuffix = ">";
  }
#endif

  if (iFlags & FLAG_VERBOSE) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  if (iFlags & FLAG_NOEXEC) RETURN_INT(0);
  if (iFlags & FLAG_FORCE) {
    if (!(iMode & S_IWRITE)) {
      iMode |= S_IWRITE;
      DEBUG_PRINTF(("chmod(%p, 0x%X);\n", path, iMode));
      iErr = -chmod(path, iMode); /* Try making the target file writable */
      DEBUG_PRINTF(("  return %d; // errno = %d\n", iErr, errno));
    }
    if (iErr) RETURN_INT(iErr);
  }
  iErr = -unlink(path); /* If error, iErr = 1 = # of errors */

  RETURN_INT(iErr);
}

int zapFile(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapFile(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) RETURN_INT(1);
  
  iErr = zapFileM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

int zapDirM(const char *path, int iMode, zapOpts *pzo) {
  char *pPath;
  int iErr;
  struct stat sStat;
  DIR *pDir;
  struct dirent *pDE;
  int nErr = 0;
  int iFlags = pzo->iFlags;
  int iVerbose = iFlags & FLAG_VERBOSE;
  int iNoExec = iFlags & FLAG_NOEXEC;
  char *pszSuffix;

  DEBUG_ENTER(("zapDirM(\"%s\", 0x%04X);\n", path, iMode));

  if (!S_ISDIR(iMode)) {
    errno = ENOTDIR;
    RETURN_INT(1);
  }

  pDir = opendir(path);
  if (!pDir) RETURN_INT(1);
  while ((pDE = readdir(pDir))) {
    DEBUG_PRINTF(("// Dir Entry \"%s\" d_type=%d\n", pDE->d_name, (int)(pDE->d_type)));
    pPath = NewPathName(path, pDE->d_name);
    pszSuffix = "";
#if _DIRENT2STAT_DEFINED /* MsvcLibX return DOS/Windows stat info in the dirent structure */
    iErr = dirent2stat(pDE, &sStat);
#else /* Unix has to query it separately */
    iErr = -lstat(pPath, &sStat); /* If error, iErr = 1 = # of errors */
#endif
    if (!iErr) switch (pDE->d_type) {
      case DT_DIR:
      	if (streq(pDE->d_name, ".")) break;	/* Skip the . directory */
      	if (streq(pDE->d_name, "..")) break;	/* Skip the .. directory */
      	iErr = zapDirM(pPath, sStat.st_mode, pzo);
      	pszSuffix = DIRSEPARATOR_STRING;
      	break;
#if defined(S_ISLNK) && S_ISLNK(S_IFLNK) /* In DOS it's defined, but always returns 0 */
      case DT_LNK:
      	pszSuffix = ">";
      	/* Fall through into the DT_REG case */
#endif
      case DT_REG:
	iErr = zapFileM(pPath, sStat.st_mode, pzo);
      	break;
      default:
      	iErr = 1;		/* We don't support deleting there */
#if defined(ENOSYS)
      	errno = ENOSYS;		/* Function not supported */
#else
      	errno = EPERM;		/* Operation not permitted */
#endif
      	pszSuffix = "?";
      	break;
    }
    if (iErr) {
      if (pDE->d_type != DT_DIR) printError("Error deleting \"%s%s\": %s", pPath, pszSuffix, strerror(errno));
      nErr += iErr;
      /* Continue the directory scan, looking for other files to delete */
    }
    free(pPath);
  }
  closedir(pDir);

  iErr = 0;
  pszSuffix = DIRSEPARATOR_STRING;
  if (path[strlen(path) - 1] == DIRSEPARATOR_CHAR) pszSuffix = ""; /* There's already a trailing separator */
  if (iVerbose) printf("%s%s%s\n", pzo->pszPrefix, path, pszSuffix);
  if (!iNoExec) iErr = rmdir(path);
  if (iErr) {
    printError("Error deleting \"%s%s\": %s", path, pszSuffix, strerror(errno));
    nErr += 1;
  }

  RETURN_INT_COMMENT(nErr, (nErr ? "%d deletions failed\n" : "Success\n", nErr));
}

int zapDir(const char *path, zapOpts *pzo) {
  int iErr;
  struct stat sStat;

  DEBUG_ENTER(("zapDir(\"%s\");\n", path));

  iErr = lstat(path, &sStat); /* Use lstat, as stat does not detect SYMLINKDs. */
  if (iErr && (errno == ENOENT)) RETURN_INT(0); /* Already deleted. Not an error. */
  if (iErr) {
    printError("Error: Can't stat \"%s\": %s", path, strerror(errno));
    RETURN_INT(1);
  }
  
  iErr = zapDirM(path, sStat.st_mode, pzo);
  RETURN_INT(iErr);
}

#ifdef _MSC_VER
#pragma warning(default:4706)
#pragma warning(default:4459)
#endif

