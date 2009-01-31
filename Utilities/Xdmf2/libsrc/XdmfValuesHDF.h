/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and ValuesHDF              */
/*                                                                 */
/*  Id : $Id: XdmfValuesHDF.h,v 1.2 2007/07/12 16:51:10 dave.demarle Exp $  */
/*  Date : $Date: 2007/07/12 16:51:10 $ */
/*  Version : $Revision: 1.2 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfValuesHDF_h
#define __XdmfValuesHDF_h


#include "XdmfValues.h"

//!  Parent Class for handeling I/O of actual data for an XdmfDataStructure
/*!

An HDF XdmfDataStructure Node Looks like :

\verbatim
<DataStructure
  Rank="2"
  Dimensions="2 4"
  Precision="4"
  DataType="Float"
  Format="HDF">
    MyData.h5:/AllValuesHDF/ThisArray
</DataStructure>
\endverbatim

XdmfValuesHDF is used to access the "MyData.h5:/AllValuesHDF/ThisArray" part wheather it's in the
HDF.
*/


class XDMF_EXPORT XdmfValuesHDF : public XdmfValues {

public :

  XdmfValuesHDF();
  virtual ~XdmfValuesHDF();

  XdmfConstString GetClassName() { return("XdmfValuesHDF"); } ;
  //! Read the Array from the External Representation
  XdmfArray *Read(XdmfArray *Array=NULL);
  //! Write the Array to the External Representation
  XdmfInt32 Write(XdmfArray *Array, XdmfConstString HeavyDataSetName=NULL);

protected :
};

#endif
