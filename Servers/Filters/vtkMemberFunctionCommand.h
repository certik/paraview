/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: vtkMemberFunctionCommand.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <vtkCommand.h>

/////////////////////////////////////////////////////////////////////////
// pqCommandMemFun

// .NAME vtkMemberFunctionCommand - Call a class member method in response to a VTK event
// .SECTION Description
// vtkMemberFunctionCommand is a vtkCommand-derivative that will listen for VTK events,
// calling a class member function when a VTK event is received.
//
// It is generally more useful than vtkCallbackCommand, which can only call
// non-member functions in response to a VTK event.
//
// Usage: create an instance of vtkMemberFunctionCommand, specialized for the class that
// will receive events.  Use the SetCallback() method to pass the instance and
// member function that will be called when an event is received.  Use
// vtkObject::AddObserver() to control which VTK events the vtkMemberFunctionCommand
// object will receive.
//
// Usage:
//
// vtkObject* subject = /* ... */
// foo* observer = /* ... */
// vtkMemberFunctionCommand<foo>* adapter = vtkMemberFunctionCommand<foo>::New();
// adapter->SetCallback(observer, &foo::bar);
// subject->AddObserver(vtkCommand::AnyEvent, adapter);
//
// .SECTION See Also
// vtkCallbackCommand
template<class ClassT>
class VTK_EXPORT vtkMemberFunctionCommand :
  public vtkCommand
{
  typedef vtkMemberFunctionCommand<ClassT> ThisT;
  
public:
  typedef vtkCommand Superclass;
  
  virtual const char* GetClassNameInternal() const { return "vtkMemberFunctionCommand"; }
  
  static ThisT* SafeDownCast(vtkObjectBase* o)
  {
    return dynamic_cast<ThisT*>(o);
  }

  static ThisT* New()
  {
    return new ThisT();
  }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    vtkCommand::PrintSelf(os, indent);
  }

  // Description:
  // Set which class instance and member function will be called when a VTK
  // event is received.
  void SetCallback(ClassT& object, void (ClassT::*method)())
  {
    this->Object = &object;
    this->Method = method;
  }

  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    if(this->Object && this->Method)
      {
      (this->Object->*this->Method)();
      }
  }

private:
  vtkMemberFunctionCommand() :
    Object(0),
    Method(0)
  {
  }
  
  ~vtkMemberFunctionCommand()
  {
  }
  
  ClassT* Object;
  void (ClassT::*Method)();
  
  vtkMemberFunctionCommand(const vtkMemberFunctionCommand&); // Not implemented
  void operator=(const vtkMemberFunctionCommand&); // Not implemented
};

// Description:
// Convenience function for creating vtkMemberFunctionCommand instances that
// automatically deduces its arguments.
//
// Usage:
//
// vtkObject* subject = /* ... */
// foo* observer = /* ... */
// vtkCommand* adapter = vtkMakeMemberFunctionCommand(observer, &foo::bar);
// subject->AddObserver(vtkCommand::AnyEvent, adapter);
//
// See Also:
// vtkMemberFunctionCommand, vtkCallbackCommand

template<class ClassT>
vtkMemberFunctionCommand<ClassT>* vtkMakeMemberFunctionCommand(
  ClassT& object, void (ClassT::*method)())
{
  vtkMemberFunctionCommand<ClassT>* result = vtkMemberFunctionCommand<ClassT>::New();
  result->SetCallback(object, method);
  
  return result;
}
