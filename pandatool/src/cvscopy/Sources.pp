#begin lib_target
  #define TARGET cvscopy
  #define LOCAL_LIBS \
    progbase pandatoolbase

  #define OTHER_LIBS \
    dconfig:c dtool:m

  #define SOURCES \
    cvsCopy.cxx cvsCopy.h cvsSourceDirectory.cxx cvsSourceDirectory.h \
    cvsSourceTree.cxx cvsSourceTree.h

  #define INSTALL_HEADERS \
    cvsCopy.h

#end lib_target

#begin test_bin_target
  #define TARGET testcopy
  #define LOCAL_LIBS cvscopy

  #define OTHER_LIBS \
    dconfig:c dtool:m pystub

  #define SOURCES \
    testCopy.cxx testCopy.h

#end test_bin_target
