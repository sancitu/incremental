// Filename: configVariableBase.I
// Created by:  drose (21Oct04)
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


////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::Constructor
//       Access: Protected
//  Description: This constructor is only intended to be called from a
//               specialized ConfigVariableFoo derived class.
////////////////////////////////////////////////////////////////////
INLINE ConfigVariableBase::
ConfigVariableBase(const string &name, 
                   ConfigVariableBase::ValueType value_type) :
  _core(ConfigVariableManager::get_global_ptr()->make_variable(name))
{
  if (value_type != VT_undefined) {
    _core->set_value_type(value_type);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::Destructor
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE ConfigVariableBase::
~ConfigVariableBase() {
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::get_name
//       Access: Published
//  Description: Returns the name of the variable.
////////////////////////////////////////////////////////////////////
INLINE const string &ConfigVariableBase::
get_name() const {
  nassertr(_core != (ConfigVariableCore *)NULL, *new string());
  return _core->get_name();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::get_value_type
//       Access: Published
//  Description: Returns the stated type of this variable.  This
//               should be VT_list, unless a later variable
//               declaration has changed it.
////////////////////////////////////////////////////////////////////
INLINE ConfigVariableBase::ValueType ConfigVariableBase::
get_value_type() const {
  nassertr(_core != (ConfigVariableCore *)NULL, VT_undefined);
  return _core->get_value_type();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::get_description
//       Access: Published
//  Description: Returns the brief description of this variable, if
//               it has been defined.
////////////////////////////////////////////////////////////////////
INLINE const string &ConfigVariableBase::
get_description() const {
  nassertr(_core != (ConfigVariableCore *)NULL, *new string());
  return _core->get_description();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::get_flags
//       Access: Public
//  Description: Returns the flags value as set by set_flags().  This
//               includes the trust level and some other settings.
//               See the individual methods is_closed(),
//               get_trust_level(), etc. to pull out the semantic
//               meaning of these flags individually.
////////////////////////////////////////////////////////////////////
INLINE int ConfigVariableBase::
get_flags() const {
  nassertr(_core != (ConfigVariableCore *)NULL, 0);
  return _core->get_flags();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::is_closed
//       Access: Public
//  Description: Returns true if the variable is not trusted by any
//               prc file (and hence cannot be modified from its
//               compiled-in default value), or false for the normal
//               case, in which the variable can be modified by any
//               prc file at or above its trust level (see
//               get_trust_level()).
//
//               This value only has effect in a release build
//               (specifically, when PRC_RESPECT_TRUST_LEVEL is
//               defined true in Config.pp).
////////////////////////////////////////////////////////////////////
INLINE bool ConfigVariableBase::
is_closed() const {
  nassertr(_core != (ConfigVariableCore *)NULL, false);
  return _core->is_closed();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::get_trust_level
//       Access: Public
//  Description: Returns the minimum trust_level a prc file must
//               demonstrate in order to redefine the value for this
//               variable.  Arguably, this should be called the
//               "mistrust level", since the larger the value, the
//               more suspicious we are of prc files.  This value is
//               not used if is_closed() returns true, which indicates
//               no file may be trusted.
//
//               This value only has effect in a release build
//               (specifically, when PRC_RESPECT_TRUST_LEVEL is
//               defined true in Config.pp).
////////////////////////////////////////////////////////////////////
INLINE int ConfigVariableBase::
get_trust_level() const {
  nassertr(_core != (ConfigVariableCore *)NULL, 0);
  return _core->get_trust_level();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::is_dynamic
//       Access: Public
//  Description: Returns true if the variable was indicated as
//               "dynamic" by its constructor, indicating that its
//               name was dynamically generated, possibly from a large
//               pool, and it should not be listed along with the
//               other variables.
////////////////////////////////////////////////////////////////////
INLINE bool ConfigVariableBase::
is_dynamic() const {
  nassertr(_core != (ConfigVariableCore *)NULL, false);
  return _core->is_dynamic();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::clear_local_value
//       Access: Published
//  Description: Removes the local value defined for this variable,
//               and allows its value to be once again retrieved from
//               the .prc files.
//
//               Returns true if the value was successfully removed,
//               false if it did not exist in the first place.
////////////////////////////////////////////////////////////////////
INLINE bool ConfigVariableBase::
clear_local_value() {
  nassertr(_core != (ConfigVariableCore *)NULL, false);
  return _core->clear_local_value();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::has_local_value
//       Access: Published
//  Description: Returns true if this variable's value has been
//               shadowed by a local assignment (as created via
//               make_local_value()), or false otherwise.
////////////////////////////////////////////////////////////////////
INLINE bool ConfigVariableBase::
has_local_value() const {
  nassertr(_core != (ConfigVariableCore *)NULL, false);
  return _core->has_local_value();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::has_value
//       Access: Public
//  Description: Returns true if this variable has an explicit value,
//               either from a prc file or locally set, or false if
//               variable has its default value.
////////////////////////////////////////////////////////////////////
INLINE bool ConfigVariableBase::
has_value() const {
  nassertr(_core != (ConfigVariableCore *)NULL, false);
  return _core->has_value();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::output
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE void ConfigVariableBase::
output(ostream &out) const {
  nassertv(_core != (ConfigVariableCore *)NULL);
  _core->output(out);
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableBase::write
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
INLINE void ConfigVariableBase::
write(ostream &out) const {
  nassertv(_core != (ConfigVariableCore *)NULL);
  _core->write(out);
}

INLINE ostream &
operator << (ostream &out, const ConfigVariableBase &variable) {
  variable.output(out);
  return out;
}
