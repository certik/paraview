/*=========================================================================

  Module:    $RCSfile: vtkAbstractMap.txx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkVector.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkAbstractMap_txx
#define __vtkAbstractMap_txx

#include "vtkAbstractMap.h"

template<class KeyType, class DataType>
vtkAbstractMap<KeyType,DataType>::vtkAbstractMap() {}

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkAbstractMap<KeyType,DataType>::vtkAbstractMap(const vtkAbstractMap<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkAbstractMap<KeyType,DataType>::operator=(const vtkAbstractMap<KeyType,DataType>&){}
#endif

#endif



