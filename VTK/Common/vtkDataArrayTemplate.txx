/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDataArrayTemplate.txx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkDataArrayTemplate_txx
#define __vtkDataArrayTemplate_txx

#include "vtkDataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkIdList.h"
#include "vtkSortDataArray.h"
#include <vtkstd/exception>
#include <vtkstd/utility>
#include <vtkstd/algorithm>

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

//----------------------------------------------------------------------------
template <class T>
class vtkDataArrayTemplateLookup
{
public:
  vtkDataArrayTemplateLookup() : Rebuild(true)
    {
    this->SortedArray = NULL;
    this->IndexArray = NULL;
    }
  ~vtkDataArrayTemplateLookup()
    {
    if (this->SortedArray)
      {
      this->SortedArray->Delete();
      this->SortedArray = NULL;
      }
    if (this->IndexArray)
      {
      this->IndexArray->Delete();
      this->IndexArray = NULL;
      }
    }
  vtkAbstractArray* SortedArray;
  vtkIdList* IndexArray;
  bool Rebuild;
};

//----------------------------------------------------------------------------
template <class T>
vtkDataArrayTemplate<T>::vtkDataArrayTemplate(vtkIdType numComp):
  vtkDataArray(numComp)
{
  this->Array = 0;
  this->Tuple = 0;
  this->TupleSize = 0;
  this->SaveUserArray = 0;
  this->DeleteMethod = VTK_DATA_ARRAY_FREE;
  this->Lookup = 0;
}

//----------------------------------------------------------------------------
template <class T>
vtkDataArrayTemplate<T>::~vtkDataArrayTemplate()
{
  this->DeleteArray();
  if(this->Tuple)
    {
    free(this->Tuple);
    }
  if(this->Lookup)
    {
    delete this->Lookup;
    }
}

