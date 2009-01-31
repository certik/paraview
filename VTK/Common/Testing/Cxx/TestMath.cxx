/*
 * Copyright 2005 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkMath.h"

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

//=============================================================================
// Helpful class for storing and using color triples.
class Triple {
public:
  Triple() {};
  Triple(double a, double b, double c) {
    data[0] = a; data[1] = b; data[2] = c;
  }
  const double *operator()() const { return data; }
  double *operator()() { return data; }
  const double &operator[](int i) const { return data[i]; }
  double &operator[](int i) { return data[i]; }
  bool operator==(const Triple &triple) const {
    return *this == triple.data;
  }
  bool operator==(const double *triple) const {
    return (   (this->data[0] - triple[0] <= 0.01*ABS(data[0])+0.02)
            && (this->data[0] - triple[0] >= -0.01*ABS(data[0])-0.02)
            && (this->data[1] - triple[1] <= 0.01*ABS(data[1])+0.02)
            && (this->data[1] - triple[1] >= -0.01*ABS(data[1])-0.02)
            && (this->data[2] - triple[2] <= 0.01*ABS(data[2])+0.02)
            && (this->data[2] - triple[2] >= -0.01*ABS(data[2])-0.02) );
  }
  bool operator!=(const Triple &triple) const {
    return *this != triple.data;
  }
  bool operator!=(const double *triple) const {
    return !(*this == triple);
  }
private:
  double data[3];
};

static ostream &operator<<(ostream &os, const Triple t)
{
  os << t[0] << ", " << t[1] << ", " << t[2];
  return os;
}

//=============================================================================
// Function for comparing colors.  Each value should be equivalent in the
// respective color space.
static int TestColorConvert(const Triple &rgb, const Triple &hsv,
                            const Triple &xyz, const Triple &lab);

int TestMath(int,char *[])
{
  int testIntValue;
  
  testIntValue = vtkMath::Factorial(5);
  if ( testIntValue != 120 )
    {
    vtkGenericWarningMacro("Factorial(5) = "<<testIntValue<<" != 120");
    return 1;
    }

  testIntValue = vtkMath::Binomial(8,3);
  if ( testIntValue != 56 )
    {
    vtkGenericWarningMacro("Binomial(8,3) = "<<testIntValue<<" != 56");
    return 1;
    }

  testIntValue = vtkMath::Binomial(5,3);
  if ( testIntValue != 10 )
    {
    vtkGenericWarningMacro("Binomial(5,3) = "<<testIntValue<<" != 10");
    return 1;
    }

  // Solving x(x - 10^-4)^2 = 0 illustrates how the Tartaglia-Cardan solver
  // filters some numerical noise by noticing there is a double root (that
  // SolveCubic does not notice).
  double c[] = { 1., -2.e-4, 1.e-8, 0.};
#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( c[0], c[1], c[2], c[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 3");
    return 1;
    }
#endif // 0  
  double r[3];
  int m[3];
  testIntValue = vtkMath::TartagliaCardanSolve( c, r, m );
  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 2");
    return 1;
    }

  // Test color conversion.
  int colorsPassed = 1;

  colorsPassed &= TestColorConvert(Triple(1.0, 1.0, 1.0),               // RGB
                                   Triple(0.0, 0.0, 1.0),   // HSV (H ambiguous)
                                   Triple(95.047, 100.0, 108.883),      // XYZ
                                   Triple(100.0, 0.0, 0.0));            // L*ab

  colorsPassed &= TestColorConvert(Triple(0.5, 0.5, 0.0),               // RGB
                                   Triple(1.0/6.0, 1.0, 0.5),           // HSV
                                   Triple(16.48, 19.86, 2.96),          // XYZ
                                   Triple(51.68, -12.89, 56.52));       // L*ab

  colorsPassed &= TestColorConvert(Triple(0.25, 0.25, 0.5),             // RGB
                                   Triple(2.0/3.0, 0.5, 0.5),           // HSV
                                   Triple(7.78, 6.27, 21.05),           // XYZ
                                   Triple(30.07, 18.51, -36.21));       // L*ab

  colorsPassed &= TestColorConvert(Triple(0.0, 0.0, 0.0),               // RGB
                                   Triple(0.0, 0.0, 0.0), // HSV (H&S ambiguous)
                                   Triple(0.0, 0.0, 0.0),               // XYZ
                                   Triple(0.0, 0.0, 0.0));              // L*ab
                                   
  if (!colorsPassed)
    {
    return 1;
    }
                                              

  return 0;
}

static int TestColorConvert(const Triple &rgb, const Triple &hsv,
                            const Triple &xyz, const Triple &lab)
{
  cout << "Ensuring the following colors are consistent: " << endl;
  cout << "   RGB:      " << rgb << endl;
  cout << "   HSV:      " << hsv << endl;
  cout << "   CIE XYZ:  " << xyz << endl;
  cout << "   CIE-L*ab: " << lab << endl;

  Triple result1;
  double *result2;

#define COMPARE(testname, target, dest) \
  if (target != dest)                              \
    { \
    vtkGenericWarningMacro(<< "Incorrect " #testname " conversion.  Got " \
                           << dest << " expected " << target); \
    return 0; \
    }

  // Test conversion between RGB and HSV.
  vtkMath::RGBToHSV(rgb(), result1());
  COMPARE(RGBToHSV, hsv, result1);
  vtkMath::HSVToRGB(hsv(), result1());
  COMPARE(HSVToRGB, rgb, result1);

  result2 = vtkMath::RGBToHSV(rgb());
  COMPARE(RGBToHSV, hsv, result2);
  result2 = vtkMath::HSVToRGB(hsv());
  COMPARE(HSVToRGB, rgb, result2);

  vtkMath::RGBToHSV(rgb[0], rgb[1], rgb[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToHSV, hsv, result1);
  vtkMath::HSVToRGB(hsv[0], hsv[1], hsv[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(HSVToRGB, rgb, result1);

  // Test conversion between RGB and XYZ.
  vtkMath::RGBToXYZ(rgb(), result1());
  COMPARE(RGBToXYZ, xyz, result1);
  vtkMath::XYZToRGB(xyz(), result1());
  COMPARE(XYZToRGB, rgb, result1);

  result2 = vtkMath::RGBToXYZ(rgb());
  COMPARE(RGBToXYZ, xyz, result2);
  result2 = vtkMath::XYZToRGB(xyz());
  COMPARE(XYZToRGB, rgb, result2);

  vtkMath::RGBToXYZ(rgb[0], rgb[1], rgb[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToXYZ, xyz, result1);
  vtkMath::XYZToRGB(xyz[0], xyz[1], xyz[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(XYZToRGB, rgb, result1);

  // Test conversion between Lab and XYZ.
  vtkMath::LabToXYZ(lab(), result1());
  COMPARE(LabToXYZ, xyz, result1);
  vtkMath::XYZToLab(xyz(), result1());
  COMPARE(XYZToLab, lab, result1);

  result2 = vtkMath::LabToXYZ(lab());
  COMPARE(LabToXYZ, xyz, result2);
  result2 = vtkMath::XYZToLab(xyz());
  COMPARE(XYZToLab, lab, result2);

  vtkMath::LabToXYZ(lab[0], lab[1], lab[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(LabToXYZ, xyz, result1);
  vtkMath::XYZToLab(xyz[0], xyz[1], xyz[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(XYZToLab, lab, result1);

  // Test conversion between Lab and RGB.
  vtkMath::LabToRGB(lab(), result1());
  COMPARE(LabToRGB, rgb, result1);
  vtkMath::RGBToLab(rgb(), result1());
  COMPARE(RGBToLab, lab, result1);

  result2 = vtkMath::LabToRGB(lab());
  COMPARE(LabToRGB, rgb, result2);
  result2 = vtkMath::RGBToLab(rgb());
  COMPARE(RGBToLab, lab, result2);

  vtkMath::LabToRGB(lab[0], lab[1], lab[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(LabToRGB, rgb, result1);
  vtkMath::RGBToLab(rgb[0], rgb[1], rgb[2],
                    &result1[0], &result1[1], &result1[2]);
  COMPARE(RGBToLab, lab, result1);

  return 1;
}
