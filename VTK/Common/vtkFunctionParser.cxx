/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkFunctionParser.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFunctionParser.h"
#include "vtkObjectFactory.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkFunctionParser, "$Revision: 1.36 $");
vtkStandardNewMacro(vtkFunctionParser);

static double vtkParserVectorErrorResult[3] = { VTK_PARSER_ERROR_RESULT, 
                                                VTK_PARSER_ERROR_RESULT, 
                                                VTK_PARSER_ERROR_RESULT };

vtkFunctionParser::vtkFunctionParser() 
{
  this->NumberOfScalarVariables = 0;
  this->NumberOfVectorVariables = 0;
  this->ScalarVariableNames = NULL;
  this->VectorVariableNames = NULL;
  this->ScalarVariableValues = NULL;
  this->VectorVariableValues = NULL;
  this->Function = NULL;
  this->ByteCode = NULL;
  this->ByteCodeSize = 0;
  this->Immediates = NULL;
  this->ImmediatesSize = 0;
  this->Stack = NULL;
  this->StackSize = 0;
  this->StackPointer = 0;

  this->EvaluateMTime.Modified();
  this->VariableMTime.Modified();
  this->ParseMTime.Modified();
  this->FunctionMTime.Modified();

  this->ReplaceInvalidValues = 0;
  this->ReplacementValue = 0.0;
}

vtkFunctionParser::~vtkFunctionParser() 
{
  int i;
  
  if (this->ScalarVariableNames)
    {
    for (i = 0; i < this->NumberOfScalarVariables; i++)
      {
      delete [] this->ScalarVariableNames[i];
      this->ScalarVariableNames[i] = NULL;
      }
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }

  if (this->VectorVariableNames)
    {
    for (i = 0; i < this->NumberOfVectorVariables; i++)
      {
      delete [] this->VectorVariableNames[i];
      this->VectorVariableNames[i] = NULL;
      }
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }

  if (this->ScalarVariableValues)
    {
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }

  if (this->VectorVariableValues)
    {
    for (i = 0; i < this->NumberOfVectorVariables; i++)
      {
      delete [] this->VectorVariableValues[i];
      this->VectorVariableValues[i] = NULL;
      }
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }

  if (this->Function)
    {
    delete [] this->Function;
    this->Function = NULL;
    }
  
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    this->ByteCode = NULL;
    }
  
  if (this->Immediates)
    {
    delete [] this->Immediates;
    this->Immediates = NULL;
    }
  
  if (this->Stack)
    {
    delete [] this->Stack;
    this->Stack = NULL;
    }
}

void vtkFunctionParser::SetFunction(const char *function)
{
  if (this->Function && function && strcmp(this->Function,function) == 0)
    {
    return;
    }

  if (this->Function)
    {
    delete [] this->Function;
    }

  if (function)
    {
    this->Function = new char[strlen(function)+1];
    strcpy(this->Function,function);
    }
  else
    {
    this->Function = NULL;
    }

  this->FunctionMTime.Modified();
  this->Modified();
}

int vtkFunctionParser::Parse()
{
  int result;
  int i;
  
  if (this->Function == NULL)
    {
    vtkErrorMacro("Parse: no function has been set");
    return 0;
    }

  this->RemoveSpaces();
  
  result = this->CheckSyntax();
  if (!result)
    {
    return 0;
    }
  
  result = this->BuildInternalFunctionStructure();
  if (!result)
    {
    vtkErrorMacro("Parse: Error creating internal structure for parse string");
    return 0;
    }
  
  // need to make sure that the ambiguous operators are correct
  // - scalar/vector +
  // - scalar/vector -
  // - scalar/vector unary minus
  // - * (2 scalars) or scalar multiple (scalar, vector)
  result = this->DisambiguateOperators();
  if (!result)
    {
    vtkErrorMacro("Parse: Error deciding between ambiguous operators");
    return 0;
    }
  
  // need to recalculate stack size based on number of vector variables
  // in byte code
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    if ((this->ByteCode[i] >= VTK_PARSER_BEGIN_VARIABLES +
         this->NumberOfScalarVariables) ||
        (this->ByteCode[i] == VTK_PARSER_IHAT) ||
        (this->ByteCode[i] == VTK_PARSER_JHAT) ||
        (this->ByteCode[i] == VTK_PARSER_KHAT))
      {
      this->StackSize += 2;
      }
    }
  
  if (this->StackSize)
    {
    this->Stack = new double[this->StackSize];
    if (!this->Stack)
      {
      vtkErrorMacro("Parse: Out of memory");
      return 0;
      }
    }  

  this->ParseMTime.Modified();
  return 1;
}

