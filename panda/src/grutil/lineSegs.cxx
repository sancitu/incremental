// Filename: lineSegs.cxx
// Created by:  drose (16Mar02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "lineSegs.h"
#include "renderState.h"
#include "renderModeAttrib.h"
#include "geom.h"
#include "geomLinestrips.h"
#include "geomPoints.h"
#include "geomVertexReader.h"
#include "geomVertexWriter.h"

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::Constructor
//       Access: Public
//  Description: Constructs a LineSegs object, which can be used to
//               create any number of disconnected lines or points of
//               various thicknesses and colors through the visible
//               scene.  After creating the object, call move_to() and
//               draw_to() repeatedly to describe the path, then call
//               create() to create a GeomNode which will render the
//               described path.
////////////////////////////////////////////////////////////////////
LineSegs::
LineSegs(const string &name) : Namable(name) {
  _color.set(1.0f, 1.0f, 1.0f, 1.0f);
  _thick = 1.0f;
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::Destructor
//       Access: Public
////////////////////////////////////////////////////////////////////
LineSegs::
~LineSegs() {
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::reset
//       Access: Public
//  Description: Removes any lines in progress and resets to the
//               initial empty state.
////////////////////////////////////////////////////////////////////
void LineSegs::
reset() {
  _list.clear();
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::move_to
//       Access: Public
//  Description: Moves the pen to the given point without drawing a
//               line.  When followed by draw_to(), this marks the
//               first point of a line segment; when followed by
//               move_to() or create(), this creates a single point.
////////////////////////////////////////////////////////////////////
void LineSegs::
move_to(const LVecBase3f &v) {
  // We create a new SegmentList with the initial point in it.
  SegmentList segs;
  segs.push_back(Point(v, _color));

  // And add this list to the list of segments.
  _list.push_back(segs);
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::draw_to
//       Access: Public
//  Description: Draws a line segment from the pen's last position
//               (the last call to move_to or draw_to) to the
//               indicated point.  move_to() and draw_to() only update
//               tables; the actual drawing is performed when create()
//               is called.
////////////////////////////////////////////////////////////////////
void LineSegs::
draw_to(const LVecBase3f &v) {
  if (_list.empty()) {
    // Let our first call to draw_to() be an implicit move_to().
    move_to(v);

  } else {
    // Get the current SegmentList, which was the last one we added to
    // the LineList.
    SegmentList &segs = _list.back();

    // Add the new point.
    segs.push_back(Point(v, _color));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::empty
//       Access: Public
//  Description: Returns true if move_to() or draw_to() have not been
//               called since the last reset() or create(), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool LineSegs::
is_empty() {
  return _list.empty();
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::get_vertex
//       Access: Public
//  Description: Returns the nth point or vertex of the line segment
//               sequence generated by the last call to create().  The
//               first move_to() generates vertex 0; subsequent
//               move_to() and draw_to() calls generate consecutively
//               higher vertex numbers.
////////////////////////////////////////////////////////////////////
Vertexf LineSegs::
get_vertex(int n) const {
  nassertr(_created_data != (GeomVertexData *)NULL, Vertexf::zero());
  GeomVertexReader vertex(_created_data, InternalName::get_vertex());
  vertex.set_row(n);
  return vertex.get_data3f();
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::set_vertex
//       Access: Public
//  Description: Moves the nth point or vertex of the line segment
//               sequence generated by the last call to create().  The
//               first move_to() generates vertex 0; subsequent
//               move_to() and draw_to() calls generate consecutively
//               higher vertex numbers.
////////////////////////////////////////////////////////////////////
void LineSegs::
set_vertex(int n, const Vertexf &vert) {
  nassertv(_created_data != (GeomVertexData *)NULL);
  GeomVertexWriter vertex(_created_data, InternalName::get_vertex());
  vertex.set_row(n);
  vertex.set_data3f(vert);
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::get_vertex_color
//       Access: Public
//  Description: Returns the color of the nth point or vertex.
////////////////////////////////////////////////////////////////////
Colorf LineSegs::
get_vertex_color(int n) const {
  nassertr(_created_data != (GeomVertexData *)NULL, Colorf::zero());
  GeomVertexReader color(_created_data, InternalName::get_color());
  color.set_row(n);
  return color.get_data4f();
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::set_vertex_color
//       Access: Public
//  Description: Changes the vertex color of the nth point or vertex.
//               See set_vertex().
////////////////////////////////////////////////////////////////////
void LineSegs::
set_vertex_color(int n, const Colorf &c) {
  nassertv(_created_data != (GeomVertexData *)NULL);
  GeomVertexWriter color(_created_data, InternalName::get_color());
  color.set_row(n);
  color.set_data4f(c);
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::get_current_position
//       Access: Public
//  Description: Returns the pen's current position.  The next call to
//               draw_to() will draw a line segment from this point.
////////////////////////////////////////////////////////////////////
const Vertexf &LineSegs::
get_current_position() {
  if (_list.empty()) {
    // Our pen isn't anywhere.  We'll put it somewhere.
    move_to(Vertexf(0.0f, 0.0f, 0.0f));
  }

  return _list.back().back()._point;
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::create
//       Access: Public
//  Description: Appends to an existing GeomNode a new Geom that
//               will render the series of line segments and points
//               described via calls to move_to() and draw_to().  The
//               lines and points are created with the color and
//               thickness established by calls to set_color() and
//               set_thick().
//
//               If dynamic is true, the line segments will be created
//               with the dynamic Geom setting, optimizing them for
//               runtime vertex animation.
////////////////////////////////////////////////////////////////////
GeomNode *LineSegs::
create(GeomNode *previous, bool dynamic) {
  if (!_list.empty()) {
    _created_data = NULL;
      
    CPT(RenderAttrib) thick = RenderModeAttrib::make(RenderModeAttrib::M_unchanged, _thick);
    CPT(RenderState) state = RenderState::make(thick);

    PT(GeomVertexData) vdata = new GeomVertexData
      ("lineSegs", GeomVertexFormat::get_v3cp(),
       dynamic ? Geom::UH_dynamic : Geom::UH_static);
    GeomVertexWriter vertex(vdata, InternalName::get_vertex());
    GeomVertexWriter color(vdata, InternalName::get_color());
    
    PT(GeomLinestrips) lines = new GeomLinestrips(Geom::UH_static);
    PT(GeomPoints) points = new GeomPoints(Geom::UH_static);
    
    int v = 0;
    LineList::const_iterator ll;
    SegmentList::const_iterator sl;
    
    for (ll = _list.begin(); ll != _list.end(); ll++) {
      const SegmentList &segs = (*ll);
      
      if (segs.size() < 2) {
        // A segment of length 1 is just a point.
        for (sl = segs.begin(); sl != segs.end(); sl++) {
          points->add_vertex(v);
          vertex.add_data3f((*sl)._point);
          color.add_data4f((*sl)._color);
          v++;
        }
        points->close_primitive();
        
      } else {
        // A segment of length 2 or more is a line segment or
        // segments.
        for (sl = segs.begin(); sl != segs.end(); sl++) {
          lines->add_vertex(v);
          vertex.add_data3f((*sl)._point);
          color.add_data4f((*sl)._color);
          v++;
        }
        lines->close_primitive();
      }
    }

    if (lines->get_num_vertices() != 0) {
      PT(Geom) geom = new Geom;
      geom->set_vertex_data(vdata);
      geom->add_primitive(lines);
      previous->add_geom(geom, state);
    }
    if (points->get_num_vertices() != 0) {
      PT(Geom) geom = new Geom;
      geom->set_vertex_data(vdata);
      geom->add_primitive(points);
      previous->add_geom(geom, state);
    }
    
    // And reset for next time.
    reset();
  }

  return previous;
}