//----------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the suppled array.
template <class T>
void vtkDataArrayTemplate<T>::SetArray(T* array, 
                                       vtkIdType size, 
                                       int save,
                                       int deleteMethod)
{
  this->DeleteArray();

  vtkDebugMacro(<<"Setting array to: " << static_cast<void*>(array));

  this->Array = array;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
  this->DeleteMethod = deleteMethod;
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.
template <class T>
int vtkDataArrayTemplate<T>::Allocate(vtkIdType sz, vtkIdType)
{
  this->MaxId = -1;

  if(sz > this->Size)
    {
    this->DeleteArray();

    this->Size = 0;

    vtkIdType newSize = (sz > 0 ? sz : 1);
    this->Array = static_cast<T*>(malloc(newSize * sizeof(T)));
    if(!this->Array)
      {
      vtkErrorMacro("Unable to allocate " << newSize
                    << " elements of size " << sizeof(T)
                    << " bytes. ");
      return 0;
      }
    this->Size = newSize;
    }
  this->DataChanged();

  return 1;
}

//----------------------------------------------------------------------------
// Release storage and reset array to initial state.
template <class T>
void vtkDataArrayTemplate<T>::Initialize()
{
  this->DeleteArray();
  this->Array = 0;
  this->Size = 0;
  this->MaxId = -1;
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Deep copy of another double array.
template <class T>
void vtkDataArrayTemplate<T>::DeepCopy(vtkDataArray* fa)
{
  // Do nothing on a NULL input.
  if(!fa)
    {
    return;
    }

  // Avoid self-copy.
  if(this == fa)
    {
    return;
    }

  // If data type does not match, do copy with conversion.
  if(fa->GetDataType() != this->GetDataType())
    {
    this->Superclass::DeepCopy(fa);
    this->DataChanged();
    return;
    }

  // Free our previous memory.
  this->DeleteArray();

  // Copy the given array into new memory.
  this->NumberOfComponents = fa->GetNumberOfComponents();
  this->MaxId = fa->GetMaxId();
  this->Size = fa->GetSize();

  this->Size = (this->Size > 0 ? this->Size : 1);
  this->Array = static_cast<T*>(malloc(this->Size * sizeof(T)));
  if(!this->Array)
    {
    vtkErrorMacro("Unable to allocate " << this->Size
                  << " elements of size " << sizeof(T)
                  << " bytes. ");
    this->Size = 0;
    this->NumberOfComponents = 0;
    this->MaxId = -1;
    return;
    }
  if (fa->GetSize() > 0)
    {
    memcpy(this->Array, fa->GetVoidPointer(0), this->Size*sizeof(T));
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkOStreamWrapper osw(os);
  if(this->Array)
    {
    osw << indent << "Array: " << static_cast<void*>(this->Array) << "\n";
    }
  else
    {
    osw << indent << "Array: (null)\n";
    }
}

//----------------------------------------------------------------------------
// Protected function does "reallocate"
template <class T>
void vtkDataArrayTemplate<T>::DeleteArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    if (this->DeleteMethod == VTK_DATA_ARRAY_FREE)
      {
      free(this->Array);
      }
    else
      {
      delete[] this->Array;
      }
    }
  this->SaveUserArray = 0;
  this->DeleteMethod = VTK_DATA_ARRAY_FREE;
  this->Array = 0;
}

//----------------------------------------------------------------------------
// Protected function does "reallocate"
template <class T>
T* vtkDataArrayTemplate<T>::ResizeAndExtend(vtkIdType sz)
{
  T* newArray;
  vtkIdType newSize;

  if(sz > this->Size)
    {
    // Requested size is bigger than current size.  Allocate enough
    // memory to fit the requested size and be more than double the
    // currently allocated memory.
    newSize = this->Size + sz;
    }
  else if (sz == this->Size)
    {
    // Requested size is equal to current size.  Do nothing.
    return this->Array;
    }
  else
    {
    // Requested size is smaller than current size.  Squeeze the
    // memory.
    newSize = sz;
    }

  // Wipe out the array completely if new size is zero.
  if(newSize <= 0)
    {
    this->Initialize();
    return 0;
    }

  // Allocate the new array or reallocate the old.
  if(this->Array && (this->SaveUserArray || 
                     this->DeleteMethod == VTK_DATA_ARRAY_DELETE))
    {
    // The old array is owned by the user so we cannot try to
    // reallocate it.  Just allocate new memory that we will own.
    newArray = static_cast<T*>(malloc(newSize*sizeof(T)));
    if(!newArray)
      {
      vtkErrorMacro("Unable to allocate " << newSize
                    << " elements of size " << sizeof(T)
                    << " bytes. ");
      return 0;
      }

    // Copy the data from the old array.
    memcpy(newArray, this->Array,
           (newSize < this->Size ? newSize : this->Size) * sizeof(T));
    if (!this->SaveUserArray)
      {
      delete[] this->Array;
      this->Array = 0;
      }
    this->DeleteMethod = VTK_DATA_ARRAY_FREE;
    }
  else
    {
    // Try to reallocate with minimal memory usage and possibly avoid
    // copying.
    newArray = static_cast<T*>(realloc(this->Array, newSize*sizeof(T)));
    if(!newArray)
      {
      vtkErrorMacro("Unable to allocate " << newSize
                    << " elements of size " << sizeof(T)
                    << " bytes. ");
      return 0;
      }
    }

  // Allocation was successful.  Save it.
  if((newSize-1) < this->MaxId)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;

  // This object has now allocated its memory and owns it.
  this->SaveUserArray = 0;

  return this->Array;
}

//----------------------------------------------------------------------------
template <class T>
int vtkDataArrayTemplate<T>::Resize(vtkIdType sz)
{
  this->DataChanged();
  if(this->ResizeAndExtend(sz*this->NumberOfComponents) || sz <= 0)
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
// Set the number of n-tuples in the array.
template <class T>
void vtkDataArrayTemplate<T>::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(number*this->NumberOfComponents);
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Set the tuple at the ith location using the jth tuple in the source array.
// This method assumes that the two arrays have the same type
// and structure. Note that range checking and memory allocation is not 
// performed; use in conjunction with SetNumberOfTuples() to allocate space.
template <class T>
void vtkDataArrayTemplate<T>::SetTuple(vtkIdType i, vtkIdType j, 
  vtkAbstractArray* source)
{
  if (source->GetDataType() != this->GetDataType())
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }
  if (this->NumberOfComponents != source->GetNumberOfComponents())
    {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
    }
  
  vtkIdType loci = i * this->NumberOfComponents;
  vtkIdType locj = j * source->GetNumberOfComponents();

  T* data = static_cast<T*>(source->GetVoidPointer(0));
  
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->Array[loci + cur] = data[locj + cur];
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert the jth tuple in the source array, at ith location in this array. 
// Note that memory allocation is performed as necessary to hold the data.
template<class T>  
void vtkDataArrayTemplate<T>::InsertTuple(vtkIdType i, vtkIdType j, 
  vtkAbstractArray* source)
{
  if (source->GetDataType() != this->GetDataType())
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return;
    }
  int inNumComp = source->GetNumberOfComponents();
  if (this->NumberOfComponents != inNumComp)
    {
    vtkWarningMacro("Input and output component sizes do not match.");
    return;
    }
  
  vtkIdType locOut = i * inNumComp;
  vtkIdType maxSize = locOut + inNumComp;
  if (maxSize > this->Size)
    {
    this->ResizeAndExtend(maxSize);
    }

  vtkIdType locIn = j * inNumComp;

  T* outPtr = this->GetPointer(locOut);
  T* inPtr = static_cast<T*>(source->GetVoidPointer(locIn));

  memcpy(outPtr, inPtr, sizeof(T)*inNumComp);

  vtkIdType maxId = maxSize-1;
  if ( maxId > this->MaxId )
    {
    this->MaxId = maxId;
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert the jth tuple in the source array, at the end in this array. 
// Note that memory allocation is performed as necessary to hold the data.
// Returns the location at which the data was inserted.
template<class T>  
vtkIdType vtkDataArrayTemplate<T>::InsertNextTuple(vtkIdType j, 
  vtkAbstractArray* source)
{
   if (source->GetDataType() != this->GetDataType())
    {
    vtkWarningMacro("Input and output array data types do not match.");
    return -1;
    }
  if (this->NumberOfComponents != source->GetNumberOfComponents())
    {
    vtkWarningMacro("Input and output component sizes do not match.");
    return -1;
    } 
  
  // If this and source are the same, we need to make sure that
  // the array grows before we get the pointer. Growing the array
  // after getting the pointer may make it invalid.
  if (this == source)
    {
    this->ResizeAndExtend(this->Size+1);
    }

  T* data = static_cast<T*>(source->GetVoidPointer(0));
  vtkIdType locj = j * source->GetNumberOfComponents();
  
  for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
    {
    this->InsertNextValue(data[locj + cur]);
    }
  return (this->GetNumberOfTuples()-1);
}

//----------------------------------------------------------------------------
// Get a pointer to a tuple at the ith location. This is a dangerous method
// (it is not thread safe since a pointer is returned).
template <class T>
double* vtkDataArrayTemplate<T>::GetTuple(vtkIdType i)
{
  // If the small Tuple array fails to allocate we need something to
  // return.  This will avoid an immediate crash for arrays that do
  // not have too many components.
  static double sentryTuple[6] = {0,0,0,0,0,0};
#if 1
  // Allocate a larger tuple buffer if needed.
  if(this->TupleSize < this->NumberOfComponents)
    {
    this->TupleSize = this->NumberOfComponents;
    free(this->Tuple);
    this->Tuple = static_cast<double*>(malloc(this->TupleSize*sizeof(double)));
    }

  // Make sure tuple allocation succeeded.
  if(!this->Tuple)
    {
    vtkErrorMacro("Unable to allocate " << this->TupleSize
                  << " elements of size " << sizeof(double)
                  << " bytes. ");
    return sentryTuple;
    }

  // Copy the data into the tuple.
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Tuple[j] = static_cast<double>(t[j]);
    }
  return this->Tuple;
#else
  // Use this version along with purify or valgrind to detect code
  // that saves the pointer returned by GetTuple.  By always
  // allocating a new tuple and freeing the old one code that keeps
  // the pointer will do invalid reads or writes.
  double* newTuple;
  newTuple = (double*)malloc(this->NumberOfComponents * sizeof(double));
  if(!newTuple)
    {
    vtkErrorMacro("Unable to allocate " << this->NumberOfComponents
                  << " elements of size " << sizeof(double)
                  << " bytes. ");
    return sentryTuple;
    }

  // Copy the data into the new tuple.
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    newTuple[j] = static_cast<double>(t[j]);
    }

  // Replace the old tuple with the new one.
  free(this->Tuple);
  this->Tuple = newTuple;
  return this->Tuple;
#endif
}

//----------------------------------------------------------------------------
// Copy the tuple value into a user-provided array.
template <class T>
void vtkDataArrayTemplate<T>::GetTuple(vtkIdType i, double* tuple)
{
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    tuple[j] = static_cast<double>(t[j]);
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::GetTupleValue(vtkIdType i, T* tuple)
{
  T* t = this->Array + this->NumberOfComponents*i;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    tuple[j] = t[j];
    }
}

//----------------------------------------------------------------------------
// Set the tuple value at the ith location in the array.
template <class T>
void vtkDataArrayTemplate<T>::SetTuple(vtkIdType i, const float* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = static_cast<T>(tuple[j]);
    }
  this->DataChanged();
}

