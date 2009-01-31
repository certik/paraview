/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXMLTreeReader.h,v $

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
// .NAME vtkXMLTreeReader - reads an XML file into a vtkTree
//
// .SECTION Description
// vtkXMLTreeReader parses an XML file and uses the nesting structure of the
// XML tags to generate a tree.  Node attributes are assigned to node arrays,
// and the special arrays .tagname and .chardata contain the tag type and the
// text internal to the tag, respectively.  The arrays are of type
// vtkStringArray.  There is an array for each attribute type in the XML file,
// even if it appears in only one tag.  If an attribute is missing from a tag,
// its value is the empty string.
//
// If MaskArrays is on (the default is off), the filter will additionally make bit
// arrays whose names are prepended with ".valid." which are 1 if the element 
// contains that attribute, and 0 otherwise. 
//
// For example, the XML file containing the text:
// <node name="jeff" age="26">
//   this is text in jeff's node
//   <node name="joe">
//     <node name="al" initials="amb" other="something"/>
//     <node name="dave" age="30"/>
//   </node>
//   <node name="lisa">this is text in lisa's node</node>
//   <node name="darlene" age="29"/>
// </node>
//
// would be parsed into a tree with the following node IDs and structure:
//
// 0 (jeff) - children: 1 (joe), 4 (lisa), 5 (darlene)
// 1 (joe)  - children: 2 (al), 3 (dave)
// 2 (al)
// 3 (dave)
// 4 (lisa)
// 5 (darlene)
//
// and the node data arrays would be as follows:
//
// name      initials  other     age       .tagname  .chardata
// ------------------------------------------------------------------------------------------------
// jeff      (empty)   (empty)   26         node     "  this is text in jeff's node\n  \n  \n  \n"
// joe       (empty)   (empty)   (empty)    node     "\n    \n    \n  "
// al        amb       something (empty)    node     (empty)
// dave      (empty)   (empty)   30         node     (empty)
// lisa      (empty)   (empty)   (empty)    node     "this is text in lisa's node"
// darlene   (empty)   (empty)   29         node     (empty)
//
// There would also be the following bit arrays if MaskArrays is on:
//
// .valid.name   .valid.initials   .valid.other   .valid.age
// ---------------------------------------------------------
// 1             0                 0              1
// 1             0                 0              0
// 1             1                 1              0
// 1             0                 0              1
// 1             0                 0              0
// 1             0                 0              1


#ifndef __vtkXMLTreeReader_h
#define __vtkXMLTreeReader_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkXMLTreeReader : public vtkTreeAlgorithm
{
public:
  static vtkXMLTreeReader* New();
  vtkTypeRevisionMacro(vtkXMLTreeReader,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If set, reads in the XML file specified.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // If set, and FileName is not set, reads in the XML string.
  vtkGetStringMacro(XMLString);
  vtkSetStringMacro(XMLString);

  // Description:
  // If on, makes bit arrays for each attribute with name .valid.attribute_name
  // for each attribute.  Default is off.
  vtkGetMacro(MaskArrays, bool);
  vtkSetMacro(MaskArrays, bool);
  vtkBooleanMacro(MaskArrays, bool);

  // Description:
  // If on, stores the XML character data (i.e. textual data between tags)
  // into an array named CharDataField, otherwise this field is skipped.
  // Default is off.
  vtkGetMacro(ReadCharData, bool);
  vtkSetMacro(ReadCharData, bool);
  vtkBooleanMacro(ReadCharData, bool);

  static const char * TagNameField;
  static const char * CharDataField;

protected:
  vtkXMLTreeReader();
  ~vtkXMLTreeReader();
  char* FileName;
  char* XMLString;
  bool ReadCharData;
  bool MaskArrays;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkXMLTreeReader(const vtkXMLTreeReader&); // Not implemented
  void operator=(const vtkXMLTreeReader&);   // Not implemented
};

#endif