int vtkFunctionParser::DisambiguateOperators()
{
  unsigned char* tempStack = new unsigned char[this->ByteCodeSize];
  int i;
  int tempStackPtr = -1;
  
  // using 0 for scalars and 1 for vectors
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    switch (this->ByteCode[i])
      {
      case VTK_PARSER_IMMEDIATE:
        tempStackPtr++;
        tempStack[tempStackPtr] = 0;
        break;
      case VTK_PARSER_UNARY_MINUS:
        if (tempStack[tempStackPtr] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_UNARY_MINUS;
          }
        break;
      case VTK_PARSER_ADD:
        if (tempStack[tempStackPtr] != 0 && tempStack[tempStackPtr-1] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_ADD;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_SUBTRACT:
        if (tempStack[tempStackPtr] != 0 && tempStack[tempStackPtr-1] != 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_SUBTRACT;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MULTIPLY:
        if (tempStack[tempStackPtr-1] == 0 && tempStack[tempStackPtr] == 1)
          {
          this->ByteCode[i] = VTK_PARSER_SCALAR_TIMES_VECTOR;
          tempStack[tempStackPtr-1] = 1;
          }
        else if (tempStack[tempStackPtr-1] == 1 && 
                 tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_TIMES_SCALAR;
          tempStack[tempStackPtr-1] = 1;
          }
        else if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting either 2 scalars or a scalar and"
                        << " a vector");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_DIVIDE:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't divide vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_POWER:
        if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("can't raise a vector to a power");
          return 0;
          }
      case VTK_PARSER_ABSOLUTE_VALUE:
      case VTK_PARSER_EXPONENT:
      case VTK_PARSER_CEILING:
      case VTK_PARSER_FLOOR:
      case VTK_PARSER_LOGARITHM:
      case VTK_PARSER_LOGARITHME:
      case VTK_PARSER_LOGARITHM10:
      case VTK_PARSER_SQUARE_ROOT:
      case VTK_PARSER_SINE:
      case VTK_PARSER_COSINE:
      case VTK_PARSER_TANGENT:
      case VTK_PARSER_ARCSINE:
      case VTK_PARSER_ARCCOSINE:
      case VTK_PARSER_ARCTANGENT:
      case VTK_PARSER_HYPERBOLIC_SINE:
      case VTK_PARSER_HYPERBOLIC_COSINE:
      case VTK_PARSER_HYPERBOLIC_TANGENT:
      case VTK_PARSER_SIGN:
        if (tempStack[tempStackPtr] == 1)
          {
          vtkErrorMacro("expecting a scalar, but got a vector");
          return 0;
          }
        break;
      case VTK_PARSER_MIN:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't apply min to vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MAX:
        if (tempStack[tempStackPtr] == 1 || tempStack[tempStackPtr-1] == 1)
          {
          vtkErrorMacro("can't apply max to vectors");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_CROSS:
        if (tempStack[tempStackPtr] == 0 || tempStack[tempStackPtr-1] == 0)
          {
          vtkErrorMacro("can't apply cross to scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_UNARY_MINUS:
        if (tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_UNARY_MINUS;
          }
        break;
      case VTK_PARSER_DOT_PRODUCT:
        if (tempStack[tempStackPtr] == 0 || tempStack[tempStackPtr-1] == 0)
          {
          vtkErrorMacro("dot product does not operate on scalars");
          return 0;
          }
        tempStack[tempStackPtr-1] = 0;
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_ADD:
        if (tempStack[tempStackPtr] != 1 && tempStack[tempStackPtr-1] != 1)
          {
          this->ByteCode[i] = VTK_PARSER_ADD;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("addition expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_SUBTRACT:
        if (tempStack[tempStackPtr] != 1 && tempStack[tempStackPtr-1] != 1)
          {
          this->ByteCode[i] = VTK_PARSER_SUBTRACT;
          }
        else if ((tempStack[tempStackPtr] == 0 &&
                  tempStack[tempStackPtr-1] != 0) ||
                 (tempStack[tempStackPtr] != 0 &&
                  tempStack[tempStackPtr-1] == 0))
          {
          vtkErrorMacro("subtraction expects either 2 vectors or 2 scalars");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_SCALAR_TIMES_VECTOR:
        if (tempStack[tempStackPtr] == 0 && tempStack[tempStackPtr-1] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_MULTIPLY;
          }
        else if (tempStack[tempStackPtr-1] == 1 &&
                 tempStack[tempStackPtr] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_VECTOR_TIMES_SCALAR;
          }
        else
          {
          vtkErrorMacro("expecting a scalar followed by a vector");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_VECTOR_TIMES_SCALAR:
        if (tempStack[tempStackPtr] == 0 && tempStack[tempStackPtr-1] == 0)
          {
          this->ByteCode[i] = VTK_PARSER_MULTIPLY;
          }
        else if (tempStack[tempStackPtr-1] == 0 &&
                 tempStack[tempStackPtr] == 1)
          {
          this->ByteCode[i] = VTK_PARSER_SCALAR_TIMES_VECTOR;
          }
        else
          {
          vtkErrorMacro("expecting a vector followed by a scalar");
          return 0;
          }
        tempStackPtr--;
        break;
      case VTK_PARSER_MAGNITUDE:
        if (tempStack[tempStackPtr] == 0)
          {
          vtkErrorMacro("magnitude expects a vector, but got a scalar");
          return 0;
          }
        tempStack[tempStackPtr] = 0;
        break;
      case VTK_PARSER_NORMALIZE:
        if (tempStack[tempStackPtr] == 0)
          {
          vtkErrorMacro("normalize expects a vector, but got a scalar");
          return 0;
          }
        break;
      case VTK_PARSER_IHAT:
      case VTK_PARSER_JHAT:
      case VTK_PARSER_KHAT:
        tempStackPtr++;
        tempStack[tempStackPtr] = 1;
        break;
      default:
        if ((this->ByteCode[i] - VTK_PARSER_BEGIN_VARIABLES) <
            this->NumberOfScalarVariables)
          {
          tempStackPtr++;
          tempStack[tempStackPtr] = 0;
          }
        else
          {
          tempStackPtr++;
          tempStack[tempStackPtr] = 1;
          }
      }
    }
  
  delete [] tempStack;
  return 1;
}

void vtkFunctionParser::Evaluate()
{
  int numBytesProcessed;
  int numImmediatesProcessed = 0;
  int stackPosition = -1;
  double magnitude;
  double temp[3];
  
  this->StackPointer = -1;

  if (this->FunctionMTime.GetMTime() > this->ParseMTime.GetMTime())
    {
    if (this->Parse() == 0)
      {
      return;
      }
    }

  for (numBytesProcessed = 0; numBytesProcessed < this->ByteCodeSize;
       numBytesProcessed++)
    {
    switch (this->ByteCode[numBytesProcessed])
      {
      case VTK_PARSER_IMMEDIATE:
        this->Stack[++stackPosition] =
          this->Immediates[numImmediatesProcessed++];
        break;
      case VTK_PARSER_UNARY_MINUS:
        this->Stack[stackPosition] =- this->Stack[stackPosition];
        break;
      case VTK_PARSER_ADD:
        this->Stack[stackPosition-1] += this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_SUBTRACT:
        this->Stack[stackPosition-1] -= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_MULTIPLY:
        this->Stack[stackPosition-1] *= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_DIVIDE:
        if (this->Stack[stackPosition]==0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition-1] = this->ReplacementValue;
            stackPosition--;
            }
          else
            {
            vtkErrorMacro("Trying to divide by zero");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition-1] /= this->Stack[stackPosition];
          stackPosition--;
          }
        break;
      case VTK_PARSER_POWER:
        this->Stack[stackPosition-1] = pow(this->Stack[stackPosition-1],
                                           this->Stack[stackPosition]);
        stackPosition--;
        break;
      case VTK_PARSER_ABSOLUTE_VALUE:
        this->Stack[stackPosition] = fabs(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_EXPONENT:
        this->Stack[stackPosition] = exp(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_CEILING:
        this->Stack[stackPosition] = ceil(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_FLOOR:
        this->Stack[stackPosition] = floor(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_LOGARITHM:
        if (this->Stack[stackPosition]<=0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = log(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_LOGARITHME:
        if (this->Stack[stackPosition]<=0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = log(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_LOGARITHM10:
        if (this->Stack[stackPosition]<=0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a logarithm of a negative value");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = 
            log(this->Stack[stackPosition])/log(static_cast<double>(10));
          }
        break;
      case VTK_PARSER_SQUARE_ROOT:
        if (this->Stack[stackPosition] < 0)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take a square root of a negative value");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = sqrt(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_SINE:
        this->Stack[stackPosition] = sin(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_COSINE:
        this->Stack[stackPosition] = cos(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_TANGENT:
        this->Stack[stackPosition] = tan(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_ARCSINE:
        if (this->Stack[stackPosition] < -1 || this->Stack[stackPosition] > 1)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take asin of a value < -1 or > 1");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = asin(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_ARCCOSINE:
        if(this->Stack[stackPosition]<-1 || this->Stack[stackPosition]>1)
          {
          if (this->ReplaceInvalidValues)
            {
            this->Stack[stackPosition] = this->ReplacementValue;
            }
          else
            {
            vtkErrorMacro("Trying to take acos of a value < -1 or > 1");
            return;
            }
          }
        else
          {
          this->Stack[stackPosition] = acos(this->Stack[stackPosition]);
          }
        break;
      case VTK_PARSER_ARCTANGENT:
        this->Stack[stackPosition] = atan(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_SINE:
        this->Stack[stackPosition] = sinh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_COSINE:
        this->Stack[stackPosition] = cosh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_HYPERBOLIC_TANGENT:
        this->Stack[stackPosition] = tanh(this->Stack[stackPosition]);
        break;
      case VTK_PARSER_MIN:
        if (this->Stack[stackPosition] < this->Stack[stackPosition-1])
          {
          this->Stack[stackPosition-1] = this->Stack[stackPosition];
          }
        stackPosition--;
        break;
      case VTK_PARSER_MAX:
        if (this->Stack[stackPosition] > this->Stack[stackPosition-1])
          {
          this->Stack[stackPosition-1] = this->Stack[stackPosition];
          }
        stackPosition--;
        break;
      case VTK_PARSER_CROSS:
        // Cross Product
        #define Ux stackPosition-5
        #define Uy stackPosition-4
        #define Uz stackPosition-3
        #define Vx stackPosition-2
        #define Vy stackPosition-1
        #define Vz stackPosition
        temp[0] = this->Stack[Uy]*this->Stack[Vz] -
                  this->Stack[Uz]*this->Stack[Vy];
        temp[1] = this->Stack[Uz]*this->Stack[Vx] -
                  this->Stack[Ux]*this->Stack[Vz];
        temp[2] = this->Stack[Ux]*this->Stack[Vy] -
                  this->Stack[Uy]*this->Stack[Vx];
        this->Stack[Ux] = temp[0];
        this->Stack[Uy] = temp[1];
        this->Stack[Uz] = temp[2];
        #undef Ux
        #undef Uy
        #undef Uz
        #undef Vx
        #undef Vy
        #undef Vz
        stackPosition-=3;
        break;
      case VTK_PARSER_SIGN:
        if (this->Stack[stackPosition] < 0)
          {
          this->Stack[stackPosition] = -1;
          }
        else if (this->Stack[stackPosition] ==  0)
          {
          this->Stack[stackPosition] = 0;
          }
        else
          {
          this->Stack[stackPosition] = 1;
          }
        break;
      case VTK_PARSER_VECTOR_UNARY_MINUS:
        this->Stack[stackPosition] = -this->Stack[stackPosition];
        this->Stack[stackPosition-1] = -this->Stack[stackPosition-1];
        this->Stack[stackPosition-2] = -this->Stack[stackPosition-2];
        break;
      case VTK_PARSER_DOT_PRODUCT:
        this->Stack[stackPosition-3] *= this->Stack[stackPosition];
        this->Stack[stackPosition-4] *= this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] *= this->Stack[stackPosition-2];
        this->Stack[stackPosition-5] = this->Stack[stackPosition-5] +
          this->Stack[stackPosition-4] + this->Stack[stackPosition-3];
        stackPosition -= 5;
        break;
      case VTK_PARSER_VECTOR_ADD:
        this->Stack[stackPosition-3] += this->Stack[stackPosition];
        this->Stack[stackPosition-4] += this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] += this->Stack[stackPosition-2];
        stackPosition -= 3;
        break;
      case VTK_PARSER_VECTOR_SUBTRACT:
        this->Stack[stackPosition-3] -= this->Stack[stackPosition];
        this->Stack[stackPosition-4] -= this->Stack[stackPosition-1];
        this->Stack[stackPosition-5] -= this->Stack[stackPosition-2];
        stackPosition -= 3;
        break;
      case VTK_PARSER_SCALAR_TIMES_VECTOR:
        this->Stack[stackPosition] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-1] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-2] *= this->Stack[stackPosition-3];
        this->Stack[stackPosition-3] = this->Stack[stackPosition-2];
        this->Stack[stackPosition-2] = this->Stack[stackPosition-1];
        this->Stack[stackPosition-1] = this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_VECTOR_TIMES_SCALAR:
        this->Stack[stackPosition-3] *= this->Stack[stackPosition];
        this->Stack[stackPosition-2] *= this->Stack[stackPosition];
        this->Stack[stackPosition-1] *= this->Stack[stackPosition];
        stackPosition--;
        break;
      case VTK_PARSER_MAGNITUDE:
        this->Stack[stackPosition-2] =
          sqrt(pow(this->Stack[stackPosition], 2) +
               pow(this->Stack[stackPosition-1], 2) +
               pow(this->Stack[stackPosition-2], 2));
        stackPosition -= 2;
        break;
      case VTK_PARSER_NORMALIZE:
        magnitude = sqrt(pow(this->Stack[stackPosition], 2) +
                         pow(this->Stack[stackPosition-1], 2) +
                         pow(this->Stack[stackPosition-2], 2));
        if (magnitude != 0)
          {
          this->Stack[stackPosition] /= magnitude;
          this->Stack[stackPosition-1] /= magnitude;
          this->Stack[stackPosition-2] /= magnitude;
          }
        break;
      case VTK_PARSER_IHAT:
        this->Stack[++stackPosition] = 1;
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 0;
        break;
      case VTK_PARSER_JHAT:
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 1;
        this->Stack[++stackPosition] = 0;
        break;
      case VTK_PARSER_KHAT:
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 0;
        this->Stack[++stackPosition] = 1;
        break;
      default:
        if ((this->ByteCode[numBytesProcessed] -
             VTK_PARSER_BEGIN_VARIABLES) < this->NumberOfScalarVariables)
          {
          this->Stack[++stackPosition] = 
            this->ScalarVariableValues[this->ByteCode[numBytesProcessed] -
                                      VTK_PARSER_BEGIN_VARIABLES];
          }
        else
          {
          int vectorNum = this->ByteCode[numBytesProcessed] -
            VTK_PARSER_BEGIN_VARIABLES - this->NumberOfScalarVariables;
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][0];
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][1];
          this->Stack[++stackPosition] = 
            this->VectorVariableValues[vectorNum][2];
          }
      }
    }
  this->StackPointer = stackPosition;

  this->EvaluateMTime.Modified();
}

int vtkFunctionParser::IsScalarResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() || 
      this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
    {
    this->Evaluate();
    }
  return (this->StackPointer == 0);
}

double vtkFunctionParser::GetScalarResult()
{
  if (!(this->IsScalarResult()))
    {
    vtkErrorMacro("GetScalarResult: no valid scalar result");
    return VTK_PARSER_ERROR_RESULT;
    }
  return this->Stack[0];
}

int vtkFunctionParser::IsVectorResult()
{
  if (this->VariableMTime.GetMTime() > this->EvaluateMTime.GetMTime() || 
      this->FunctionMTime.GetMTime() > this->EvaluateMTime.GetMTime())
    {
    this->Evaluate();
    }
  return (this->StackPointer == 2);
}

double *vtkFunctionParser::GetVectorResult()
{
  if (!(this->IsVectorResult()))
    {
    vtkErrorMacro("GetVectorResult: no valid vector result");
    return vtkParserVectorErrorResult;
    }
  return this->Stack;
}

char* vtkFunctionParser::GetScalarVariableName(int i)
{
  if (i >= 0 && i < this->NumberOfScalarVariables)
    {
    return this->ScalarVariableNames[i];
    }
  return NULL;
}

char* vtkFunctionParser::GetVectorVariableName(int i)
{
  if (i >= 0 && i < this->NumberOfVectorVariables)
    {
    return this->VectorVariableNames[i];
    }
  return NULL;
}

int vtkFunctionParser::IsVariableName(int currentIndex)
{
  int i;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strncmp(this->ScalarVariableNames[i], &this->Function[currentIndex],
                strlen(this->ScalarVariableNames[i])) == 0)
      {
      return 1;
      }
    }
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strncmp(this->VectorVariableNames[i], &this->Function[currentIndex],
                strlen(this->VectorVariableNames[i])) == 0)
      {
      return 1;
      }
    }

  return 0;
}

int vtkFunctionParser::IsElementaryOperator(int op)
{
  return strchr("+-.*/^", op) != NULL;
}

void vtkFunctionParser::SetScalarVariableValue(const char* inVariableName,
                                               double value)
{
  int i;
  double *tempValues;
  char** tempNames;
  char* variableName = this->RemoveSpacesFrom(inVariableName);

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strcmp(variableName, this->ScalarVariableNames[i]) == 0)
      {
      if (this->ScalarVariableValues[i] != value)
        {
        this->ScalarVariableValues[i] = value;
        this->VariableMTime.Modified();
        this->Modified();
        }
      delete [] variableName;
      return;
      }
    }

  tempValues = new double [this->NumberOfScalarVariables];
  tempNames = new char *[this->NumberOfScalarVariables];
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    tempValues[i] = this->ScalarVariableValues[i];
    tempNames[i] = new char [strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(tempNames[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    }

  if (this->ScalarVariableValues)
    {
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }

  this->ScalarVariableValues = new double [this->NumberOfScalarVariables + 1];
  this->ScalarVariableNames = new char *[this->NumberOfScalarVariables + 1];
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    this->ScalarVariableValues[i] = tempValues[i];
    this->ScalarVariableNames[i] = new char [strlen(tempNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], tempNames[i]);
    delete [] tempNames[i];
    tempNames[i] = NULL;
    }
  delete [] tempValues;
  delete [] tempNames;
  
  this->ScalarVariableValues[i] = value;
  this->ScalarVariableNames[i] = new char [strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[i], variableName);
  this->NumberOfScalarVariables++;

  this->VariableMTime.Modified();
  this->Modified();
  delete [] variableName;
}

void vtkFunctionParser::SetScalarVariableValue(int i, double value)
{
  if (i < 0 || i >= this->NumberOfScalarVariables)
    {
    return;
    }

  if (this->ScalarVariableValues[i] != value)
    {
    this->ScalarVariableValues[i] = value;
    this->VariableMTime.Modified();
    }
  this->Modified();
}

double vtkFunctionParser::GetScalarVariableValue(const char* inVariableName)
{
  int i;
  char* variableName = this->RemoveSpacesFrom(inVariableName);
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strcmp(variableName, this->ScalarVariableNames[i]) == 0)
      {
      delete [] variableName;
      return this->ScalarVariableValues[i];
      }
    }
  vtkErrorMacro("GetScalarVariableValue: scalar variable " << variableName 
                << " does not exist");
  delete [] variableName;
  return VTK_PARSER_ERROR_RESULT;
}

double vtkFunctionParser::GetScalarVariableValue(int i)
{
  if (i < 0 || i >= this->NumberOfScalarVariables)
    {
    vtkErrorMacro("GetScalarVariableValue: scalar variable " << i 
                  << " does not exist");
    return VTK_PARSER_ERROR_RESULT;
    }

  return this->ScalarVariableValues[i];
}

void vtkFunctionParser::SetVectorVariableValue(const char* inVariableName,
                                               double xValue, double yValue,
                                               double zValue)
{
  int i;
  double **tempValues;
  char** tempNames;
  char* variableName = this->RemoveSpacesFrom(inVariableName);
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strcmp(variableName, this->VectorVariableNames[i]) == 0)
      {
      if (this->VectorVariableValues[i][0] != xValue ||
          this->VectorVariableValues[i][1] != yValue ||
          this->VectorVariableValues[i][2] != zValue)
        {
        this->VectorVariableValues[i][0] = xValue;
        this->VectorVariableValues[i][1] = yValue;
        this->VectorVariableValues[i][2] = zValue;
        this->VariableMTime.Modified();
        this->Modified();
        }
      delete [] variableName;
      return;
      }
    }
  
  tempValues = new double *[this->NumberOfVectorVariables];
  tempNames = new char *[this->NumberOfVectorVariables];
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    tempValues[i] = new double[3];
    tempValues[i][0] = this->VectorVariableValues[i][0];
    tempValues[i][1] = this->VectorVariableValues[i][1];
    tempValues[i][2] = this->VectorVariableValues[i][2];
    tempNames[i] = new char [strlen(this->VectorVariableNames[i]) + 1];
    strcpy(tempNames[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    delete [] this->VectorVariableValues[i];
    this->VectorVariableValues[i] = NULL;
    }

  if (this->VectorVariableValues)
    {
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }

  this->VectorVariableValues = new double *[this->NumberOfVectorVariables + 1];
  this->VectorVariableNames = new char *[this->NumberOfVectorVariables + 1];
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    this->VectorVariableValues[i] = new double[3];
    this->VectorVariableValues[i][0] = tempValues[i][0];
    this->VectorVariableValues[i][1] = tempValues[i][1];
    this->VectorVariableValues[i][2] = tempValues[i][2];
    this->VectorVariableNames[i] = new char [strlen(tempNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], tempNames[i]);
    delete [] tempNames[i];
    tempNames[i] = NULL;
    delete [] tempValues[i];
    tempValues[i] = NULL;
    }
  delete [] tempValues;
  delete [] tempNames;
  
  this->VectorVariableValues[i] = new double[3];
  this->VectorVariableValues[i][0] = xValue;
  this->VectorVariableValues[i][1] = yValue;
  this->VectorVariableValues[i][2] = zValue;
  this->VectorVariableNames[i] = new char [strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[i], variableName);
  this->NumberOfVectorVariables++;

  this->VariableMTime.Modified();
  this->Modified();
  delete [] variableName;
}

