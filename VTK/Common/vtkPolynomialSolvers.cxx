/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPolynomialSolvers.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov

=========================================================================*/
#include "vtkPolynomialSolvers.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkMath.h"

#ifndef isnan
// This is compiler specific not platform specific: MinGW doesn't need that.
# if defined(_MSC_VER) || defined(__BORLANDC__)
#  include <float.h>
#  define isnan(x) _isnan(x)
# endif
#endif

vtkCxxRevisionMacro(vtkPolynomialSolvers, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkPolynomialSolvers);

//----------------------------------------------------------------------------
void vtkPolynomialSolvers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
int polynomialEucliDiv( double* A, int m, double* B, int n, double* Q, double* R )
{
  // Note: for execution speed, no sanity checks are performed on A and B. 
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    Q[0] = 0.;
    for ( i = 0; i <= m; ++ i ) R[i] = A[i];

    return m;
    }
  
  double iB0 = 1. / B[0];
  if ( ! n )
    {
    for ( i = 0; i <= m; ++ i ) Q[i] = A[i] * iB0;

    return -1;
    }
  
  int nj;
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j ) Q[i] -= B[j] * Q[i - j] ;
    Q[i] *= iB0;
    }

  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    R[n - i] = A[m - i + 1];
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j ) R[n - i] -= B[n - i + 1 + j] * Q[mMn - j];

    if ( R[n - i] ) r = i - 1;
    }

  if ( ! r && ! R[0] ) return -1;

  return r;
}

//----------------------------------------------------------------------------
// Polynomial Euclidean division of A (deg m) by B (deg n).
// Does not store Q and stores -R instead of R
int polynomialEucliDivOppositeR( double* A, int m, double* B, int n, double* mR )
{
  // Note: for execution speed, no sanity checks are performed on A and B. 
  // You must know what you are doing.

  int mMn = m - n;
  int i;

  if ( mMn < 0 )
    {
    for ( i = 0; i <= m; ++ i ) mR[i] = A[i];
    return m;
    }
  
  if ( ! n ) return -1;
  
  double iB0 = 1. / B[0];
  int nj;
  double* Q = new double[mMn + 1];
  for ( i = 0; i <= mMn; ++ i )
    {
    nj = i > n ? n : i;
    Q[i] = A[i];
    for ( int j = 1; j <= nj; ++ j ) 
      {
      Q[i] -= B[j] * Q[i - j] ;
      }
    Q[i] *= iB0;
    }

  int r = 0;
  for ( i = 1; i <= n; ++ i )
    {
    mR[n - i] = - A[m - i + 1];
    nj = mMn + 1 > i ? i : mMn + 1;
    for ( int j = 0; j < nj; ++ j ) mR[n - i] += B[n - i + 1 + j] * Q[mMn - j];

    if ( mR[n - i] ) 
      {
      r = i - 1;
      }
    }
  delete [] Q;
  
  if ( ! r && ! mR[0] ) return -1;

  return r;
}

//----------------------------------------------------------------------------
// Evaluate the value of the degree d univariate polynomial P at x
// using Horner's algorithm.
inline double evaluateHorner( double* P, int d, double x )
{
  double val = P[0];
  for ( int i = 1; i <= d; ++ i ) val = val * x + P[i];

  return val;
}

