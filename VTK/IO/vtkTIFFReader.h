/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTIFFReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTIFFReader - read TIFF files
// .SECTION Description
// vtkTIFFReader is a source object that reads TIFF files.
// It should be able to read almost any TIFF file
//
// .SECTION See Also
// vtkTIFFWriter

#ifndef __vtkTIFFReader_h
#define __vtkTIFFReader_h

#include "vtkImageReader2.h"

//BTX
class vtkTIFFReaderInternal;
//ETX

class VTK_IO_EXPORT vtkTIFFReader : public vtkImageReader2
{
public:
  static vtkTIFFReader *New();
  vtkTypeRevisionMacro(vtkTIFFReader,vtkImageReader2);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the given file name a tiff file file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
    return ".tif .tiff";
    }

  // Description:
  // Return a descriptive name for the file format that might be useful
  // in a GUI.
  virtual const char* GetDescriptiveName()
    {
    return "TIFF";
    }

  // Description:
  // Auxilary methods used by the reader internally.
  void InitializeColors();

  //BTX
  enum { NOFORMAT, RGB, GRAYSCALE, PALETTE_RGB, PALETTE_GRAYSCALE, OTHER };

  void ReadImageInternal( void *, void *outPtr,
                          int *outExt, unsigned int size );

  // Description:
  // Method to access internal image. Not to be used outside the class.
  vtkTIFFReaderInternal *GetInternalImage() { return this->InternalImage; }

  int EvaluateImageAt( void*, void* );
  //ETX

protected:
  vtkTIFFReader();
  ~vtkTIFFReader();

  void GetColor( int index,
                 unsigned short *r, unsigned short *g, unsigned short *b );
  unsigned int  GetFormat();
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);

private:
  vtkTIFFReader(const vtkTIFFReader&);  // Not implemented.
  void operator=(const vtkTIFFReader&);  // Not implemented.

  unsigned short *ColorRed;
  unsigned short *ColorGreen;
  unsigned short *ColorBlue;
  int TotalColors;
  unsigned int ImageFormat;
  vtkTIFFReaderInternal *InternalImage;
  int *InternalExtents;
};
#endif


