/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkNetworkImageSource.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNetworkImageSource - an image source that whose data is encoded in a vtkClientServerStream
// .SECTION Description
// vtkNetworkImageSource is a subclass of vtkImageAlgorithm that takes a
// vtkClientServerStream with a message whose only argument is a string
// containing a .vtk dataset for a vtkImageData. Because the string contains
// binary (non-ASCII) data, it is not NULL-terminated, and so the method
// ReadImageFromString was not CSS-wrapped properly if we passed the string
// directly to this method. Instead we pass in the vtkClientServerStream
// and unpack it inside this method. Once the CSS has been unpacked, we
// pass the string to a vtkStructuredPointsReader to read the dataset
// contained in the string.

#ifndef __vtkNetworkImageSource_h
#define __vtkNetworkImageSource_h

#include "vtkImageAlgorithm.h"

class vtkImageData;
class vtkClientServerStream;

class VTK_EXPORT vtkNetworkImageSource : public vtkImageAlgorithm
{
public:
  static vtkNetworkImageSource* New();
  vtkTypeRevisionMacro(vtkNetworkImageSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Pass in a vtkClientServerStream containing a string that is a binary-
  // encoded .vtk dataset containing a vtkImageData.
  void ReadImageFromString(vtkClientServerStream &css);
  
protected:
  vtkNetworkImageSource();
  ~vtkNetworkImageSource();

  vtkImageData* Buffer;

  int RequestData(vtkInformation *request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);
  int RequestInformation(vtkInformation *request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector);

private:
  vtkNetworkImageSource(const vtkNetworkImageSource&); // Not implemented.
  void operator=(const vtkNetworkImageSource&); // Not implemented.
};

#endif