template <class T>
void vtkDataArrayTemplate<T>::SetTuple(vtkIdType i, const double* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = static_cast<T>(tuple[j]);
    }
  this->DataChanged();
}

template <class T>
void vtkDataArrayTemplate<T>::SetTupleValue(vtkIdType i, const T* tuple)
{
  vtkIdType loc = i * this->NumberOfComponents;
  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    this->Array[loc+j] = tuple[j];
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple into the ith location
// in the array.
template <class T>
void vtkDataArrayTemplate<T>::InsertTuple(vtkIdType i, const float* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }
  this->DataChanged();
}

template <class T>
void vtkDataArrayTemplate<T>::InsertTuple(vtkIdType i, const double* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }
  this->DataChanged();
}

template <class T>
void vtkDataArrayTemplate<T>::InsertTupleValue(vtkIdType i, const T* tuple)
{
  T* t = this->WritePointer(i*this->NumberOfComponents,
                            this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = *tuple++;
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Insert (memory allocation performed) the tuple onto the end of the array.
template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTuple(const float* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1, this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }

  this->DataChanged();
  return this->MaxId / this->NumberOfComponents;
}

template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTuple(const double* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1,this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = static_cast<T>(*tuple++);
    }

  this->DataChanged();
  return this->MaxId / this->NumberOfComponents;
}