void vtkFunctionParser::SetVectorVariableValue(int i, double xValue,
                                               double yValue, double zValue)
{
  if (i < 0 || i >= this->NumberOfVectorVariables)
    {
    return;
    }
  if (this->VectorVariableValues[i][0] != xValue ||
      this->VectorVariableValues[i][1] != yValue ||
      this->VectorVariableValues[i][2] != zValue)
    {
    this->VectorVariableValues[i][0] = xValue;
    this->VectorVariableValues[i][1] = yValue;
    this->VectorVariableValues[i][2] = zValue;
    this->VariableMTime.Modified();
    this->Modified();
    }
}

double* vtkFunctionParser::GetVectorVariableValue(const char* inVariableName)
{
  int i;
  char* variableName = this->RemoveSpacesFrom(inVariableName);
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strcmp(variableName, this->VectorVariableNames[i]) == 0)
      {
      delete [] variableName;
      return this->VectorVariableValues[i];
      }
    }
  vtkErrorMacro("GetVectorVariableValue: vector variable " << variableName 
                << " does not exist");
  delete [] variableName;
  return vtkParserVectorErrorResult;
}

double* vtkFunctionParser::GetVectorVariableValue(int i)
{
  if (i < 0 || i >= this->NumberOfVectorVariables)
    {
    vtkErrorMacro("GetVectorVariableValue: vector variable " << i 
                  << " does not exist");
    return vtkParserVectorErrorResult;
    }
  return this->VectorVariableValues[i];
}