//----------------------------------------------------------------------------
// Counts the number of real roots in [a[0],a[1]] of a real d-th degree 
// polynomial using Sturm's theorem.
int vtkPolynomialSolvers::SturmRootCount( double* P, int d, double* a )
{
  if ( ! P[0] )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootCount: Zero leading coefficient");
    return -1;
    }

  if ( d < 1 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootCount: Degree < 1");
    return -1;
    }

  if ( a[1] <= a[0] )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootCount: Erroneous interval endpoints");
    return -1;
    }

  double* SSS = new double[( d + 1 ) * ( d + 2 ) / 2];
  int* degSSS = new int[d + 2];
  
  int offsetA = 0;
  degSSS[0] = d;
  SSS[0] = P[0];
  SSS[d] = P[d];

  int offsetB = d + 1;
  degSSS[1] = d - 1;
  SSS[offsetB] = static_cast<double>(d) * P[0];

  int i;
  double oldVal[] = { P[0], P[0] };
  for ( i = 1; i < d; ++ i ) 
    {
    SSS[i] = P[i];
    SSS[offsetB + i] = static_cast<double>( d - i ) * P[i];
    for ( int k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * a[k] + P[i];
    }
  for ( int k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * a[k] + P[d];

  int varSgn[] = { 0, 0 };
  int offsetR;
  int nSSS = 1;
  for ( ; degSSS[nSSS] > -1; ++ nSSS )
    {
    double newVal[] = { SSS[offsetB], SSS[offsetB] };
    for ( int k = 0; k < 2; ++ k )
      {
      for ( i = 1; i <= degSSS[nSSS]; ++ i ) newVal[k] = newVal[k] * a[k] + SSS[offsetB + i];

      if ( oldVal[k] * newVal[k] < 0. ) ++ varSgn[k];
      if ( newVal[k] ) oldVal[k] = newVal[k];
      }

    offsetR = offsetB + degSSS[nSSS] + 1;
    degSSS[nSSS + 1] = polynomialEucliDivOppositeR( SSS + offsetA, degSSS[nSSS - 1], SSS + offsetB, degSSS[nSSS], SSS + offsetR );
   
    offsetA = offsetB;
    offsetB = offsetR;
   }

  delete [] degSSS;
  delete [] SSS;

  return varSgn[0] - varSgn[1];
}

//----------------------------------------------------------------------------
// Find all real roots in ] a[0] ; a[1] ] of a real 
// d-th degree polynomial using Sturm's theorem.
int vtkPolynomialSolvers::SturmBisectionSolve( double* P, int d, double* a, double *lowerBnds, double tol )
{
  // 0. Stupidity checks

  if ( tol <= 0 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Tolerance must be positive");
    return -1;
    }

  if ( ! P[0] )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Zero leading coefficient");
    return -1;
    }

  if ( d < 1 )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Degree < 1");
    return -1;
    }

  if ( a[1] < a[0] + tol )
    {
    vtkGenericWarningMacro(<<"vtkPolynomialSolvers::SturmRootBisectionSolve: Erroneous interval endpoints and/or tolerance");
    return -1;
    }

  // 1. Root counting

  double* SSS = new double[( d + 1 ) * ( d + 2 ) / 2];
  int* degSSS = new int[d + 2];
  
  int offsetA = 0;
  degSSS[0] = d;
  SSS[0] = P[0];
  SSS[d] = P[d];

  int offsetB = d + 1;
  degSSS[1] = d - 1;
  SSS[offsetB] = static_cast<double>(d) * P[0];

  int i;
  double oldVal[] = { P[0], P[0] };
  for ( i = 1; i < d; ++ i ) 
    {
    SSS[i] = P[i];
    SSS[offsetB + i] = static_cast<double>( d - i ) * P[i];
    for ( int k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * a[k] + P[i];
    }
  for ( int k = 0; k < 2; ++ k ) oldVal[k] = oldVal[k] * a[k] + P[d];

  int varSgn[] = { 0, 0 };
  int offsetR;
  int nSSS = 1;
  for ( ; degSSS[nSSS] > -1; ++ nSSS )
    {
    double newVal[] = { SSS[offsetB], SSS[offsetB] };
    for ( int k = 0; k < 2; ++ k )
      {
      for ( i = 1; i <= degSSS[nSSS]; ++ i ) newVal[k] = newVal[k] * a[k] + SSS[offsetB + i];
      if ( oldVal[k] * newVal[k] < 0. ) ++ varSgn[k];
      if ( newVal[k] ) oldVal[k] = newVal[k];
      }

    offsetR = offsetB + degSSS[nSSS] + 1;
    degSSS[nSSS + 1] = polynomialEucliDivOppositeR( SSS + offsetA, degSSS[nSSS - 1], SSS + offsetB, degSSS[nSSS], SSS + offsetR );
   
    offsetA = offsetB;
    offsetB = offsetR;
   }

  int nRoots = varSgn[0] - varSgn[1];
  if ( ! nRoots ) return 0;

  // 2. Root bracketing

  lowerBnds[0] = a[0];
  double localTol = a[1] - a[0];

  int* lowerVarSgn = new int[nRoots];
  int* upperVarSgn = new int[nRoots];
  lowerVarSgn[0] = varSgn[0];
  upperVarSgn[0] = varSgn[1];

  int midVarSgn, nIntervals = 1;
  double x, xOldVal, xVal;
  while ( nIntervals < nRoots && localTol > tol )
    {
    localTol *= .5;
    int nloc = nIntervals;
    for ( i = 0; i < nloc; ++ i )
      {
      x = lowerBnds[i] + localTol;
      offsetA = 0;
      xOldVal = 0.;
      midVarSgn = 0;
      for ( int j = 0; j < nSSS; offsetA += degSSS[j ++] + 1 )
        {
        xVal = evaluateHorner( SSS + offsetA, degSSS[j], x );
        if ( xOldVal * xVal < 0. ) ++ midVarSgn;
        if ( xVal ) xOldVal = xVal;
        }
    
      if ( midVarSgn == lowerVarSgn[i] ) lowerBnds[i] = x;
      else if ( midVarSgn != upperVarSgn[i] )
        {
        lowerBnds[nIntervals] = x;
        upperVarSgn[nIntervals] = upperVarSgn[i];
        lowerVarSgn[nIntervals ++] = upperVarSgn[i] = midVarSgn;
        }
      }
    }
  
  delete [] lowerVarSgn;
  delete [] upperVarSgn;
  delete [] degSSS;
  delete [] SSS;

  if ( localTol <= tol ) return nIntervals;

  // 3. Root polishing

  double* lowerVals = new double[nIntervals];
  for ( i = 0; i < nIntervals; ++ i ) lowerVals[i] = evaluateHorner( P, d, lowerBnds[i] );

  while ( localTol > tol )
    {
    localTol *= .5;
    for ( i = 0; i < nIntervals; ++ i )
      {
      x = lowerBnds[i] + localTol;
      xVal = evaluateHorner( P, d, x );
      if ( lowerVals[i] * xVal > 0. )
        {
        lowerBnds[i] = x;
        lowerVals[i] = xVal;
        }
      }
    }

  delete [] lowerVals;

  return nIntervals;
}

