// Filename: bioStream.h
// Created by:  drose (25Sep02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef BIOSTREAM_H
#define BIOSTREAM_H

#include "pandabase.h"

// This module is not compiled if OpenSSL is not available.
#ifdef HAVE_SSL

#include "bioStreamBuf.h"

////////////////////////////////////////////////////////////////////
//       Class : IBioStream
// Description : An input stream object that reads data from an
//               OpenSSL BIO object.  This is used by the HTTPClient
//               and HTTPDocument classes to provide a C++ interface
//               to OpenSSL.
//
//               Seeking is not supported.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS IBioStream : public istream {
public:
  INLINE IBioStream();
  INLINE IBioStream(BIO *source, bool owns_source);

  INLINE IBioStream &open(BIO *source, bool owns_source);
  INLINE IBioStream &close();

private:
  BioStreamBuf _buf;
};

#include "bioStream.I"

#endif  // HAVE_SSL


#endif