char* vtkFunctionParser::RemoveSpacesFrom(const char* variableName)
{
  int len = static_cast<int>(strlen(variableName));
  int i;
  char* resultString = new char[len+1];
  char* out = resultString;
  for(i=0; i < len; ++i)
    {
    if(variableName[i] != ' ')
      {
      *out++ = variableName[i];
      }
    }
  *out = '\0';
  return resultString;
}

void vtkFunctionParser::RemoveSpaces()
{
  char *tempString;
  int i, length;
  
  this->FunctionLength = 0;
  length = static_cast<int>(strlen(this->Function));
  
  tempString = new char[length+1];
  for (i = 0; i < length; i++)
    {
    if (!isspace(this->Function[i]))
      {
      tempString[this->FunctionLength] = this->Function[i];
      this->FunctionLength++;
      }
    }
  
  delete [] this->Function;
  this->Function = new char[this->FunctionLength+1];
  strncpy(this->Function, tempString, this->FunctionLength);
  this->Function[this->FunctionLength] = '\0';
  delete [] tempString;
}

int vtkFunctionParser::OperatorWithinVariable(int idx)
{
  int i;
  char *tmpString;
  int start, end;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (strchr(this->ScalarVariableNames[i], this->Function[idx]) != 0)
      {
      tmpString = strstr(this->Function, this->ScalarVariableNames[i]);
      if (tmpString)
        {
        start = static_cast<int>(tmpString - this->Function);
        end = start + strlen(this->ScalarVariableNames[i]);
        if (start <= idx && end >= idx)
          {
          return 1;
          }
        }
      }
    }
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (strchr(this->VectorVariableNames[i], this->Function[idx]) != 0)
      {
      tmpString = strstr(this->Function, this->VectorVariableNames[i]);
      if (tmpString)
        {
        start = static_cast<int>(tmpString - this->Function);
        end = start + strlen(this->VectorVariableNames[i]);
        if (start <= idx && end >= idx)
          {
          return 1;
          }
        }
      }
    }
  
  return 0;
}