//----------------------------------------------------------------------------
// Solves a d-th degree polynomial equation using Lin-Bairstow's method.
//
int vtkPolynomialSolvers::LinBairstowSolve( double* c, int d, double* r, double& tolerance )
{
  if ( ! c[0] )
    {
    vtkGenericWarningMacro(<<"vtkMath::LinBairstowSolve: Zero leading coefficient");
    return 0;
    }

  int i;
  int dp1 = d + 1;
  for ( i = 1 ; i < dp1; ++ i )
    {
    c[i] /= c[0];
    }
 
  double* div1 = new double[dp1];
  double* div2 = new double[dp1];
  div1[0] = div2[0] = 1;
  for ( i = d ; i > 2; i -= 2 )
    {
    double det, detR, detS;
    double R = 0.;
    double S = 0.;
    double dR = 1.;
    double dS = 0.;
    int nIterations = 1;

    while ( ( fabs( dR ) + fabs( dS ) ) > tolerance )
      {
      if ( ! ( nIterations % 100 ) )
        {
        R = vtkMath::Random( 0., 2. );
        if ( ! ( nIterations % 200 ) ) tolerance *= 4.;
        }

      div1[1] = c[1] - R;
      div2[1] = div1[1] - R;

      for ( int j = 2; j <= i; ++ j )
        {
        div1[j] = c[j] - R * div1[j - 1] - S * div1[j - 2];
        div2[j] = div1[j] - R * div2[j - 1] - S * div2[j - 2];
        }

      det  = div2[i - 1] * div2[i -3]  - div2[i - 2] * div2[i - 2];
      detR = div1[i]     * div2[i -3]  - div1[i - 1] * div2[i - 2];
      detS = div1[i - 1] * div2[i - 1] - div1[i]     * div2[i - 2];

      if ( fabs( det ) < VTK_DBL_EPSILON )
        {
        det = detR = detS = 1.;
        }

      dR = detR / det;
      dS = detS / det;
      R += dR;
      S += dS;
      ++ nIterations;
      }

    for ( int j = 0; j < i - 1; ++ j ) c[j] = div1[j];
    c[i] = S;
    c[i - 1] = R;
    }

  int nr = 0;  
  for ( i = d; i >= 2; i -= 2 )
    {
    double delta = c[i - 1] * c[i - 1] - 4. * c[i];
    if ( delta >= 0 )
      {
      // check whether there is 2 simple or 1 double root(s)
      if ( delta )
        {
        delta = sqrt( delta );
        // we have 2 simple real roots
        r[nr ++] = ( - c[i - 1] - delta ) / 2.;
        // insert 2nd simple real root
        r[nr ++] = ( - c[i - 1] + delta ) / 2.;
        }
      else
        {
        // we have a double real root
        r[nr ++] = - c[1];
        r[nr ++] = - c[1];
        }
      }
    }
  if ( ( d % 2 ) == 1 )
    {
    // what's left when degree is odd
    r[nr ++] = - c[1];
    }

  delete [] div1;
  delete [] div2;
  return nr;
}

