// Filename: lsimpleMatrix.h
// Created by:  drose (15Dec11)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifndef LSIMPLEMATRIX_H
#define LSIMPLEMATRIX_H

#include "pandabase.h"

#ifdef HAVE_EIGEN
#include <Eigen/Dense>
#endif

////////////////////////////////////////////////////////////////////
//       Class : LSimpleMatrix
// Description : This class provides an underlying storage of the
//               various linear-algebra classes (e.g. LVecBase3,
//               LMatrix4) in the absence of the Eigen linear algebra
//               library.
////////////////////////////////////////////////////////////////////
template <class FloatType, int NumRows, int NumCols>
class LSimpleMatrix {
public:
  INLINE LSimpleMatrix();
  INLINE LSimpleMatrix(const LSimpleMatrix<FloatType, NumRows, NumCols> &copy);
  INLINE void operator = (const LSimpleMatrix<FloatType, NumRows, NumCols> &copy);
  INLINE const FloatType &operator () (int row, int col) const;
  INLINE FloatType &operator () (int row, int col);
  INLINE const FloatType &operator () (int col) const;
  INLINE FloatType &operator () (int col);

private:
  FloatType _array[NumRows][NumCols];
};

#include "lsimpleMatrix.I"

// Now, do we actually use LSimpleMatrix, or do we use Eigen::Matrix?
#ifdef HAVE_EIGEN
#ifdef LINMATH_ALIGN
#define LINMATH_MATRIX(FloatType, NumRows, NumCols) Eigen::Matrix<FloatType, NumRows, NumCols, Eigen::RowMajor>
#else  // LINMATH_ALIGN
#define LINMATH_MATRIX(FloatType, NumRows, NumCols) Eigen::Matrix<FloatType, NumRows, NumCols, Eigen::DontAlign | Eigen::RowMajor>
#endif  // LINMATH_ALIGN

#else  // HAVE_EIGEN
#define LINMATH_MATRIX(FloatType, NumRows, NumCols) LSimpleMatrix<FloatType, NumRows, NumCols>
#endif  // HAVE_EIGEN

#define SIMPLE_MATRIX(FloatType, NumRows, NumCols) LSimpleMatrix<FloatType, NumRows, NumCols>

// This is as good a place as any to define this alignment macro.
#ifdef LINMATH_ALIGN
#define ALIGN_LINMATH ALIGN_16BYTE
#else
#define ALIGN_LINMATH 
#endif  // LINMATH_ALIGN

#endif

  
