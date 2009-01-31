// -*- c++ -*-
/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqOptions.cxx,v $

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

#include "pqOptions.h"

#include <vtkObjectFactory.h>
#include <vtkstd/string>

vtkStandardNewMacro(pqOptions);
vtkCxxRevisionMacro(pqOptions, "$Revision: 1.7 $");

//-----------------------------------------------------------------------------
pqOptions::pqOptions()
{
  this->BaselineImage = 0;
  this->TestDirectory = 0;
  this->DataDirectory = 0;
  this->ImageThreshold = 12;
  this->ExitAppWhenTestsDone = 0;
  this->DisableRegistry = 0;
  this->TestFileName = 0;
  this->TestInitFileName = 0;
  this->ServerResourceName = 0;
}

//-----------------------------------------------------------------------------
pqOptions::~pqOptions()
{
  this->SetBaselineImage(0);
  this->SetTestDirectory(0);
  this->SetDataDirectory(0);
  this->SetTestFileName(0);
  this->SetTestInitFileName(0);
  this->SetServerResourceName(0);
}

//-----------------------------------------------------------------------------
void pqOptions::Initialize()
{
  this->Superclass::Initialize();
  
  this->AddArgument("--compare-view", NULL, 
    &this->BaselineImage,
    "Compare the viewport to a reference image, and exit.");
  
  this->AddArgument("--test-directory", NULL,
    &this->TestDirectory,
    "Set the temporary directory where test-case output will be stored.");
  
  this->AddArgument("--data-directory", NULL,
    &this->DataDirectory,
    "Set the data directory where test-case data are.");
 
  this->AddArgument("--run-test", NULL,
    &this->TestFileName,  "Run a recorded test case.");

  this->AddArgument("--run-test-init", NULL,
    &this->TestInitFileName,  "Run a recorded test initialization case.");
  
  this->AddArgument("--image-threshold", NULL, &this->ImageThreshold,
    "Set the threshold beyond which viewport-image comparisons fail.");

  this->AddBooleanArgument("--exit", NULL, &this->ExitAppWhenTestsDone,
    "Exit application when testing is done. Use for testing.");

  this->AddBooleanArgument("--disable-registry", "-dr", &this->DisableRegistry,
    "Do not use registry when running ParaView (for testing).");

  this->AddArgument("--server", "-s",
    &this->ServerResourceName,
    "Set the name of the server resource to connect with when the client starts.");
}

//-----------------------------------------------------------------------------
int pqOptions::PostProcess(int argc, const char * const *argv)
{
  this->TestFiles.clear();
  if (this->TestInitFileName)
    {
    this->TestFiles << QString(this->TestInitFileName);
    }
  if (this->TestFileName)
    {
    this->TestFiles << QString(this->TestFileName);
    }
  return this->Superclass::PostProcess(argc, argv);
}

//-----------------------------------------------------------------------------
int pqOptions::WrongArgument(const char* arg)
{
  vtkstd::string argument = arg;
  int index = argument.find('=');
  if ( index != -1)
    {
    vtkstd::string key = argument.substr(0, index);
    vtkstd::string value = argument.substr(index+1);
    if (key == "--run-test")
      {
      this->TestFiles.push_back(value.c_str());
      return 1;
      }
    }
  return this->Superclass::WrongArgument(arg);
}

//-----------------------------------------------------------------------------
void pqOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageThreshold: " << this->ImageThreshold
    << endl;
  os << indent << "BaselineImage: " << (this->BaselineImage?
    this->BaselineImage : "(none)") << endl;
  os << indent << "TestDirectory: " << (this->TestDirectory?
    this->TestDirectory : "(none)") << endl;
  os << indent << "DataDirectory: " << (this->DataDirectory?
    this->DataDirectory : "(none)") << endl;

  os << indent << "ServerResourceName: " 
    << (this->ServerResourceName? this->ServerResourceName : "(none)") << endl;
}
