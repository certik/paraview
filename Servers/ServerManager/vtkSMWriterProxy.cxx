/*=========================================================================

Program:   ParaView
Module:    $RCSfile: vtkSMWriterProxy.cxx,v $

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterProxy.h"

#include "vtkErrorCode.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMWriterProxy);
vtkCxxRevisionMacro(vtkSMWriterProxy, "Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkSMWriterProxy::vtkSMWriterProxy()
{
  this->ErrorCode = vtkErrorCode::NoError;
  this->SupportsParallel = 0;
}

//-----------------------------------------------------------------------------
int vtkSMWriterProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
  vtkPVXMLElement* element)
{
  if (element->GetAttribute("supports_parallel"))
    {
    element->GetScalarAttribute("supports_parallel", &this->SupportsParallel);
    }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline()
{
  this->Superclass::UpdatePipeline();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "GetErrorCode"
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);
  pm->GetLastResult(this->GetConnectionID(), this->GetServers()).GetArgument(
    0, 0, &this->ErrorCode);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline(double time)
{
  this->Superclass::UpdatePipeline(time);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "GetErrorCode"
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);
  pm->GetLastResult(this->GetConnectionID(), this->GetServers()).GetArgument(
    0, 0, &this->ErrorCode);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ErrorCode: " 
     << vtkErrorCode::GetStringFromErrorCode(this->ErrorCode) << endl;
  os << indent << "SupportsParallel: "
     << this->SupportsParallel << endl;
}
