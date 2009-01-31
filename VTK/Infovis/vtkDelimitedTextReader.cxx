/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDelimitedTextReader.cxx,v $

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

#include "vtkDelimitedTextReader.h"
#include "vtkCommand.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkDelimitedTextReader, "$Revision: 1.18 $");
vtkStandardNewMacro(vtkDelimitedTextReader);

struct vtkDelimitedTextReaderInternals
{
  ifstream *File;
};

// Forward function reference (definition at bottom :)
static int splitString(const vtkStdString& input, 
                       const char *fieldDelimiters,
                       char stringDelimiter,
                       bool useStringDelimiter,
                       bool mergeConsecutiveDelimiters,
                       vtkstd::vector<vtkStdString>& results, 
                       bool includeEmpties=true);


// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
static int my_getline(istream& stream, vtkStdString &output, int& line_count, char delim='\n');

// ----------------------------------------------------------------------

vtkDelimitedTextReader::vtkDelimitedTextReader()
{
  this->Internals = new vtkDelimitedTextReaderInternals();

  this->Internals->File = 0;
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->ReadBuffer = new char[2048];
  this->HaveHeaders = false;
  this->FieldDelimiterCharacters = NULL;
  this->SetFieldDelimiterCharacters(",");
  this->StringDelimiter = '"';
  this->UseStringDelimiter = true;
  this->MaxRecords = 0;
  this->MergeConsecutiveDelimiters = false;
}

// ----------------------------------------------------------------------

vtkDelimitedTextReader::~vtkDelimitedTextReader()
{
  if (this->Internals->File)
    {
    delete this->Internals->File;
    this->Internals->File = 0;
    }

  this->SetFileName(0);
  this->SetFieldDelimiterCharacters(NULL);

  delete [] this->ReadBuffer;
  delete this->Internals;
}

// ----------------------------------------------------------------------

void vtkDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field delimiters: '" << this->FieldDelimiterCharacters
     << "'" << endl;
  os << indent << "String delimiter: '" << this->StringDelimiter
     << "'" << endl;
  os << indent << "UseStringDelimiter: " 
     << (this->UseStringDelimiter ? "true" : "false") << endl;
  os << indent << "HaveHeaders: " 
     << (this->HaveHeaders ? "true" : "false") << endl;
  os << indent << "MergeConsecutiveDelimiters: " 
     << (this->MergeConsecutiveDelimiters ? "true" : "false") << endl;
  os << indent << "MaxRecords: " << this->MaxRecords
     << endl;
}

// ----------------------------------------------------------------------

void vtkDelimitedTextReader::OpenFile()
{
  // If the file was open close it.
  if (this->Internals->File)
    {
    this->Internals->File->close();
    delete this->Internals->File;
    this->Internals->File = NULL;
    }
  
  // Open the new file.
  vtkDebugMacro(<< "vtkDelimitedTextReader is opening file: " << this->FileName);
  this->Internals->File = new ifstream(this->FileName, ios::in);

  // Check to see if open was successful
  if (! this->Internals->File || this->Internals->File->fail())
    {
    vtkErrorMacro(<< "vtkDelimitedTextReader could not open file " 
                  << this->FileName);
    return;
    }
}

// ----------------------------------------------------------------------

