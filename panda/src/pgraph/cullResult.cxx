// Filename: cullResult.cxx
// Created by:  drose (28Feb02)
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

#include "cullResult.h"
#include "cullBinManager.h"
#include "cullBinAttrib.h"
#include "textureAttrib.h"
#include "lightAttrib.h"
#include "colorAttrib.h"
#include "alphaTestAttrib.h"
#include "depthWriteAttrib.h"
#include "colorScaleAttrib.h"
#include "fogAttrib.h"
#include "transparencyAttrib.h"
#include "renderState.h"
#include "clockObject.h"
#include "config_pgraph.h"

// This value is used instead of 1.0 to represent the alpha level of a
// pixel that is to be considered "opaque" for the purposes of M_dual.

// Ideally, 1.0 is the only correct value for this.  Realistically, we
// have to fudge it lower for two reasons:

// (1) The modelers tend to paint textures with very slight
// transparency levels in places that are not intended to be
// transparent, without realizing it.  These very faint transparency
// regions are normally (almost) invisible, but when rendered with
// M_dual they may be revealed as regions of poor alpha sorting.

// (2) There seems to be some problem in DX where, in certain
// circumstances apparently related to automatic texture management,
// it spontaneously drops out the bottom two bits of an eight-bit
// alpha channel, causing a value of 255 to become a value of 252
// instead.

// We use 256 as the denominator here (instead of, say, 255) because a
// fractional power of two will have a terminating representation in
// base 2, and thus will be more likely to have a precise value in
// whatever internal representation the graphics API will use.
static const float dual_opaque_level = 252.0f / 256.0f;
static const double bin_color_flash_rate = 1.0;  // 1 state change per second

