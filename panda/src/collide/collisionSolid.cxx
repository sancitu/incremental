// Filename: collisionSolid.cxx
// Created by:  drose (24Apr00)
// 
////////////////////////////////////////////////////////////////////

#include "collisionSolid.h"
#include "config_collide.h"
#include "collisionEntry.h"

#include <renderRelation.h>
#include <geomNode.h>
#include <cullFaceTransition.h>
#include <renderModeTransition.h>
#include <lightTransition.h>
#include <textureTransition.h>
#include <transparencyTransition.h>
#include <colorTransition.h>
#include <datagram.h>
#include <datagramIterator.h>
#include <bamReader.h>
#include <bamWriter.h>
#include <indent.h>

TypeHandle CollisionSolid::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
CollisionSolid::
CollisionSolid() {
  _viz_stale = true;
  _tangible = true;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
CollisionSolid::
CollisionSolid(const CollisionSolid &copy) :
  _tangible(copy._tangible)
{
  // Actually, there's not a whole lot here we want to copy.
  _viz_stale = true;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
CollisionSolid::
~CollisionSolid() {
  clear_viz_arcs();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::update_viz
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
update_viz(Node *parent) {
  if (_viz_stale) {
    clear_viz_arcs();
    recompute_viz(parent);
    _viz_stale = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::output
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
output(ostream &out) const {
  out << get_type();
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::write
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
write(ostream &out, int indent_level) const {
  indent(out, indent_level) << (*this) << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::test_intersection_from_sphere
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int CollisionSolid::
test_intersection_from_sphere(CollisionHandler *,
			      const CollisionEntry &) const {
  collide_cat.warning()
    << get_type() << "::test_intersection_from_sphere() called!\n";
  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::test_intersection_from_ray
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int CollisionSolid::
test_intersection_from_ray(CollisionHandler *,
			   const CollisionEntry &) const {
  collide_cat.warning()
    << get_type() << "::test_intersection_from_ray() called!\n";
  return 0;
}


////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::clear_viz_arcs
//       Access: Protected
//  Description: Removes all of the solids' visualization geometry and
//               marks the viz as stale so it will be recomputed next
//               time the solid is rendered.
////////////////////////////////////////////////////////////////////
void CollisionSolid::
clear_viz_arcs() {
  VizArcs::iterator vi;
  for (vi = _solid_viz_arcs.begin(); vi != _solid_viz_arcs.end(); ++vi) {
    remove_arc(*vi);
  }
  for (vi = _wireframe_viz_arcs.begin(); vi != _wireframe_viz_arcs.end(); ++vi) {
    remove_arc(*vi);
  }
  for (vi = _other_viz_arcs.begin(); vi != _other_viz_arcs.end(); ++vi) {
    remove_arc(*vi);
  }
  _solid_viz_arcs.clear();
  _wireframe_viz_arcs.clear();
  _other_viz_arcs.clear();
  _viz_stale = true;
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::add_solid_viz
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
add_solid_viz(Node *parent, GeomNode *viz) {
  RenderRelation *arc = new RenderRelation(parent, viz);
  arc->set_transition(new CullFaceTransition(CullFaceProperty::M_cull_clockwise));
  arc->set_transition(new RenderModeTransition(RenderModeProperty::M_filled));
  arc->set_transition(new LightTransition(LightTransition::all_off()));
  arc->set_transition(new TextureTransition(TextureTransition::off()));
  arc->set_transition(new TransparencyTransition(TransparencyProperty::M_alpha));

  if (is_tangible()) {
    arc->set_transition(new ColorTransition(1.0, 1.0, 1.0, 0.5));
  } else {
    arc->set_transition(new ColorTransition(1.0, 0.3, 0.5, 0.5));
  }

  _solid_viz_arcs.push_back(arc);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::add_wireframe_viz
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
add_wireframe_viz(Node *parent, GeomNode *viz) {
  RenderRelation *arc = new RenderRelation(parent, viz);
  arc->set_transition(new CullFaceTransition(CullFaceProperty::M_cull_none));
  arc->set_transition(new RenderModeTransition(RenderModeProperty::M_wireframe));
  arc->set_transition(new LightTransition(LightTransition::all_off()));
  arc->set_transition(new TextureTransition(TextureTransition::off()));
  arc->set_transition(new TransparencyTransition(TransparencyProperty::M_none));
  if (is_tangible()) {
    arc->set_transition(new ColorTransition(0.0, 0.0, 1.0, 1.0));
  } else {
    arc->set_transition(new ColorTransition(1.0, 1.0, 0.0, 1.0));
  }

  _wireframe_viz_arcs.push_back(arc);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void CollisionSolid::
write_datagram(BamWriter *, Datagram &me)
{
  me.add_uint8(_tangible);
}

////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void CollisionSolid::
fillin(DatagramIterator& scan, BamReader*)
{
  _tangible = (scan.get_uint8() != 0);
}


////////////////////////////////////////////////////////////////////
//     Function: CollisionSolid::add_other_viz
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
void CollisionSolid::
add_other_viz(Node *parent, GeomNode *viz) {
  RenderRelation *arc = new RenderRelation(parent, viz);
  arc->set_transition(new CullFaceTransition(CullFaceProperty::M_cull_clockwise));
  arc->set_transition(new RenderModeTransition(RenderModeProperty::M_filled));
  arc->set_transition(new LightTransition(LightTransition::all_off()));
  arc->set_transition(new TextureTransition(TextureTransition::off()));
  arc->set_transition(new TransparencyTransition(TransparencyProperty::M_alpha));

  _other_viz_arcs.push_back(arc);
}