int vtkDelimitedTextReader::RequestData(
                                        vtkInformation*, 
                                        vtkInformationVector**, 
                                        vtkInformationVector* outputVector)
{
  int line_count = 0;

  // Check that the filename has been specified
  if (!this->FileName)
    {
    vtkErrorMacro("vtkDelimitedTextReader: You must specify a filename!");
    return 0;
    }
    
  // Open the file
  this->OpenFile();
  
  // Get the total size of the file ...
  this->Internals->File->seekg(0, ios::end);
  const int total_bytes = this->Internals->File->tellg();
  
  // Go to the top of the file
  this->Internals->File->seekg(0, ios::beg);

  // Store the text data into a vtkTable
  vtkTable* table = vtkTable::GetData(outputVector);
  
  // The first line of the file might contain the headers, so we want
  // to be a little bit careful about it.  If we don't have headers
  // we'll have to make something up.
  vtkstd::vector<vtkStdString> headers;


  // Not all platforms support vtkstd::getline(istream&, vtkstd::string) so
  // I have to do this the clunky way.

  vtkstd::vector<vtkStdString> firstLineFields;
  vtkStdString firstLine;

  my_getline(*(this->Internals->File), firstLine, line_count);
  vtkDebugMacro(<<"First line of file: " << firstLine.c_str());
   
  if (this->HaveHeaders)
    {
    splitString(firstLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                headers);
    }
  else
    {
    splitString(firstLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                firstLineFields);

    for (unsigned int i = 0; i < firstLineFields.size(); ++i)
      {
      // I know it's not a great idea to use sprintf.  It's safe right
      // here because an unsigned int will never take up enough
      // characters to fill up this buffer.
      char fieldName[64];
      sprintf(fieldName, "Field %d", i);
      headers.push_back(fieldName);
      }
    }

  // Now we can create the arrays that will hold the data for each
  // field.
  vtkstd::vector<vtkStdString>::const_iterator fieldIter;
  for(fieldIter = headers.begin(); fieldIter != headers.end(); ++fieldIter)
    { 
    vtkStringArray* array = vtkStringArray::New();
    array->SetName((*fieldIter).c_str());
    table->AddColumn(array);
    array->Delete();
    }
  
  // If the first line did not contain headers then we need to add it
  // to the table.
  if (!this->HaveHeaders)
    {
    vtkVariantArray* dataArray = vtkVariantArray::New();
    vtkstd::vector<vtkStdString>::const_iterator I;
    for(I = firstLineFields.begin(); I != firstLineFields.end(); ++I)
      {
      dataArray->InsertNextValue(vtkVariant(*I));
      }
    
    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
    }

  // Okay read the file and add the data to the table
  vtkStdString nextLine;
  while (my_getline(*(this->Internals->File), nextLine, line_count))
    {
    if(this->MaxRecords && line_count > this->MaxRecords)
      break;
    
    double progress = total_bytes
      ? static_cast<double>(this->Internals->File->tellg()) / static_cast<double>(total_bytes)
      : 0.5;
      
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

    vtkDebugMacro(<<"Next line: " << nextLine.c_str());
    vtkstd::vector<vtkStdString> dataVector;

    // Split string on the delimiters
    splitString(nextLine,
                this->FieldDelimiterCharacters,
                this->StringDelimiter,
                this->UseStringDelimiter,
                this->MergeConsecutiveDelimiters,
                dataVector);
    
    vtkDebugMacro(<<"Split into " << dataVector.size() << " fields");
    // Add data to the output arrays

    // Convert from vector to variant array
    vtkVariantArray* dataArray = vtkVariantArray::New();
    vtkstd::vector<vtkStdString>::const_iterator I;
    for(I = dataVector.begin(); I != dataVector.end(); ++I)
      {
      dataArray->InsertNextValue(vtkVariant(*I));
      }
    
    // Pad out any missing columns
    while (dataArray->GetNumberOfTuples() < table->GetNumberOfColumns())
      {
      dataArray->InsertNextValue(vtkVariant());
      }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
    }
 
  return 1;
}

// ----------------------------------------------------------------------

static int 
splitString(const vtkStdString& input, 
            const char *fieldDelimiters,
            char stringDelimiter,
            bool useStringDelimiter,
            bool mergeConsecutiveDelimiters,
            vtkstd::vector<vtkStdString>& results, 
            bool includeEmpties)
{
  if (input.size() == 0)
    {
    return 0;
    }

  bool inString = false;
  char thisCharacter = 0;
  char lastCharacter = 0;

  vtkstd::string currentField;

  for (unsigned int i = 0; i < input.size(); ++i)
    {
    thisCharacter = input[i];

    // Zeroth: are we in an escape sequence? If so, interpret this
    // character accordingly.
    if (lastCharacter == '\\')
      {
      char characterToAppend;
      switch (thisCharacter)
        {
        case '0': characterToAppend = '\0'; break;
        case 'a': characterToAppend = '\a'; break;
        case 'b': characterToAppend = '\b'; break;
        case 't': characterToAppend = '\t'; break;
        case 'n': characterToAppend = '\n'; break;
        case 'v': characterToAppend = '\v'; break;
        case 'f': characterToAppend = '\f'; break;
        case 'r': characterToAppend = '\r'; break;
        case '\\': characterToAppend = '\\'; break;
        default:  characterToAppend = thisCharacter; break;
        }

      currentField += characterToAppend;
      lastCharacter = thisCharacter;
      if (lastCharacter == '\\') lastCharacter = 0;
      }
    else 
      {
      // We're not in an escape sequence.

      // First, are we /starting/ an escape sequence?
      if (thisCharacter == '\\')
        {
        lastCharacter = thisCharacter;
        continue;
        }
      else if (useStringDelimiter && thisCharacter == stringDelimiter)
        {
        // this should just toggle inString
        inString = (inString == false);
        }
      else if (!inString && (strchr(fieldDelimiters, thisCharacter) != NULL))
        {
        if (mergeConsecutiveDelimiters && 
            (strchr(fieldDelimiters, lastCharacter) != NULL))
          {
          continue; // We're in the middle of a string of delimiters.
          }

        // A delimiter starts a new field unless we're in a string, in
        // which case it's normal text and we won't even get here.
        if (includeEmpties || currentField.size() > 0)
          {
          results.push_back(currentField);
          }
        currentField = vtkStdString();
        }
      else
        {
        // The character is just plain text.  Accumulate it and move on.
        currentField += thisCharacter;
        }
      
      lastCharacter = thisCharacter;
      }
    }

  results.push_back(currentField);
  return static_cast<int>(results.size());
}

// ----------------------------------------------------------------------

static int
my_getline(istream& in, vtkStdString &out, int& line_count, char delimiter)
{
  ++line_count;

  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;
  
  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
    {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
      {
      out += downcast;
      }
    else
      {
      return numCharactersRead;
      }
    }

  return numCharactersRead;
}
  
      
  
