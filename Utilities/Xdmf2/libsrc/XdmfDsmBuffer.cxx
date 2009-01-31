/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDsmBuffer.cxx,v 1.8 2007/07/13 14:31:56 dave.demarle Exp $  */
/*  Date : $Date: 2007/07/13 14:31:56 $ */
/*  Version : $Revision: 1.8 $ */
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
#include "XdmfDsmBuffer.h"
#include "XdmfDsmComm.h"
#include "XdmfDsmMsg.h"
#include "XdmfArray.h"

#define XDMF_DSM_OPCODE_PUT     0x01
#define XDMF_DSM_OPCODE_GET     0x02


extern "C"{
void *
XdmfDsmBufferServiceThread(void *DsmObj){
    XdmfDsmBuffer *Dsm = (XdmfDsmBuffer *)DsmObj;
    return(Dsm->ServiceThread());
}
}



XdmfDsmBuffer::XdmfDsmBuffer() {
    this->ThreadDsmReady = 0;
}

XdmfDsmBuffer::~XdmfDsmBuffer() {
}

/*
XdmfInt32
XdmfDsmBuffer::Copy(XdmfDsmBuffer *Source){
    if(XdmfDsm::Copy((XdmfDsm *)Source) != XDMF_SUCCESS) return(XDMF_FAIL);
    return(XDMF_SUCCESS);
}
*/

void *
XdmfDsmBuffer::ServiceThread(){
    XdmfInt32   ReturnOpcode;
    // Create a copy of myself to get a Unique XdmfDsmMessage
    XdmfDsmBuffer   UniqueBuffer;

    UniqueBuffer.Copy(this);
    XdmfDebug("Starting DSM Service on node " << UniqueBuffer.GetComm()->GetId());
    this->ThreadDsmReady = 1;
    UniqueBuffer.ServiceLoop(&ReturnOpcode);
    this->ThreadDsmReady = 0;
    XdmfDebug("Ending DSM Service on node " << UniqueBuffer.GetComm()->GetId() << " last op = " << ReturnOpcode);
    return((void *)this);
}

XdmfInt32
XdmfDsmBuffer::ServiceOnce(XdmfInt32 *ReturnOpcode){
    XdmfInt32   status = XDMF_FAIL;

    this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
    status = this->Comm->Check(this->Msg);
    if(status != XDMF_SUCCESS){
        // Nothing to do
        return(XDMF_SUCCESS);
    }
    // Service One Call
    // cout << ".... Service a Call" << endl;
    return(this->Service(ReturnOpcode));
}

