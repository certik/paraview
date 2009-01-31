/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDsmComm.h,v 1.4 2007/07/12 20:41:37 dave.demarle Exp $  */
/*  Date : $Date: 2007/07/12 20:41:37 $ */
/*  Version : $Revision: 1.4 $ */
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
#ifndef __XdmfDsmComm_h
#define __XdmfDsmComm_h

#include "XdmfObject.h"


//! Base comm object for Distributed Shared Memory implementation
/*!
*/

//! Macros to Shift to XDR if necessary (implement later)
#define XDMF_SHIFT64(a)     (a)
#define XDMF_SHIFT32(a)     (a)

class XdmfDsmMsg;

class XDMF_EXPORT XdmfDsmComm : public XdmfObject {

public:
  XdmfDsmComm();
  virtual ~XdmfDsmComm();

  XdmfConstString GetClassName() { return ( "XdmfDsmComm" ) ; };


//! Id 
    XdmfGetValueMacro(Id, XdmfInt32);
    XdmfSetValueMacro(Id, XdmfInt32);

//! Total 
    XdmfGetValueMacro(TotalSize, XdmfInt32);
    XdmfSetValueMacro(TotalSize, XdmfInt32);

    virtual XdmfInt32   Init();
    virtual XdmfInt32   Send(XdmfDsmMsg *Msg);
    virtual XdmfInt32   Receive(XdmfDsmMsg *Msg);
    virtual XdmfInt32   Check(XdmfDsmMsg *Msg);

    // XdmfInt32   Send() { return(this->Send(this->Msg)); };
    // XdmfInt32   Receive() { return(this->Receive(this->Msg)); };
    // XdmfInt32   Check() { return(this->Check(this->Msg)); };

protected:
    XdmfInt32       Id;
    XdmfInt32       TotalSize;
};

#endif // __XdmfDsmComm_h