template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextTupleValue(const T* tuple)
{
  T* t = this->WritePointer(this->MaxId + 1,this->NumberOfComponents);

  for(int j=0; j < this->NumberOfComponents; ++j)
    {
    *t++ = *tuple++;
    }

  this->DataChanged();
  return this->MaxId / this->NumberOfComponents;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveTuple(vtkIdType id)
{
  if ( id < 0 || id >= this->GetNumberOfTuples())
    {
    // Nothing to be done
    return;
    }
  if ( id == this->GetNumberOfTuples() - 1 )
    {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
    }
  // Remove the element by moving those after it over by one.  We must
  // use memmove instead of memcpy because the memory areas overlap.
  vtkIdType len = (this->GetNumberOfTuples() - id) - 1;
  len *= this->GetNumberOfComponents();
  vtkIdType from = (id+1) * this->GetNumberOfComponents();
  vtkIdType to = id * this->GetNumberOfComponents();
  memmove(this->Array + to, this->Array + from, len * sizeof(T));
  this->Resize(this->GetNumberOfTuples() - 1);
  this->DataChanged();
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveFirstTuple()
{
  this->RemoveTuple(0);
  this->DataChanged();
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::RemoveLastTuple()
{
  this->Resize(this->GetNumberOfTuples()- 1);
  this->DataChanged();
}

//----------------------------------------------------------------------------
// Return the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents.
template <class T>
double vtkDataArrayTemplate<T>::GetComponent(vtkIdType i, int j)
{
  return static_cast<double>(this->GetValue(i*this->NumberOfComponents + j));
}

//----------------------------------------------------------------------------
// Set the data component at the ith tuple and jth component location.
// Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
// memory has been allocated (use SetNumberOfTuples() and
// SetNumberOfComponents()).
template <class T>
void vtkDataArrayTemplate<T>::SetComponent(vtkIdType i, int j,
                                           double c)
{
  this->SetValue(i*this->NumberOfComponents + j, static_cast<T>(c));
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::InsertComponent(vtkIdType i, int j,
                                              double c)
{
  this->InsertValue(i*this->NumberOfComponents + j, static_cast<T>(c));
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::SetNumberOfValues(vtkIdType number)
{
  if(this->Allocate(number))
    {
    this->MaxId = number - 1;
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
template <class T>
T* vtkDataArrayTemplate<T>::WritePointer(vtkIdType id,
                                         vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  this->DataChanged();
  return this->Array + id;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::InsertValue(vtkIdType id, T f)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = f;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
  this->DataChanged();
}

//----------------------------------------------------------------------------
template <class T>
vtkIdType vtkDataArrayTemplate<T>::InsertNextValue(T f)
{
  this->InsertValue (++this->MaxId,f);
  return this->MaxId;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeRange(int comp)
{
  // If we got component -1 on a vector array, compute vector magnitude.
  if(comp < 0 && this->NumberOfComponents == 1)
    {
    comp = 0;
    }

  // Choose index into component range cache.
  int index = (comp<0)? this->NumberOfComponents : comp;

  if(index >= VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES ||
     (this->GetMTime() > this->ComponentRangeComputeTime[index]))
    {
    // We need to compute the range.
    this->Range[0] =  VTK_DOUBLE_MAX;
    this->Range[1] =  VTK_DOUBLE_MIN;

    if(comp >= 0)
      {
      this->ComputeScalarRange(comp);
      }
    else
      {
      this->ComputeVectorRange();
      }

    // Store the result in the range cache if there is room.
    if(index < VTK_MAXIMUM_NUMBER_OF_CACHED_COMPONENT_RANGES)
      {
      this->ComponentRangeComputeTime[index].Modified();
      this->ComponentRange[index][0] = this->Range[0];
      this->ComponentRange[index][1] = this->Range[1];
      }
    }
  else
    {
    // Copy value from range cache entry for this component.
    this->Range[0] = this->ComponentRange[index][0];
    this->Range[1] = this->ComponentRange[index][1];
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeScalarRange(int comp)
{
  // Compute range only if there are data.
  T* begin = this->Array+comp;
  T* end = this->Array+comp+this->MaxId+1;
  if(begin == end)
    {
    return;
    }

  // Compute the range of scalar values.
  int numComp = this->NumberOfComponents;
  T range[2] = {*begin, *begin};
  for(T* i = begin+numComp; i != end; i += numComp)
    {
    T s = *i;
    if(s < range[0])
      {
      range[0] = s;
      }
    else if(s > range[1])
      {
      range[1] = s;
      }
    }

  // Store the range.
  this->Range[0] = range[0];
  this->Range[1] = range[1];
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ComputeVectorRange()
{
  // Compute range only if there are data.
  T* begin = this->Array;
  T* end = this->Array+this->MaxId+1;
  if(begin == end)
    {
    return;
    }

  // Compute the range of vector magnitude squared.
  int numComp = this->NumberOfComponents;
  double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  for(T* i = begin; i != end; i += numComp)
    {
    double s = 0.0;
    for(int j=0; j < numComp; ++j)
      {
      double t = i[j];
      s += t*t;
      }
    if(s < range[0])
      {
      range[0] = s;
      }
    // this cannot be an elseif because there may be only one vector in which
    // case the range[1] would be left at a bad value
    if(s > range[1])
      {
      range[1] = s;
      }
    }

  // Store the range of vector magnitude.
  this->Range[0] = sqrt(range[0]);
  this->Range[1] = sqrt(range[1]);
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ExportToVoidPointer(void *out_ptr)
{
  if(out_ptr && this->Array) 
    {
    memcpy(static_cast<T*>(out_ptr), this->Array, 
           (this->MaxId + 1)*sizeof(T));
    }
}

//----------------------------------------------------------------------------
template <class T>
vtkArrayIterator* vtkDataArrayTemplate<T>::NewIterator()
{
  vtkArrayIteratorTemplate<T>* iter = vtkArrayIteratorTemplate<T>::New();
  iter->Initialize(this);
  return iter;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::UpdateLookup()
{
  if (!this->Lookup)
    {
    this->Lookup = new vtkDataArrayTemplateLookup<T>();
    this->Lookup->SortedArray = vtkAbstractArray::CreateArray(this->GetDataType());
    this->Lookup->IndexArray = vtkIdList::New();
    }
  if (this->Lookup->Rebuild)
    {
    int numComps = this->GetNumberOfComponents();
    vtkIdType numTuples = this->GetNumberOfTuples();
    this->Lookup->SortedArray->DeepCopy(this);
    this->Lookup->IndexArray->SetNumberOfIds(numComps*numTuples);
    for (vtkIdType i = 0; i < numComps*numTuples; i++)
      {
      this->Lookup->IndexArray->SetId(i, i);
      }
    vtkSortDataArray::Sort(this->Lookup->SortedArray, this->Lookup->IndexArray);
    this->Lookup->Rebuild = false;
    }
}

//----------------------------------------------------------------------------
template <class T>
vtkIdType vtkDataArrayTemplate<T>::LookupValue(vtkVariant var)
{
  T* dummyPtr = 0;
  bool valid = true;
  T value = var.ToNumeric(&valid, dummyPtr);
  if (valid)
    {
    return this->LookupValue(value);
    }
  return -1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::LookupValue(vtkVariant var, vtkIdList* ids)
{
  T* dummyPtr = 0;
  bool valid = true;
  T value = var.ToNumeric(&valid, dummyPtr);
  ids->Reset();
  if (valid)
    {
    this->LookupValue(value, ids);
    }
}

//----------------------------------------------------------------------------
template <class T>
vtkIdType vtkDataArrayTemplate<T>::LookupValue(T value)
{
  this->UpdateLookup();
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  T* ptr = static_cast<T*>(this->Lookup->SortedArray->GetVoidPointer(0));
  T* ptrEnd = ptr + numComps*numTuples;
  T* found = vtkstd::lower_bound(ptr, ptrEnd, value);
  if (found != ptrEnd && *found == value)
    {
    return this->Lookup->IndexArray->GetId(static_cast<vtkIdType>(found - ptr));
    }
  return -1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::LookupValue(T value, vtkIdList* ids)
{
  this->UpdateLookup();
  ids->Reset();
  int numComps = this->GetNumberOfComponents();
  vtkIdType numTuples = this->GetNumberOfTuples();
  T* ptr = static_cast<T*>(this->Lookup->SortedArray->GetVoidPointer(0));
  vtkstd::pair<T*,T*> found = 
    vtkstd::equal_range(ptr, ptr + numComps*numTuples, value);
  vtkIdType ind = static_cast<vtkIdType>(found.first - ptr);
  vtkIdType endInd = static_cast<vtkIdType>(found.second - ptr);
  for (; ind != endInd; ++ind)
    {
    ids->InsertNextId(this->Lookup->IndexArray->GetId(ind));
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::DataChanged()
{
  if (this->Lookup)
    {
    this->Lookup->Rebuild = true;
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataArrayTemplate<T>::ClearLookup()
{
  if (this->Lookup)
    {
    delete this->Lookup;
    this->Lookup = NULL;
    }
}

#endif
