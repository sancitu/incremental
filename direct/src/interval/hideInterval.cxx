// Filename: hideInterval.cxx
// Created by:  drose (27Aug02)
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

#include "hideInterval.h"

int HideInterval::_unique_index;
TypeHandle HideInterval::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: HideInterval::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
HideInterval::
HideInterval(const NodePath &node, const string &name) :
  CInterval(name, 0.0, true),
  _node(node)
{
  nassertv(!node.is_empty());
  if (_name.empty()) {
    ostringstream name_strm;
    name_strm
      << "HideInterval-" << node.node()->get_name() << "-" << ++_unique_index;
    _name = name_strm.str();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: HideInterval::instant
//       Access: Published, Virtual
//  Description: This is called in lieu of initialize() .. step()
//               .. finalize(), when everything is to happen within
//               one frame.  The interval should initialize itself,
//               then leave itself in the final state.
////////////////////////////////////////////////////////////////////
void HideInterval::
instant() {
  _node.hide();
}

////////////////////////////////////////////////////////////////////
//     Function: HideInterval::reverse_instant
//       Access: Published, Virtual
//  Description: This is called in lieu of reverse_initialize()
//               .. step() .. reverse_finalize(), when everything is
//               to happen within one frame.  The interval should
//               initialize itself, then leave itself in the initial
//               state.
////////////////////////////////////////////////////////////////////
void HideInterval::
reverse_instant() {
  _node.show();
}