int vtkFunctionParser::CheckSyntax()
{
  int index = 0, parenthesisCount = 0, currentChar;
  char* ptr;
  int functionNumber, constantNumber;
  int *expectCommaOnParenthesisCount = new int[this->FunctionLength];
  int i;
  
  for (i = 0; i < this->FunctionLength; i++)
    {
    expectCommaOnParenthesisCount[i] = 0;
    }
  
  while (1)
    {
    currentChar = this->Function[index];
    
    // Check for valid operand (must appear)
    
    // Check for leading -
    if (currentChar == '-')
      {
      currentChar = this->Function[++index];  
      if(index == this->FunctionLength)
        {
        vtkErrorMacro("Syntax error: unary minus with no operand;"
                      << " see position " << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      }

    // Check for math function
    if ((functionNumber = this->GetMathFunctionNumber(index)))
      {
      if ((functionNumber == VTK_PARSER_MIN) ||
          (functionNumber == VTK_PARSER_MAX) ||
          (functionNumber == VTK_PARSER_CROSS))
        {
        expectCommaOnParenthesisCount[parenthesisCount+1] = 1;
        }
      index += this->GetMathFunctionStringLength(functionNumber);
      currentChar = this->Function[index];
      if ( currentChar != '(' )
        {
        vtkErrorMacro("Syntax error: input to math function not in "
                      << "parentheses; see position " << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      }
    
    // Check for opening parenthesis
    if( currentChar == '(' )
      {
      parenthesisCount++;
      index++;
      continue;
      }
    
    // Check for number
    if(isdigit(currentChar) ||
       (currentChar == '.' && isdigit(this->Function[index+1])))
      {
      strtod(&this->Function[index], &ptr);
      index += int(ptr-&this->Function[index]);
      currentChar = this->Function[index];
      }
    // Check for named constant
    else if ((constantNumber = this->GetMathConstantNumber(index)))
      {
      index += this->GetMathConstantStringLength(constantNumber);
      currentChar = this->Function[index];
      }
    else
      { // Check for variable
      if (!this->IsVariableName(index))
        {
        vtkErrorMacro("Syntax error: expecting a variable name; "
                      << "see position " << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      index += this->GetVariableNameLength(this->GetOperandNumber(index) -
                                           VTK_PARSER_BEGIN_VARIABLES);
      currentChar = this->Function[index];
      }

    // Check for possible second number from min or max function
    if (expectCommaOnParenthesisCount[parenthesisCount])
      {
      // Check for comma
      if (currentChar == ',')
        {
        expectCommaOnParenthesisCount[parenthesisCount] = 2;
        index++;
        continue;
        }
      }
    
    // Check for closing parenthesis
    while ( currentChar == ')' )
      {
      if (expectCommaOnParenthesisCount[parenthesisCount] == 1)
        {
        // We can't be closing this function if
        // expectCommaOnParenthesisCount[..] is still 1; either it was always
        // 0 or it should have been incremented to 2.
        vtkErrorMacro("Syntax Error: two parameters separated by commas "
                      << "expected; see position " << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      parenthesisCount--;
      if(parenthesisCount < 0)
        {
        vtkErrorMacro("Syntax Error: mismatched parenthesis; see position " 
                      << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      if( this->Function[index - 1] == '(' )
        {
        vtkErrorMacro("Syntax Error: empty parentheses; see position " 
                      << index);
        delete [] expectCommaOnParenthesisCount;
        return 0;
        }
      currentChar = this->Function[++index];
      }
    
    // If we get here, we have a legal operand and now a legal operator or
    // end of string must follow.
    
    // Check for EOS
    // The only way to end the checking loop without error.
    if (index == this->FunctionLength)
      {
      break;
      }
    // Check for operator
    if(!this->IsElementaryOperator(currentChar))
      {
      vtkErrorMacro("Syntax error: operator expected; see position "
                    << index);
      delete [] expectCommaOnParenthesisCount;
      return 0;
      }
    
    // If we get here, we have an operand and an operator; the next loop will
    // check for another operand (must appear)
    index++;
    } // while
  
  // Check that all opened parentheses are also closed
  if(parenthesisCount > 0)
    {
    vtkErrorMacro("Syntax Error: missing closing parenthesis; see position " 
                  << index);
    delete [] expectCommaOnParenthesisCount;
    return 0;
    }

  
  // The string is ok
  delete [] expectCommaOnParenthesisCount;
  return 1;
}

int vtkFunctionParser::BuildInternalFunctionStructure()
{
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    this->ByteCode = NULL;
    }
  if (this->Immediates)
    {
    delete [] this->Immediates;
    this->Immediates = NULL;
    }
  if (this->Stack)
    {
    delete [] this->Stack;
    this->Stack = NULL;
    }
  
  this->ByteCodeSize = this->ImmediatesSize = this->StackSize = 0;
  this->StackPointer = 0;
  this->BuildInternalSubstringStructure(0, this->FunctionLength - 1);  

  return 1;
}

void vtkFunctionParser::BuildInternalSubstringStructure(int beginIndex,
                                                        int endIndex)
{
  int mathFunctionNum, beginIndex2;
  int opNum, parenthesisCount, i;
  static const char* const elementaryMathOps = "+-.*/^";
  
  if (this->IsSubstringCompletelyEnclosed(beginIndex, endIndex))
    {
    this->BuildInternalSubstringStructure(beginIndex+1, endIndex-1);
    return;
    }

  if (this->Function[beginIndex] == '-')
    {
    if (this->IsSubstringCompletelyEnclosed(beginIndex+1, endIndex))
      {
      this->BuildInternalSubstringStructure(beginIndex+2, endIndex-1);
      this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
      return;
      }
    if (this->GetMathFunctionNumber(beginIndex+1) > 0 &&
        this->FindEndOfMathFunction(beginIndex+1) == endIndex)
      {
      this->BuildInternalSubstringStructure(beginIndex+1, endIndex);
      this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
      return;
      }
    if (this->GetMathConstantNumber(beginIndex+1) > 0 &&
        this->FindEndOfMathConstant(beginIndex+1) == endIndex)
      {
      this->BuildInternalSubstringStructure(beginIndex+1, endIndex);
      this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
      return;
      }
    }

  if (isalpha(this->Function[beginIndex]))
    {
    mathFunctionNum = this->GetMathFunctionNumber(beginIndex);
    if (mathFunctionNum > 0)
      {
      beginIndex2 = beginIndex;
      while (this->Function[beginIndex2] != '(')
        {
        beginIndex2++;
        }
      if (this->IsSubstringCompletelyEnclosed(beginIndex2, endIndex))
        {
        if ((mathFunctionNum == VTK_PARSER_MIN) ||
            (mathFunctionNum == VTK_PARSER_MAX) ||
            (mathFunctionNum == VTK_PARSER_CROSS))
          {
          parenthesisCount = 0;
          for (i = endIndex-1; i > beginIndex2; i--)
            {
            if (this->Function[i] == ')')
              {
              parenthesisCount++;
              }
            else if (this->Function[i] == '(')
              {
              parenthesisCount--;
              }
            if (parenthesisCount == 0 && this->Function[i] == ',')
              {
              this->BuildInternalSubstringStructure(beginIndex2+1, i-1);
              this->BuildInternalSubstringStructure(i+1, endIndex-1);
              this->AddInternalByte(mathFunctionNum);
              this->StackPointer--;
              return;
              }
            }
          }

        this->BuildInternalSubstringStructure(beginIndex2+1, endIndex-1);
        this->AddInternalByte(mathFunctionNum);
        return;
        }
      }
    }

  for (opNum = 0; opNum < 6; opNum++)
    {
    parenthesisCount = 0;
    for (i = endIndex; i > beginIndex; i--)
      {
      if (this->Function[i] == ')')
        {
        parenthesisCount++;
        }
      else if (this->Function[i] == '(')
        {
        parenthesisCount--;
        }
      if (parenthesisCount == 0 &&
          this->Function[i] == elementaryMathOps[opNum] &&
          !(this->Function[i] == '-' &&
            (this->IsElementaryOperator(this->Function[i-1]) ||
             this->Function[i-1] == '(' ||
             (this->Function[i-1] == 'e' && i > 1 &&
              isdigit(this->Function[i-2])))) &&
          !(this->Function[i] == '.' &&
            (i+1 < this->FunctionLength) &&
             (isdigit(this->Function[i+1]))) &&
          !this->OperatorWithinVariable(i))
        {
        this->BuildInternalSubstringStructure(beginIndex, i-1);
        this->BuildInternalSubstringStructure(i+1, endIndex);
        this->AddInternalByte(
          this->GetElementaryOperatorNumber(elementaryMathOps[opNum]));
        this->StackPointer--;
        return;
        }
      }
    }

  beginIndex2 = beginIndex;
  if (this->Function[beginIndex] == '-')
    {
    beginIndex2++;
    }
  
  this->AddInternalByte(this->GetOperandNumber(beginIndex2));
  this->StackPointer++;
  
  if (this->StackPointer > this->StackSize)
    {
    this->StackSize++;
    }
  if (beginIndex2 > beginIndex)
    {
    this->AddInternalByte(VTK_PARSER_UNARY_MINUS);
    }
}

void vtkFunctionParser::AddInternalByte(unsigned char newByte)
{
  int i;
  unsigned char *tempByteCode = new unsigned char[this->ByteCodeSize];

  for (i = 0; i < this->ByteCodeSize; i++)
    { // Copy current byte code to a temporary array
    tempByteCode[i] = this->ByteCode[i];
    }
  if (this->ByteCode)
    {
    delete [] this->ByteCode;
    }
  
  // Allocate space for new byte.
  this->ByteCode = new unsigned char[this->ByteCodeSize + 1];
  
  // Copy contents of temporary array back to ByteCode.
  for (i = 0; i < this->ByteCodeSize; i++)
    {
    this->ByteCode[i] = tempByteCode[i];
    }
  
  this->ByteCode[this->ByteCodeSize] = newByte;
  this->ByteCodeSize++;
  delete [] tempByteCode;
}

int vtkFunctionParser::IsSubstringCompletelyEnclosed(int beginIndex,
                                                     int endIndex)
{
  int i, parenthesisCount;
  
  if ( this->Function[beginIndex] == '(' && this->Function[endIndex]== ')' )
    {
    parenthesisCount = 1;
    for (i = beginIndex + 1; i < endIndex; i++)
      {
      if (this->Function[i] == '(' )
        {
        parenthesisCount++;
        }
      else if(this->Function[i] == ')' )
        {
        parenthesisCount--;
        }
      if (parenthesisCount == 0)
        {
        break;
        }
      }
    if (i == endIndex)
      {
      return 1;
      }
    }
  return 0;
}

int vtkFunctionParser::GetMathFunctionNumber(int currentIndex)
{
  if (strncmp(&this->Function[currentIndex], "abs", 3) == 0)
    {
    return VTK_PARSER_ABSOLUTE_VALUE;
    }
  if (strncmp(&this->Function[currentIndex], "exp", 3) == 0)
    {
    return VTK_PARSER_EXPONENT;
    }
  if (strncmp(&this->Function[currentIndex], "ceil", 4) == 0)
    {
    return VTK_PARSER_CEILING;
    }
  if (strncmp(&this->Function[currentIndex], "floor", 5) == 0)
    {
    return VTK_PARSER_FLOOR;
    }
  if (strncmp(&this->Function[currentIndex], "ln", 2) == 0)
    {
    return VTK_PARSER_LOGARITHME;
    }
  if (strncmp(&this->Function[currentIndex], "log10", 5) == 0)
    {
    return VTK_PARSER_LOGARITHM10;
    }
  if (strncmp(&this->Function[currentIndex], "log", 3) == 0)
    {
    vtkErrorMacro("The use of log function is being deprecated. "
                  "Please use log10 or ln instead");
    return VTK_PARSER_LOGARITHM;
    }
  if (strncmp(&this->Function[currentIndex], "sqrt", 4) == 0)
    {
    return VTK_PARSER_SQUARE_ROOT;
    }
  if (strncmp(&this->Function[currentIndex], "sin", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "sinh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_SINE;
      }
    return VTK_PARSER_SINE;
    }
  if (strncmp(&this->Function[currentIndex], "cos", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "cosh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_COSINE;
      }
    return VTK_PARSER_COSINE;
    }
  if (strncmp(&this->Function[currentIndex], "tan", 3) == 0)
    {
    if (strncmp(&this->Function[currentIndex], "tanh", 4) == 0)
      {
      return VTK_PARSER_HYPERBOLIC_TANGENT;
      }
    return VTK_PARSER_TANGENT;
    }
  if (strncmp(&this->Function[currentIndex], "asin", 4) == 0)
    {
    return VTK_PARSER_ARCSINE;
    }
  if (strncmp(&this->Function[currentIndex], "acos", 4) == 0)
    {
    return VTK_PARSER_ARCCOSINE;
    }
  if (strncmp(&this->Function[currentIndex], "atan", 4) == 0)
    {
    return VTK_PARSER_ARCTANGENT;
    }
  if (strncmp(&this->Function[currentIndex], "min", 3) == 0)
    {
    return VTK_PARSER_MIN;
    }
  if (strncmp(&this->Function[currentIndex], "max", 3) == 0)
    {
    return VTK_PARSER_MAX;
    }
  if (strncmp(&this->Function[currentIndex], "cross", 5) == 0) 
    {
    return VTK_PARSER_CROSS;
    }
  if (strncmp(&this->Function[currentIndex], "sign", 4) == 0)
    {
    return VTK_PARSER_SIGN;
    }
  if (strncmp(&this->Function[currentIndex], "mag", 3) == 0)
    {
    return VTK_PARSER_MAGNITUDE;
    }
  if (strncmp(&this->Function[currentIndex], "norm", 4) == 0)
    {
    return VTK_PARSER_NORMALIZE;
    }
  
  return 0;
}

int vtkFunctionParser::GetMathFunctionStringLength(int mathFunctionNumber)
{
  switch (mathFunctionNumber)
    {
    case VTK_PARSER_LOGARITHME:
      return 2;
    case VTK_PARSER_ABSOLUTE_VALUE:
    case VTK_PARSER_EXPONENT:
    case VTK_PARSER_LOGARITHM:
    case VTK_PARSER_SINE:
    case VTK_PARSER_COSINE:
    case VTK_PARSER_TANGENT:
    case VTK_PARSER_MAGNITUDE:
    case VTK_PARSER_MIN:
    case VTK_PARSER_MAX:
      return 3;
    case VTK_PARSER_CEILING:
    case VTK_PARSER_SQUARE_ROOT:
    case VTK_PARSER_ARCSINE:
    case VTK_PARSER_ARCCOSINE:
    case VTK_PARSER_ARCTANGENT:
    case VTK_PARSER_HYPERBOLIC_SINE:
    case VTK_PARSER_HYPERBOLIC_COSINE:
    case VTK_PARSER_HYPERBOLIC_TANGENT:
    case VTK_PARSER_NORMALIZE:
      return 4;
    case VTK_PARSER_FLOOR:
    case VTK_PARSER_LOGARITHM10:
    case VTK_PARSER_CROSS:
      return 5;
    default:
      vtkWarningMacro("Unknown math function");
      return 0;
    }
}

int vtkFunctionParser::GetMathConstantNumber(int currentIndex)
{
  if (strncmp(&this->Function[currentIndex], "iHat", 4) == 0)
    {
    return VTK_PARSER_IHAT;
    }
  if (strncmp(&this->Function[currentIndex], "jHat", 4) == 0)
    {
    return VTK_PARSER_JHAT;
    }
  if (strncmp(&this->Function[currentIndex], "kHat", 4) == 0)
    {
    return VTK_PARSER_KHAT;
    }  
  
  return 0;
}

int vtkFunctionParser::GetMathConstantStringLength(int mathConstantNumber)
{
  switch (mathConstantNumber)
    {
    case VTK_PARSER_IHAT:
    case VTK_PARSER_JHAT:
    case VTK_PARSER_KHAT:
      return 4;
    default:
      vtkWarningMacro("Unknown math constant");
      return 0;
    }
}

int vtkFunctionParser::GetVariableNameLength(int variableNumber)
{
  if (variableNumber < this->NumberOfScalarVariables)
    {
    return static_cast<int>(strlen(this->ScalarVariableNames[variableNumber]));
    }
  else
    {
    return static_cast<int>(strlen(
      this->VectorVariableNames[variableNumber -
                               this->NumberOfScalarVariables]));
    }
}

int vtkFunctionParser::FindEndOfMathFunction(int beginIndex)
{
  int i = beginIndex, parenthesisCount;
  
  while (this->Function[i] != '(' )
    {
    i++;
    }

  i++;

  for (parenthesisCount = 1; parenthesisCount > 0; ++i)
    {
    parenthesisCount += (this->Function[i] == '(' ? 1 :
                         (this->Function[i] == ')' ? -1 : 0));
    }
  return i - 1;
}

int vtkFunctionParser::FindEndOfMathConstant(int beginIndex)
{
  if(int constantNumber = this->GetMathConstantNumber(beginIndex))
    {
    return beginIndex+this->GetMathConstantStringLength(constantNumber);
    }
  return beginIndex;
}

int vtkFunctionParser::GetElementaryOperatorNumber(char op)
{
  static const char* const operators = "+-*/^";
  int i;
  
  for(i = 0; i < 5; i++)
    {
    if (operators[i] == op)
      {
      return VTK_PARSER_ADD + i;
      }
    }
  if (op == '.')
    {
    return VTK_PARSER_DOT_PRODUCT;
    }
  
  return 0;
}

int vtkFunctionParser::GetOperandNumber(int currentIndex)
{
  int i, variableIndex = -1;

  if (isdigit(this->Function[currentIndex]) ||
     this->Function[currentIndex] == '.') // Number
    {
    double *tempImmediates = new double[this->ImmediatesSize];
    for (i = 0; i < this->ImmediatesSize; i++)
      { // Copy current immediates to a temporary array
      tempImmediates[i] = this->Immediates[i];
      }
    if (this->Immediates)
      {
      delete [] this->Immediates;
      }
    
    // Allocate space for new immediate value.
    this->Immediates = new double[this->ImmediatesSize + 1];
    
    // Copy contents of temporary array back to Immediates.
    for (i = 0; i < this->ImmediatesSize; i++)
      {
      this->Immediates[i] = tempImmediates[i];
      }
  
    this->Immediates[this->ImmediatesSize] =
      atof(&this->Function[currentIndex]);
    this->ImmediatesSize++;
    delete [] tempImmediates;
    return VTK_PARSER_IMMEDIATE;
    }

  if (!strncmp(&this->Function[currentIndex], "iHat", 4))
    {
    return VTK_PARSER_IHAT;
    }
  if (!strncmp(&this->Function[currentIndex], "jHat", 4))
    {
    return VTK_PARSER_JHAT;
    }
  if (!strncmp(&this->Function[currentIndex], "kHat", 4))
    {
    return VTK_PARSER_KHAT;
    }
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->ScalarVariableNames[i],
                strlen(this->ScalarVariableNames[i])) == 0)
      {
      if (variableIndex == -1 ||
          (strlen(this->ScalarVariableNames[i]) >
           strlen(this->ScalarVariableNames[variableIndex])))
        {
        variableIndex = i;
        }
      }
    }
  if (variableIndex >= 0)
    {
    return VTK_PARSER_BEGIN_VARIABLES + variableIndex;
    }

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    { // Variable
    if (strncmp(&this->Function[currentIndex], this->VectorVariableNames[i],
                strlen(this->VectorVariableNames[i])) == 0)
      {
      if (variableIndex == -1 ||
          (strlen(this->VectorVariableNames[i]) >
           strlen(this->VectorVariableNames[variableIndex])))
        {
        variableIndex = i;
        }
      }
    }
  if (variableIndex >= 0)
    {
    return VTK_PARSER_BEGIN_VARIABLES + variableIndex + this->NumberOfScalarVariables;
    }
  
  return 0;
}

void vtkFunctionParser::RemoveScalarVariables()
{
  int i;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    }
  if (this->NumberOfScalarVariables > 0)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    delete [] this->ScalarVariableValues;
    this->ScalarVariableValues = NULL;
    }
  this->NumberOfScalarVariables = 0;
}

void vtkFunctionParser::RemoveVectorVariables()
{
  int i;
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    }
  if (this->NumberOfVectorVariables > 0)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    delete [] this->VectorVariableValues;
    this->VectorVariableValues = NULL;
    }
  this->NumberOfVectorVariables = 0;
}

void vtkFunctionParser::RemoveAllVariables()
{
  this->RemoveScalarVariables();
  this->RemoveVectorVariables();
}

void vtkFunctionParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  int i;

  os << indent << "Function: "
     << (this->Function ? this->Function : "(none)") << endl;

  os << indent << "NumberOfScalarVariables: " 
     << this->NumberOfScalarVariables << endl;

  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    os << indent << "  " << this->GetScalarVariableName(i) << ": " 
       << this->GetScalarVariableValue(i) << endl;
    }

  os << indent << "NumberOfVectorVariables: " 
     << this->NumberOfVectorVariables << endl;

  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    os << indent << "  " << this->GetVectorVariableName(i) << ": (" 
       << this->GetVectorVariableValue(i)[0] << ", "
       << this->GetVectorVariableValue(i)[1] << ", "
       << this->GetVectorVariableValue(i)[2] << ")" << endl;
    }

  if (this->EvaluateMTime.GetMTime() > this->FunctionMTime.GetMTime() &&
      this->EvaluateMTime.GetMTime() > this->VariableMTime.GetMTime() &&
      this->StackPointer == 0 || this->StackPointer == 2)
    {
    if (this->StackPointer == 0)
      {
      os << indent << "ScalarResult: " << this->GetScalarResult() << endl;
      os << indent << "VectorResult: " << "(none)" << endl;
      }
    else if (this->StackPointer == 2)
      {
      os << indent << "ScalarResult: " << "(none)" << endl;
      os << indent << "VectorResult: " << "(" 
       << this->GetVectorResult()[0] << ", "
       << this->GetVectorResult()[1] << ", "
       << this->GetVectorResult()[2] << ")" << endl;
      }
    }
  else
    {
    os << indent << "ScalarResult: " << "(none)" << endl;
    os << indent << "VectorResult: " << "(none)" << endl;
    }

  os << indent << "Replace Invalid Values: " 
     << (this->ReplaceInvalidValues ? "On" : "Off") << endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << endl;
}