XdmfInt32
XdmfDsmBuffer::ServiceUntilIdle(XdmfInt32 *ReturnOpcode){
    XdmfInt32   status = XDMF_SUCCESS;

    while(status == XDMF_SUCCESS){
        this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
        status = this->Comm->Check(this->Msg);
        if(status != XDMF_SUCCESS){
            // Nothing to do
            return(XDMF_SUCCESS);
        }
        // Service One Call
        status = this->Service(ReturnOpcode);
        if(status != XDMF_SUCCESS){
            XdmfErrorMessage("ServiceUntilIdle detected error in Service() Method");
            return(XDMF_FAIL);
        }
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::ServiceLoop(XdmfInt32 *ReturnOpcode){
    XdmfInt32   op, status = XDMF_SUCCESS;

    while(status == XDMF_SUCCESS){
        status = this->Service(&op);
        if(status != XDMF_SUCCESS) return(XDMF_FAIL);
        if(ReturnOpcode) *ReturnOpcode = op;
        if(op == XDMF_DSM_OPCODE_DONE) return(XDMF_SUCCESS);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::Service(XdmfInt32 *ReturnOpcode){
    XdmfInt32   Opcode, who, status = XDMF_FAIL;
    XdmfInt64   aLength, Address;
    XdmfByte    *datap;

    status = this->ReceiveCommandHeader(&Opcode, &who, &Address, &aLength);
    if(status == XDMF_FAIL){
        XdmfErrorMessage("Error Receiving Command Header");
        return(XDMF_FAIL);
    }
    switch(Opcode){
        case XDMF_DSM_OPCODE_PUT :
            XdmfDebug("PUT request from " << who << " for " << aLength << " bytes @ " << Address);
            if(aLength > (this->EndAddress - Address + 1)){
                XdmfErrorMessage("Length too long");
                return(XDMF_FAIL);
            }
            datap = (XdmfByte *)this->Storage->GetDataPointer();
            datap += Address - this->StartAddress;
            this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
            status = this->ReceiveData(who, datap, aLength); 
            if(status == XDMF_FAIL){
                XdmfErrorMessage("ReceiveData() failed");
                return(XDMF_FAIL);
            }
            XdmfDebug("Serviced PUT request from " << who << " for " << aLength << " bytes @ " << Address);
            break;
        case XDMF_DSM_OPCODE_GET :
            XdmfDebug("(Server " << this->Comm->GetId() << ") Get request from " << who << " for " << aLength << " bytes @ " << Address);
            if(aLength > (this->EndAddress - Address + 1)){
                XdmfErrorMessage("Length " << aLength << " too long for address of len " << this->EndAddress - Address);
                XdmfErrorMessage("Server Start = " << this->StartAddress << " End = " << this->EndAddress);
                return(XDMF_FAIL);
            }
            datap = (XdmfByte *)this->Storage->GetDataPointer();
            datap += Address - this->StartAddress;
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->SendData(who, datap, aLength); 
            if(status == XDMF_FAIL){
                XdmfErrorMessage("SendData() failed");
                return(XDMF_FAIL);
            }
            XdmfDebug("(Server " << this->Comm->GetId() << ") Serviced GET request from " << who << " for " << aLength << " bytes @ " << Address);
            break;
        case XDMF_DSM_OPCODE_DONE :
            break;
        default :
            XdmfErrorMessage("Unknown Opcode " << Opcode);
            return(XDMF_FAIL);
    }
    if(ReturnOpcode) *ReturnOpcode = Opcode;
    return(XDMF_SUCCESS);
}
XdmfInt32
XdmfDsmBuffer::Put(XdmfInt64 Address, XdmfInt64 aLength, void *Data){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt64   astart, aend, len;
    XdmfByte    *datap = (XdmfByte *)Data;

    while(aLength){
        who = this->AddressToId(Address);
        if(who == XDMF_FAIL){
            XdmfErrorMessage("Address Error");
            return(XDMF_FAIL);
        }
        this->GetAddressRangeForId(who, &astart, &aend);
        len = MIN(aLength, aend - Address + 1);
        XdmfDebug("Put " << len << " Bytes to Address " << Address << " Id = " << who);
        if(who == MyId){
            XdmfByte *dp;

            // cout << "That's me!!" << endl;
            dp = (XdmfByte *)this->Storage->GetDataPointer();
            dp += Address - this->StartAddress;
            memcpy(dp, datap, len);

        }else{
            XdmfInt32   status;

            status = this->SendCommandHeader(XDMF_DSM_OPCODE_PUT, who, Address, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send PUT Header to " << who);
                return(XDMF_FAIL);
            }
            this->Msg->SetTag(XDMF_DSM_COMMAND_TAG);
            status = this->SendData(who, datap, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send " << len << " bytes of data to " << who);
                return(XDMF_FAIL);
            }

        }
        aLength -= len;
        Address += len;
        datap += len;
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmBuffer::Get(XdmfInt64 Address, XdmfInt64 aLength, void *Data){
    XdmfInt32   who, MyId = this->Comm->GetId();
    XdmfInt64   astart, aend, len;
    XdmfByte    *datap = (XdmfByte *)Data;

    while(aLength){
        who = this->AddressToId(Address);
        if(who == XDMF_FAIL){
            XdmfErrorMessage("Address Error");
            return(XDMF_FAIL);
        }
        this->GetAddressRangeForId(who, &astart, &aend);
        len = MIN(aLength, aend - Address + 1);
        XdmfDebug("Get " << len << " Bytes from Address " << Address << " Id = " << who);
        if(who == MyId){
            XdmfByte *dp;

            // cout << "That's me!!" << endl;
            dp = (XdmfByte *)this->Storage->GetDataPointer();
            dp += Address - this->StartAddress;
            memcpy(datap, dp, len);

        }else{
            XdmfInt32   status;

            status = this->SendCommandHeader(XDMF_DSM_OPCODE_GET, who, Address, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to send PUT Header to " << who);
                return(XDMF_FAIL);
            }
            this->Msg->SetTag(XDMF_DSM_RESPONSE_TAG);
            status = this->ReceiveData(who, datap, len);
            if(status == XDMF_FAIL){
                XdmfErrorMessage("Failed to receive " << len << " bytes of data from " << who);
                return(XDMF_FAIL);
            }

        }
        aLength -= len;
        Address += len;
        datap += len;
    }
    return(XDMF_SUCCESS);
}