////////////////////////////////////////////////////////////////////
//     Function: CullResult::make_next
//       Access: Public
//  Description: Returns a newly-allocated CullResult object that
//               contains a copy of just the subset of the data from
//               this CullResult object that is worth keeping around
//               for next frame.
////////////////////////////////////////////////////////////////////
PT(CullResult) CullResult::
make_next() const {
  PT(CullResult) new_result = new CullResult(_gsg, _draw_region_pcollector);
  new_result->_bins.reserve(_bins.size());

  CullBinManager *bin_manager = CullBinManager::get_global_ptr();

  for (size_t i = 0; i < _bins.size(); ++i) {
    CullBin *old_bin = _bins[i];
    if (old_bin == (CullBin *)NULL || 
        old_bin->get_bin_type() != bin_manager->get_bin_type(i)) {
      new_result->_bins.push_back((CullBin *)NULL);
    } else {
      new_result->_bins.push_back(old_bin->make_next());
    }
  }

  return new_result;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::add_object
//       Access: Public
//  Description: Adds the indicated CullableObject to the appropriate
//               bin.  The bin becomes the owner of the object
//               pointer, and will eventually delete it.
////////////////////////////////////////////////////////////////////
void CullResult::
add_object(CullableObject *object, const CullTraverser *traverser) {
  static const Colorf flash_alpha_color(0.92, 0.96, 0.10, 1.0f);
  static const Colorf flash_binary_color(0.21f, 0.67f, 0.24f, 1.0f);
  static const Colorf flash_multisample_color(0.78f, 0.05f, 0.81f, 1.0f);
  static const Colorf flash_dual_color(0.92f, 0.01f, 0.01f, 1.0f);

  bool force = !allow_incomplete_render;
  Thread *current_thread = traverser->get_current_thread();

  // Check to see if there's a special transparency setting.
  const RenderState *state = object->_state;
  nassertv(state != (const RenderState *)NULL);

  const TransparencyAttrib *trans = state->get_transparency();
  if (trans != (const TransparencyAttrib *)NULL) {
    switch (trans->get_mode()) {
    case TransparencyAttrib::M_alpha:
      // M_alpha implies an alpha-write test, so we don't waste time
      // writing 0-valued pixels.
      object->_state = state->compose(get_alpha_state());
#ifndef NDEBUG
      check_flash_transparency(object->_state, flash_alpha_color);
#endif
      break;

    case TransparencyAttrib::M_binary:
      // M_binary is implemented by explicitly setting the alpha test.
      object->_state = state->compose(get_binary_state());
#ifndef NDEBUG
      check_flash_transparency(object->_state, flash_binary_color);
#endif
      break;

    case TransparencyAttrib::M_multisample:
    case TransparencyAttrib::M_multisample_mask:
      // The multisample modes are implemented using M_binary if the
      // GSG in use doesn't support multisample.
      if (!_gsg->get_supports_multisample()) {
        object->_state = state->compose(get_binary_state());
      }
#ifndef NDEBUG
      check_flash_transparency(object->_state, flash_multisample_color);
#endif
      break;

    case TransparencyAttrib::M_dual:
#ifndef NDEBUG
      check_flash_transparency(object->_state, flash_dual_color);
      state = object->_state;
#endif
      if (!m_dual) {
        // If m_dual is configured off, it becomes M_alpha.
        break;
      }

      // M_dual is implemented by drawing the opaque parts first,
      // without transparency, then drawing the transparent parts
      // later.  This means we must copy the object and add it to
      // both bins.  We can only do this if we do not have an
      // explicit bin already applied; otherwise, M_dual falls back
      // to M_alpha. 
      {
        const CullBinAttrib *bin_attrib = state->get_bin();
        if (bin_attrib == (CullBinAttrib *)NULL || 
            bin_attrib->get_bin_name().empty()) {
          // We make a copy of the object to draw the transparent part
          // without decals; this gets placed in the transparent bin.
#ifndef NDEBUG
          if (m_dual_transparent) 
#endif
            {
              CullableObject *transparent_part = new CullableObject(*object);
              CPT(RenderState) transparent_state = object->has_decals() ? 
                get_dual_transparent_state_decals() : 
                get_dual_transparent_state();
              transparent_part->_state = state->compose(transparent_state);
              if (transparent_part->munge_geom
                  (_gsg, _gsg->get_geom_munger(transparent_part->_state, current_thread),
                   traverser, force)) {
                CullBin *bin = get_bin(transparent_part->_state->get_bin_index());
                nassertv(bin != (CullBin *)NULL);
#ifndef NDEBUG
                check_flash_bin(transparent_part->_state, bin);
#endif
                if (force || transparent_part->request_resident()) {
                  bin->add_object(transparent_part, current_thread);
                }
              }
            }
          
          // Now we can draw the opaque part, with decals.  This will
          // end up in the opaque bin.
          object->_state = state->compose(get_dual_opaque_state());
#ifndef NDEBUG
          if (!m_dual_opaque) {
            delete object;
            return;
          }
#endif
        }
        // The object is assigned to a specific bin; M_dual becomes
        // M_alpha.
      }
      break;
      
    default:
      // Other kinds of transparency need no special handling.
      break;
    }
  }

  CullBin *bin = get_bin(object->_state->get_bin_index());
  nassertv(bin != (CullBin *)NULL);

#ifndef NDEBUG
  check_flash_bin(object->_state, bin);
#endif

  // Munge vertices as needed for the GSG's requirements, and the
  // object's current state.
  if (object->munge_geom(_gsg, _gsg->get_geom_munger(object->_state, current_thread), traverser, force)) {
    if (force || object->request_resident()) {
      bin->add_object(object, current_thread);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::finish_cull
//       Access: Public
//  Description: Called after all the geoms have been added, this
//               indicates that the cull process is finished for this
//               frame and gives the bins a chance to do any
//               post-processing (like sorting) before moving on to
//               draw.
////////////////////////////////////////////////////////////////////
void CullResult::
finish_cull(SceneSetup *scene_setup, Thread *current_thread) {
  CullBinManager *bin_manager = CullBinManager::get_global_ptr();

  for (size_t i = 0; i < _bins.size(); ++i) {
    if (!bin_manager->get_bin_active(i)) {
      // If the bin isn't active, don't sort it, and don't draw it.
      // In fact, clear it.
      _bins[i] = NULL;

    } else {
      CullBin *bin = _bins[i];
      if (bin != (CullBin *)NULL) {
        bin->finish_cull(scene_setup, current_thread);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::draw
//       Access: Public
//  Description: Asks all the bins to draw themselves in the correct
//               order.
////////////////////////////////////////////////////////////////////
void CullResult::
draw(Thread *current_thread) {
  // Ask the bin manager for the correct order to draw all the bins.
  CullBinManager *bin_manager = CullBinManager::get_global_ptr();
  int num_bins = bin_manager->get_num_bins();
  for (int i = 0; i < num_bins; i++) {
    int bin_index = bin_manager->get_bin(i);
    nassertv(bin_index >= 0);

    if (bin_index < (int)_bins.size() && _bins[bin_index] != (CullBin *)NULL) {
      _bins[bin_index]->draw(current_thread);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::bin_removed
//       Access: Public, Static
//  Description: Intended to be called by
//               CullBinManager::remove_bin(), this informs all the
//               CullResults in the world to remove the indicated
//               bin_index from their cache if it has been cached.
////////////////////////////////////////////////////////////////////
void CullResult::
bin_removed(int bin_index) {
  // Do something here.
  nassertv(false);
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::make_new_bin
//       Access: Private
//  Description: Allocates a new CullBin for the given bin_index and
//               stores it for next time.
////////////////////////////////////////////////////////////////////
CullBin *CullResult::
make_new_bin(int bin_index) {
  CullBinManager *bin_manager = CullBinManager::get_global_ptr();
  PT(CullBin) bin = bin_manager->make_new_bin(bin_index, _gsg,
                                              _draw_region_pcollector);
  if (bin != (CullBin *)NULL) {
    // Now store it in the vector.
    while (bin_index >= (int)_bins.size()) {
      _bins.push_back((CullBin *)NULL);
    }
    nassertr(bin_index >= 0 && bin_index < (int)_bins.size(), NULL);
    _bins[bin_index] = bin;
  }

  return bin;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::get_alpha_state
//       Access: Private
//  Description: Returns a RenderState that changes the alpha test to
//               > 0, for implementing M_alpha.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullResult::
get_alpha_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    // We don't monkey with the priority, since we want to allow the
    // user to override this if he desires.
    state = RenderState::make(AlphaTestAttrib::make(AlphaTestAttrib::M_greater, 0.0f));
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::get_binary_state
//       Access: Private
//  Description: Returns a RenderState that applies the effects of
//               M_binary.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullResult::
get_binary_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make(AlphaTestAttrib::make(AlphaTestAttrib::M_greater_equal, 0.5f),
                              TransparencyAttrib::make(TransparencyAttrib::M_none),
                              RenderState::get_max_priority());
  }
  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::check_flash_bin
//       Access: Private
//  Description: If the user configured flash-bin-binname, then update
//               the object's state to flash all the geometry in the
//               bin.
////////////////////////////////////////////////////////////////////
void CullResult::
check_flash_bin(CPT(RenderState) &state, CullBin *bin) {
  if (bin->has_flash_color()) {
    int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * bin_color_flash_rate);
    if ((cycle & 1) == 0) {
      state = state->remove_attrib(TextureAttrib::get_class_type());
      state = state->remove_attrib(LightAttrib::get_class_type());
      state = state->remove_attrib(ColorScaleAttrib::get_class_type());
      state = state->remove_attrib(FogAttrib::get_class_type());
      state = state->add_attrib(ColorAttrib::make_flat(bin->get_flash_color()));
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::check_flash_transparency
//       Access: Private
//  Description: If the user configured show-transparency, then
//               update the object's state to flash the current
//               geometry with the specified color.
////////////////////////////////////////////////////////////////////
void CullResult::
check_flash_transparency(CPT(RenderState) &state, const Colorf &transparency) {
  if (show_transparency) {
    int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * bin_color_flash_rate);
    if ((cycle & 1) == 0) {
      state = state->remove_attrib(TextureAttrib::get_class_type());
      state = state->remove_attrib(LightAttrib::get_class_type());
      state = state->remove_attrib(ColorScaleAttrib::get_class_type());
      state = state->remove_attrib(FogAttrib::get_class_type());
      state = state->add_attrib(ColorAttrib::make_flat(transparency));
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::get_dual_transparent_state
//       Access: Private
//  Description: Returns a RenderState that renders only the
//               transparent parts of an object, in support of M_dual.
//               This state is suitable only for objects that do not
//               contain decals.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullResult::
get_dual_transparent_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    // The alpha test for > 0 prevents us from drawing empty pixels,
    // and hence filling up the depth buffer with large empty spaces
    // that may obscure other things.  However, this does mean we draw
    // pixels twice where the alpha == 1.0 (since they were already
    // drawn in the opaque pass).  This is not normally a problem,
    // except when we are using decals; in the case of decals, we
    // don't want to draw the 1.0 pixels again, since these are the
    // ones that may have been decaled onto.
    state = RenderState::make(AlphaTestAttrib::make(AlphaTestAttrib::M_greater, 0.0f),
                              TransparencyAttrib::make(TransparencyAttrib::M_alpha),
                              DepthWriteAttrib::make(DepthWriteAttrib::M_off),
                              RenderState::get_max_priority());
  }

#ifndef NDEBUG
  if (m_dual_flash) {
    int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * bin_color_flash_rate);
    if ((cycle & 1) == 0) {
      static CPT(RenderState) flash_state = NULL;
      if (flash_state == (const RenderState *)NULL) {
        flash_state = state->add_attrib(ColorScaleAttrib::make(LVecBase4f(0.8f, 0.2f, 0.2f, 1.0f)));
        flash_state = flash_state->add_attrib(AlphaTestAttrib::make(AlphaTestAttrib::M_less, 1.0f));
      }
      return flash_state;
    }
  }
#endif  // NDEBUG

  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::get_dual_transparent_state_decals
//       Access: Private
//  Description: Returns a RenderState that renders only the
//               transparent parts of an object, but suitable for
//               objects that contain decals.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullResult::
get_dual_transparent_state_decals() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    // This is exactly the same as above except here we make the alpha
    // test of < 1.0 instead of > 0.0.  This makes us draw big empty
    // pixels where the alpha values are 0.0, but we don't overwrite
    // the decals where the pixels are 1.0.
    state = RenderState::make(AlphaTestAttrib::make(AlphaTestAttrib::M_less, dual_opaque_level),
                              TransparencyAttrib::make(TransparencyAttrib::M_alpha),
                              DepthWriteAttrib::make(DepthWriteAttrib::M_off),
                              RenderState::get_max_priority());
  }

#ifndef NDEBUG
  if (m_dual_flash) {
    int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * bin_color_flash_rate);
    if ((cycle & 1) == 0) {
      static CPT(RenderState) flash_state = NULL;
      if (flash_state == (const RenderState *)NULL) {
        flash_state = state->add_attrib(ColorScaleAttrib::make(LVecBase4f(0.8f, 0.2f, 0.2f, 1.0f)));
      }
      return flash_state;
    }
  }
#endif  // NDEBUG

  return state;
}

////////////////////////////////////////////////////////////////////
//     Function: CullResult::get_dual_opaque_state
//       Access: Private
//  Description: Returns a RenderState that renders only the
//               opaque parts of an object, in support of M_dual.
////////////////////////////////////////////////////////////////////
CPT(RenderState) CullResult::
get_dual_opaque_state() {
  static CPT(RenderState) state = NULL;
  if (state == (const RenderState *)NULL) {
    state = RenderState::make(AlphaTestAttrib::make(AlphaTestAttrib::M_greater_equal, dual_opaque_level),
                              TransparencyAttrib::make(TransparencyAttrib::M_none),
                              RenderState::get_max_priority());
  }

#ifndef NDEBUG
  if (m_dual_flash) {
    int cycle = (int)(ClockObject::get_global_clock()->get_frame_time() * bin_color_flash_rate);
    if ((cycle & 1) == 0) {
      static CPT(RenderState) flash_state = NULL;
      if (flash_state == (const RenderState *)NULL) {
        flash_state = state->add_attrib(ColorScaleAttrib::make(LVecBase4f(0.2f, 0.2f, 0.8f, 1.0f)));
      }
      return flash_state;
    }
  }
#endif  // NDEBUG

  return state;
}

