// Filename: check_md5.cxx
// Created by:
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

#include "pandabase.h"

#include "hashVal.h"
#include "filename.h"

#ifndef HAVE_GETOPT
#include <gnu_getopt.h>
#else
#include <getopt.h>
#endif

void
usage() {
  cerr << 
    "\n"
    "Usage:\n\n"
    "check_md5 [-d] [-i \"input string\"] [file1 file2 ...]\n"
    "check_md5 -h\n\n";
}

void
help() {
  usage();
  cerr << 
    "This program outputs the MD5 hash of one or more files (or of a string\n"
    "passed on the command line with -i).\n\n"

    "An MD5 hash is a 128-bit value.  The output is presented as a 32-digit\n"
    "hexadecimal string by default, but with -d, it is presented as four\n"
    "big-endian unsigned 32-bit decimal integers.\n\n";
}

void
output_hash(const string &filename, const HashVal &hash, bool output_decimal) {
  if (!filename.empty()) {
    cout << filename << " ";
  }
  if (output_decimal) {
    hash.output_dec(cout);
  } else {
    hash.output_hex(cout);
  }
  cout << "\n";
}
  

int
main(int argc, char *argv[]) {
  extern char *optarg;
  extern int optind;
  const char *optstr = "i:dh";

  bool got_input_string = false;
  string input_string;
  bool output_decimal = false;

  int flag = getopt(argc, argv, optstr);

  while (flag != EOF) {
    switch (flag) {
    case 'i':
      got_input_string = true;
      input_string = optarg;
      break;

    case 'd':
      output_decimal = true;
      break;

    case 'h':
      help();
      exit(1);

    default:
      exit(1);
    }
    flag = getopt(argc, argv, optstr);
  }

  argc -= (optind-1);
  argv += (optind-1);

  if (argc < 2 && !got_input_string) {
    usage();
    exit(1);
  }

  if (got_input_string) {
    HashVal hash;
    hash.hash_string(input_string);
    output_hash("", hash, output_decimal);
  }

  bool okflag = true;

  for (int i = 1; i < argc; i++) {
    Filename source_file = Filename::from_os_specific(argv[i]);

    if (!source_file.exists()) {
      cerr << source_file << " not found!\n";
      okflag = false;
    } else {
      HashVal hash;
      if (!hash.hash_file(source_file)) {
        cerr << "Unable to read " << source_file << "\n";
        okflag = false;
      } else {
        output_hash(source_file.get_basename(), hash, output_decimal);
      }
    }
  }

  if (!okflag) {
    exit(1);
  }

  return 0;
}
