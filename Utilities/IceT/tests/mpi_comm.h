/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* $Id: mpi_comm.h,v 1.1 2003/06/17 18:38:54 andy Exp $ */

#ifndef _MPI_COMM_H_
#define _MPI_COMM_H_

#include "test-util.h"
#include <GL/ice-t_mpi.h>

void init_mpi_comm(int *argcp, char ***argvp)
{
    IceTCommunicator comm;

    MPI_Init(argcp, argvp);
    comm = icetCreateMPICommunicator(MPI_COMM_WORLD);

    initialize_test(argcp, argvp, comm);

    icetDestroyMPICommunicator(comm);    
}

void finalize_communication(void)
{
    MPI_Finalize();
}

#endif /*_MPI_COMM_H_*/
