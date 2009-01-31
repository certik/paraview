/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* $Id: init.c,v 1.5 2007/09/06 23:15:49 kmorel Exp $ */

#include "test-util.h"
#include "test_codes.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "glwin.h"

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#define dup(fildes)             _dup(fildes)
#define dup2(fildes, fildes2)   _dup2(fildes, fildes2)
#endif

IceTStrategy strategy_list[5];
int STRATEGY_LIST_SIZE = 5;
/* int STRATEGY_LIST_SIZE = 1; */

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

static void checkOglError(void)
{
    GLenum error = glGetError();
#define TRY_ERROR(ename)                                                \
    if (error == ename) {                                               \
        printf("## Current OpenGL error = " #ename "\n");               \
        return;                                                         \
    }
    
    TRY_ERROR(GL_NO_ERROR);
    TRY_ERROR(GL_INVALID_ENUM);
    TRY_ERROR(GL_INVALID_VALUE);
    TRY_ERROR(GL_INVALID_OPERATION);
    TRY_ERROR(GL_STACK_OVERFLOW);
    TRY_ERROR(GL_STACK_UNDERFLOW);
    TRY_ERROR(GL_OUT_OF_MEMORY);
#ifdef GL_TABLE_TOO_LARGE
    TRY_ERROR(GL_TABLE_TOO_LARGE);
#endif
    printf("## UNKNOWN OPENGL ERROR CODE!!!!!!\n");
#undef TRY_ERROR
}

static void checkIceTError(void)
{
    GLenum error = icetGetError();
#define TRY_ERROR(ename)                                                \
    if (error == ename) {                                               \
        printf("## Current Ice-T error = " #ename "\n");                \
        return;                                                         \
    }
    TRY_ERROR(ICET_NO_ERROR);
    TRY_ERROR(ICET_SANITY_CHECK_FAIL);
    TRY_ERROR(ICET_INVALID_ENUM);
    TRY_ERROR(ICET_BAD_CAST);
    TRY_ERROR(ICET_OUT_OF_MEMORY);
    TRY_ERROR(ICET_INVALID_OPERATION);
    TRY_ERROR(ICET_INVALID_VALUE);
    printf("## UNKNOWN ICE-T ERROR CODE!!!!!\n");
#undef TRY_ERROR
}

/* Just in case I need to actually print stuff out to the screen in the
   future. */
static FILE *realstdout;
#if 0
static void realprintf(const char *fmt, ...)
{
    va_list ap;

    if (realstdout != NULL) {
        va_start(ap, fmt);
        vfprintf(realstdout, fmt, ap);
        va_end(ap);
        fflush(realstdout);
    }
}
#endif

static IceTContext context;

static void usage(char **argv)
{
    printf("\nUSAGE: %s [options] [-R] testname [testargs]\n", argv[0]);
    printf("\nWhere options are:\n");
    printf("  -width <n>  Width of window (default n=1024)\n");
    printf("  -height <n> Height of window (default n=768).\n");
    printf("  -display <display>\n");
    printf("              X server each node contacts.  Default display=localhost:0\n");
    printf("  -nologdebug Do not add debugging statements.  Provides less information, but\n");
    printf("              makes identifying errors and warnings easier.\n");
    printf("  -redirect   Redirect standard output to log.????, where ???? is the rank\n");
    printf("  --          Parse no more arguments.\n");
    printf("  -h          This help message.\n");
}

void initialize_test(int *argcp, char ***argvp, IceTCommunicator comm)
{
    int arg;
    int argc = *argcp;
    char **argv = *argvp;
    int width = 1024;
    int height = 768;
    char display[1024];
    GLbitfield diag_level = ICET_DIAG_FULL;
    int redirect = 0;
    int rank, num_proc;

    rank = (*comm->Comm_rank)(comm);
    num_proc = (*comm->Comm_size)(comm);

    display[0] = '\0';

  /* Parse my arguments. */
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-width") == 0) {
            width = atoi(argv[++arg]);
        } else if (strcmp(argv[arg], "-height") == 0) {
            height = atoi(argv[++arg]);
        } else if (strcmp(argv[arg], "-display") == 0) {
            sprintf(display, "DISPLAY=%s", argv[++arg]);
        } else if (strcmp(argv[arg], "-nologdebug") == 0) {
            diag_level &= ICET_DIAG_WARNINGS | ICET_DIAG_ALL_NODES;
        } else if (strcmp(argv[arg], "-redirect") == 0) {
            redirect = 1;
        } else if (strcmp(argv[arg], "-h") == 0) {
            usage(argv);
            exit(0);
        } else if (   (strcmp(argv[arg], "-R") == 0)
                   || (strncmp(argv[arg], "-", 1) != 0) ) {
            break;
        } else if (strcmp(argv[arg], "--") == 0) {
            arg++;
            break;
        } else {
            printf("Unknown options `%s'.  Try -h for help.\n", argv[arg]);
            exit(1);
        }
    }

  /* Fix arguments for next bout of parsing. */
    *argcp = 1;
    for ( ; arg < argc; arg++, (*argcp)++) {
        argv[*argcp] = argv[arg];
    }
    argc = *argcp;

  /* Make sure selected options are consistent. */
    if ((num_proc > 1) && (argc < 2)) {
        printf("You must select a test on the command line when using more than one process.\n");
        printf("Try -h for help.\n");
        exit(1);
    }
    if (redirect && (argc < 2)) {
        printf("You must select a test on the command line when redirecting the output.\n");
        printf("Try -h for help.\n");
        exit(1);
    }

  /* Create a renderable window. */
    if (display[0] != '\0') {
        putenv(strdup(display));
    }
    wincreat(0, 0, width, height, (char *)"ICE-T test");
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    swap_buffers();

    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;

  /* Create an ICE-T context. */
    context = icetCreateContext(comm);
    icetDiagnostics(diag_level);

  /* Redirect standard output on demand. */
    if (redirect) {
        char filename[64];
        int outfd;
        if (rank == 0) {
            realstdout = fdopen(dup(1), "wt");
        } else {
            realstdout = NULL;
        }
        sprintf(filename, "log.%04d", rank);
        outfd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (outfd < 0) {
            printf("Could not open %s for writing.\n", filename);
            exit(1);
        }
        dup2(outfd, 1);
    } else {
        realstdout = stdout;
    }

    strategy_list[0] = ICET_STRATEGY_DIRECT;
    strategy_list[1] = ICET_STRATEGY_SERIAL;
    strategy_list[2] = ICET_STRATEGY_SPLIT;
    strategy_list[3] = ICET_STRATEGY_REDUCE;
    strategy_list[4] = ICET_STRATEGY_VTREE;
}

extern void finalize_communication(void);
void finalize_test(int result)
{
    GLint rank;

    checkOglError();
    checkIceTError();

    icetGetIntegerv(ICET_RANK, &rank);
    if (rank == 0) {
        switch (result) {
          case TEST_PASSED:
              printf("***Test Passed***\n");
              break;
          case TEST_NOT_RUN:
              printf("***TEST NOT RUN***\n");
              break;
          case TEST_NOT_PASSED:
              printf("***TEST NOT PASSED***\n");
              break;
          case TEST_FAILED:
              printf("***TEST FAILED***\n");
              break;
        }
    }

    icetDestroyContext(context);
    finalize_communication();
}
