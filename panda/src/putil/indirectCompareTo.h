// Filename: indirectCompareTo.h
// Created by:  drose (04Apr00)
// 
////////////////////////////////////////////////////////////////////

#ifndef INDIRECTCOMPARETO_H
#define INDIRECTCOMPARETO_H

#include <pandabase.h>

////////////////////////////////////////////////////////////////////
// 	 Class : IndirectCompareTo
// Description : An STL function object class, this is intended to be
//               used on any ordered collection of pointers to classes
//               that contain a compare_to() method.  It defines the
//               order of the pointers via compare_to().
////////////////////////////////////////////////////////////////////
template<class ObjectType>
class IndirectCompareTo {
public:
  INLINE bool operator () (const ObjectType *a, const ObjectType *b) const;
};

#include "indirectCompareTo.I"

#endif

