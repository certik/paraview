/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVFileInformation.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFileInformation - Information object that can
// be used to obtain information about a file/directory.
// .SECTION Description
// vtkPVFileInformation can be used to collect information about file
// or directory. vtkPVFileInformation can collect information
// from a vtkPVFileInformationHelper object alone. 
// .SECTION See Also
// vtkPVFileInformationHelper

#ifndef __vtkPVFileInformation_h
#define __vtkPVFileInformation_h

#include "vtkPVInformation.h"

class vtkCollection;
class vtkPVFileInformationSet;

class VTK_EXPORT vtkPVFileInformation : public vtkPVInformation
{
public:
  static vtkPVFileInformation* New();
  vtkTypeRevisionMacro(vtkPVFileInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  // The object must be a vtkPVFileInformationHelper.
  virtual void CopyFromObject(vtkObject* object);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  enum FileTypes
    {
    INVALID=0,
    SINGLE_FILE,
    DIRECTORY,
    FILE_GROUP,
    DRIVE
    };
  //ETX

  // Description:
  // Initializes the information object.
  void Initialize();

  // Description:
  // Get the name of the file/directory whose information is 
  // represented by this object.
  vtkGetStringMacro(Name);

  // Description:
  // Get the full path of the file/directory whose information is 
  // represented by this object.
  vtkGetStringMacro(FullPath);

  // Description:
  // Get the type of this file object.
  vtkGetMacro(Type, int);

  // Description:
  // Get the Contents for this directory.
  // Returns a collection with vtkPVFileInformation objects
  // for the contents of this directory if Type = DIRECTORY
  // or the contents of this file grouop if Type ==FILE_GROUP.
  vtkGetObjectMacro(Contents, vtkCollection);
//BTX
protected:
  vtkPVFileInformation();
  ~vtkPVFileInformation();

  vtkCollection* Contents;
  
  char* Name;     // Name of this file/directory.
  char* FullPath; // Full path for this file/directory.
  int Type;       // Type i.e. File/Directory/FileGroup.

  vtkSetStringMacro(Name);
  vtkSetStringMacro(FullPath);


  void GetWindowsDirectoryListing();
  void GetDirectoryListing();

  // Goes thru the collection of vtkPVFileInformation objects
  // are creates file groups, if possible. 
  void OrganizeCollection(vtkPVFileInformationSet& vector);

  bool DetectType();
  void GetSpecialDirectories();

  int FastFileTypeDetection;
private:
  vtkPVFileInformation(const vtkPVFileInformation&); // Not implemented.
  void operator=(const vtkPVFileInformation&); // Not implemented.

  struct vtkInfo;
//ETX
};


#endif

