// Filename: colorBlendTransition.cxx
// Created by:  mike (28Jan99)
// 
////////////////////////////////////////////////////////////////////

#include "colorBlendTransition.h"
#include "colorBlendAttribute.h"

#include <indent.h>

TypeHandle ColorBlendTransition::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::make_copy
//       Access: Public, Virtual
//  Description: Returns a newly allocated ColorBlendTransition just like
//               this one.
////////////////////////////////////////////////////////////////////
NodeTransition *ColorBlendTransition::
make_copy() const {
  return new ColorBlendTransition(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::make_attrib
//       Access: Public, Virtual
//  Description: Returns a newly allocated ColorBlendAttribute.
////////////////////////////////////////////////////////////////////
NodeAttribute *ColorBlendTransition::
make_attrib() const {
  return new ColorBlendAttribute;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::set_value_from
//       Access: Protected, Virtual
//  Description: Copies the value from the other transition pointer,
//               which is guaranteed to be another ColorBlendTransition.
////////////////////////////////////////////////////////////////////
void ColorBlendTransition::
set_value_from(const OnTransition *other) {
  const ColorBlendTransition *ot;
  DCAST_INTO_V(ot, other);
  _value = ot->_value;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::compare_values
//       Access: Protected, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int ColorBlendTransition::
compare_values(const OnTransition *other) const {
  const ColorBlendTransition *ot;
  DCAST_INTO_R(ot, other, false);
  return _value.compare_to(ot->_value);
}

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::output_value
//       Access: Protected, Virtual
//  Description: Formats the value for human consumption on one line.
////////////////////////////////////////////////////////////////////
void ColorBlendTransition::
output_value(ostream &out) const {
  out << _value;
}

////////////////////////////////////////////////////////////////////
//     Function: ColorBlendTransition::write_value
//       Access: Protected, Virtual
//  Description: Formats the value for human consumption on multiple
//               lines if necessary.
////////////////////////////////////////////////////////////////////
void ColorBlendTransition::
write_value(ostream &out, int indent_level) const {
  indent(out, indent_level) << _value << "\n";
}
