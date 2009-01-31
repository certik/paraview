/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* $Id: diagnostics.h,v 1.1 2003/06/17 18:38:54 andy Exp $ */

#ifndef _ICET_DIAGNOSTICS_H_
#define _ICET_DIAGNOSTICS_H_

#include <GL/ice-t.h>
#include <stdio.h>

ICET_EXPORT void icetRaiseDiagnostic(const char *msg, GLenum type,
				     GLbitfield level,
				     const char *file, int line);

#define icetRaiseError(msg, type)			\
    icetRaiseDiagnostic(msg, type, ICET_DIAG_ERRORS, __FILE__, __LINE__)

#define icetRaiseWarning(msg, type)			\
    icetRaiseDiagnostic(msg, type, ICET_DIAG_WARNINGS, __FILE__, __LINE__)

#ifdef DEBUG
#define icetRaiseDebug(msg)				\
    icetRaiseDiagnostic(msg, ICET_NO_ERROR, ICET_DIAG_DEBUG, __FILE__, __LINE__)

#define icetRaiseDebug1(msg, arg1)			\
    {							\
	char msg_string[256];				\
	sprintf(msg_string, msg, arg1);			\
	icetRaiseDebug(msg_string);			\
    }

#define icetRaiseDebug2(msg, arg1, arg2)		\
    {							\
	char msg_string[256];				\
	sprintf(msg_string, msg, arg1, arg2);		\
	icetRaiseDebug(msg_string);			\
    }

#define icetRaiseDebug4(msg, arg1, arg2, arg3, arg4)	\
    {							\
	char msg_string[256];				\
	sprintf(msg_string, msg, arg1,arg2,arg3,arg4);	\
	icetRaiseDebug(msg_string);			\
    }
#else /* DEBUG */
#define icetRaiseDebug(msg)
#define icetRaiseDebug1(msg,arg1)
#define icetRaiseDebug2(msg,arg1,arg2)
#define icetRaiseDebug4(msg,arg1,arg2,arg3,arg4)
#endif /* DEBUG */

ICET_EXPORT void icetDebugBreak(void);

#endif /* _ICET_DIAGNOSTICS_H_ */
