// Filename: loaderFileTypeEgg.h
// Created by:  drose (20Jun00)
// 
////////////////////////////////////////////////////////////////////

#ifndef LOADERFILETYPEEGG_H
#define LOADERFILETYPEEGG_H

#include <pandabase.h>

#include <loaderFileType.h>

////////////////////////////////////////////////////////////////////
// 	 Class : LoaderFileTypeEgg
// Description : This defines the Loader interface to read Egg files.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG LoaderFileTypeEgg : public LoaderFileType {
public:
  LoaderFileTypeEgg();

  virtual string get_name() const;
  virtual string get_extension() const;

  virtual void resolve_filename(Filename &path) const;
  virtual PT_Node load_file(const Filename &path, bool report_errors) const;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    LoaderFileType::init_type();
    register_type(_type_handle, "LoaderFileTypeEgg",
                  LoaderFileType::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
 
private:
  static TypeHandle _type_handle;
};

#endif

