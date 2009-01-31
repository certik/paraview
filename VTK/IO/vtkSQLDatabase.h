/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQLDatabase.h,v $

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
// .NAME vtkSQLDatabase - maintains a connection to an sql database
//
// .SECTION Description
// Abstract base class for all SQL database connection classes.
// Manages a connection to the database, and is responsible for creating
// instances of the associated vtkSQLQuery objects associated with this
// class in order to perform execute queries on the database.
// To allow connections to a new type of database, create both a subclass
// of this class and vtkSQLQuery, and implement the required functions:
//
// Open() - open the database connection, if possible.
// Close() - close the connection.
// GetQueryInstance() - create and return an instance of the vtkSQLQuery
//                      subclass associated with the database type.
//
// The subclass should also provide API to set connection parameters.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes.
//
// .SECTION See Also
// vtkSQLQuery

#ifndef __vtkSQLDatabase_h
#define __vtkSQLDatabase_h

#include "vtkObject.h"

class vtkSQLQuery;
class vtkStringArray;

// This is a list of features that each database may or may not
// support.  As yet (April 2007) we don't provide access to most of
// them.  
#define VTK_SQL_FEATURE_TRANSACTIONS            1000
#define VTK_SQL_FEATURE_QUERY_SIZE              1001
#define VTK_SQL_FEATURE_BLOB                    1002
#define VTK_SQL_FEATURE_UNICODE                 1003
#define VTK_SQL_FEATURE_PREPARED_QUERIES        1004
#define VTK_SQL_FEATURE_NAMED_PLACEHOLDERS      1005
#define VTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS 1006
#define VTK_SQL_FEATURE_LAST_INSERT_ID          1007
#define VTK_SQL_FEATURE_BATCH_OPERATIONS        1008

class VTK_IO_EXPORT vtkSQLDatabase : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSQLDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Open a new connection to the database.
  // You need to set up any database parameters before calling this function.
  // Returns true is the database was opened sucessfully, and false otherwise.
  virtual bool Open() = 0;

  // Description:
  // Close the connection to the database.
  virtual void Close() = 0;
  
  // Description:
  // Return whether the database has an open connection
  virtual bool IsOpen() = 0;

  // Description:
  // Return an empty query on this database.
  virtual vtkSQLQuery* GetQueryInstance() = 0;
  
   // Description:
  // Get the last error text from the database
  virtual const char* GetLastErrorText() = 0;
  
  // Description:
  // Get the list of tables from the database
  virtual vtkStringArray* GetTables() = 0;
    
  // Description:
  // Get the list of fields for a particular table
  virtual vtkStringArray* GetRecord(const char *table) = 0;

  // Description:
  // Return whether a feature is supported by the database.
  virtual bool IsSupported(int vtkNotUsed(feature)) { return false; }


protected:
  vtkSQLDatabase();
  ~vtkSQLDatabase();

private:
  vtkSQLDatabase(const vtkSQLDatabase &); // Not implemented.
  void operator=(const vtkSQLDatabase &); // Not implemented.
};

#endif // __vtkSQLDatabase_h

