/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDelimitedTextReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkDelimitedTextReader - reader for pulling in flat text files
//
// .SECTION Description
// vtkDelimitedTextReader is an interface for pulling in data from a
// flat, delimited text file (delimiter can be any character).
//
// This class emits ProgressEvent for every 100 lines it reads.
//
// .SECTION Thanks
// Thanks to Andy Wilson and Brian Wylie from Sandia National Laboratories 
// for implementing this class.
// 
// .SECTION Caveats
//
// This reader assumes that the first line in the file (whether that's
// headers or the first document) contains at least as many fields as
// any other line in the file.

#ifndef __vtkDelimitedTextReader_h
#define __vtkDelimitedTextReader_h

#include "vtkTableAlgorithm.h"

class vtkTable;

//BTX
struct vtkDelimitedTextReaderInternals;
//ETX

class VTK_INFOVIS_EXPORT vtkDelimitedTextReader : public vtkTableAlgorithm
{
public:
  static vtkDelimitedTextReader* New();
  vtkTypeRevisionMacro(vtkDelimitedTextReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // Get/set the characters that will be used to separate fields.  For
  // example, set this to "," for a comma-separated value file.  Set
  // it to ".:;" for a file where columns can be separated by a
  // period, colon or semicolon.  The order of the characters in the
  // string does not matter.  Defaults to a comma.
  vtkSetStringMacro(FieldDelimiterCharacters);
  vtkGetStringMacro(FieldDelimiterCharacters);

  // Description:
  // Get/set the character that will begin and end strings.  Microsoft
  // Excel, for example, will export the following format:
  //
  // "First Field","Second Field","Field, With, Commas","Fourth Field"
  //
  // The third field has a comma in it.  By using a string delimiter,
  // this will be correctly read.  The delimiter defaults to '"'.
  vtkGetMacro(StringDelimiter, char);
  vtkSetMacro(StringDelimiter, char);

  // Description:
  // Set/get whether to use the string delimiter.  Defaults to on.
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  vtkBooleanMacro(UseStringDelimiter, bool);

  // Description:
  // Set/get whether to treat the first line of the file as headers.
  vtkGetMacro(HaveHeaders,bool);
  vtkSetMacro(HaveHeaders,bool);

  // Description:
  // Set/get whether to merge successive delimiters.  Use this if (for
  // example) your fields are separated by spaces but you don't know
  // exactly how many.
  vtkSetMacro(MergeConsecutiveDelimiters, bool);
  vtkGetMacro(MergeConsecutiveDelimiters, bool);
  vtkBooleanMacro(MergeConsecutiveDelimiters, bool);

  // Description:
  // Set/get the maximum number of records to read from the file (zero = unlimited)
  vtkGetMacro(MaxRecords, int);
  vtkSetMacro(MaxRecords, int);

 protected:
  vtkDelimitedTextReader();
  ~vtkDelimitedTextReader();

  vtkDelimitedTextReaderInternals* Internals;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  void OpenFile();

  char* FileName;
  char *FieldDelimiterCharacters;
  char StringDelimiter;
  bool UseStringDelimiter;
  bool HaveHeaders;
  bool MergeConsecutiveDelimiters;
  char *ReadBuffer;
  int MaxRecords;

private:
  vtkDelimitedTextReader(const vtkDelimitedTextReader&); // Not implemented
  void operator=(const vtkDelimitedTextReader&);   // Not implemented
};

#endif

