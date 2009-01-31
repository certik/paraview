/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* $Id: projections.h,v 1.1 2003/06/17 18:38:54 andy Exp $ */

#include <GL/ice-t.h>

ICET_EXPORT void icetProjectTile(GLint tile);

ICET_EXPORT void icetGetViewportProject(GLint x, GLint y,
					GLsizei width, GLsizei height,
					GLdouble *mat_out);
