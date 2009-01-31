/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDsmCommMpi.h,v 1.3 2007/07/10 22:17:10 dave.demarle Exp $  */
/*  Date : $Date: 2007/07/10 22:17:10 $ */
/*  Version : $Revision: 1.3 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfDsmCommMpi_h
#define __XdmfDsmCommMpi_h

#include "XdmfDsmComm.h"
#include <mpi.h>

//! Base comm object for Distributed Shared Memory implementation
/*!
*/


class XDMF_EXPORT XdmfDsmCommMpi : public XdmfDsmComm {

public:
  XdmfDsmCommMpi();
  ~XdmfDsmCommMpi();

  XdmfConstString GetClassName() { return ( "XdmfDsmCommMpi" ) ; };


    //! Set the MPI Communicator
    XdmfSetValueMacro(Comm, MPI_Comm);
    //! Get the MPI Communicator
    XdmfGetValueMacro(Comm, MPI_Comm);

    XdmfInt32   DupComm(MPI_Comm Source);
    XdmfInt32   Init();
    XdmfInt32   Send(XdmfDsmMsg *Msg);
    XdmfInt32   Receive(XdmfDsmMsg *Msg);
    XdmfInt32   Check(XdmfDsmMsg *Msg);


protected:
    MPI_Comm    Comm;
};

#endif // __XdmfDsmCommMpi_h
