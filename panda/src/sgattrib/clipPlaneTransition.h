// Filename: clipPlaneTransition.h
// Created by:  mike (19Jan99)
//
////////////////////////////////////////////////////////////////////

#ifndef CLIPPLANETRANSITION_H
#define CLIPPLANETRANSITION_H

#include <pandabase.h>

#include <multiNodeTransition.h>

#include <planeNode.h>
#include <pointerTo.h>

////////////////////////////////////////////////////////////////////
// 	 Class : ClipPlaneTransition
// Description : This allows the definition of zero or more arbitrary
//               clipping planes.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA ClipPlaneTransition : public MultiNodeTransition {
public:
  INLINE ClipPlaneTransition();
  INLINE static ClipPlaneTransition all_off();

  INLINE void set_identity(const PT(PlaneNode) &prop);
  INLINE void set_on(const PT(PlaneNode) &prop);
  INLINE void set_off(const PT(PlaneNode) &prop);

  INLINE bool is_identity(const PT(PlaneNode) &prop) const;
  INLINE bool is_on(const PT(PlaneNode) &prop) const;
  INLINE bool is_off(const PT(PlaneNode) &prop) const;
  
  virtual NodeTransition *make_copy() const;
  virtual NodeAttribute *make_attrib() const;
  virtual NodeTransition *make_identity() const;

protected:
  virtual void output_property(ostream &out, const PT_Node &prop) const;
  virtual void write_property(ostream &out, const PT_Node &prop,
			      int indent_level) const;

public:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    MultiNodeTransition::init_type();
    register_type(_type_handle, "ClipPlaneTransition",
		  MultiNodeTransition::get_class_type());
  }

private:
  static TypeHandle _type_handle;
  friend class ClipPlaneAttribute;
};

#include "clipPlaneTransition.I"

#endif


