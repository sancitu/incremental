// Filename: testCopy.cxx
// Created by:  drose (31Oct00)
// 
////////////////////////////////////////////////////////////////////

#include "testCopy.h"
#include "cvsSourceDirectory.h"

////////////////////////////////////////////////////////////////////
//     Function: TestCopy::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
TestCopy::
TestCopy() {
  set_program_description
    ("This program copies one or more files into a CVS source hierarchy.  "
     "Rather than copying the named files immediately into the current "
     "directory, it first scans the entire source hierarchy, identifying all "
     "the already-existing files.  If the named file to copy matches the "
     "name of an already-existing file in the current directory or elsewhere "
     "in the hierarchy, that file is overwritten.\n\n"

     "This is primarily useful as a test program for libcvscopy.");
}

////////////////////////////////////////////////////////////////////
//     Function: TestCopy::run
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void TestCopy::
run() {
  SourceFiles::iterator fi;
  for (fi = _source_files.begin(); fi != _source_files.end(); ++fi) {
    CVSSourceDirectory *dest = import(*fi, 0, _model_dir);
    if (dest == (CVSSourceDirectory *)NULL) {
      exit(1);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TestCopy::copy_file
//       Access: Protected, Virtual
//  Description: Called by import() if the timestamps indicate that a
//               file needs to be copied.  This does the actual copy
//               of a file from source to destination.  If new_file is
//               true, then dest does not already exist.
////////////////////////////////////////////////////////////////////
bool TestCopy::
copy_file(Filename source, Filename dest,
	  CVSSourceDirectory *dir, int type, bool new_file) {
  source.set_binary();
  dest.set_binary();

  ifstream in;
  ofstream out;
  if (!source.open_read(in)) {
    cerr << "Cannot read " << source << "\n";
    return false;
  }

  if (!dest.open_write(out)) {
    cerr << "Cannot write " << dest << "\n";
    return false;
  }

  int c;
  c = in.get();
  while (!in.eof() && !in.fail() && !out.fail()) {
    out.put(c);
    c = in.get();
  }

  if (in.fail()) {
    cerr << "Error reading " << source << "\n";
    return false;
  }
  if (out.fail()) {
    cerr << "Error writing " << dest << "\n";
    return false;
  }
  
  return true;
}


int main(int argc, char *argv[]) {
  TestCopy prog;
  prog.parse_command_line(argc, argv);
  prog.run();
  return 0;
}
