// Filename: graphicsPipe.h
// Created by:  mike (09Jan97)
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

#ifndef GRAPHICSPIPE_H
#define GRAPHICSPIPE_H

#include "pandabase.h"

#include "graphicsDevice.h"
#include "typedReferenceCount.h"
#include "pointerTo.h"
#include "pmutex.h"

class HardwareChannel;
class GraphicsOutput;
class GraphicsWindow;
class GraphicsBuffer;
class GraphicsStateGuardian;
class FrameBufferProperties;
class Texture;

////////////////////////////////////////////////////////////////////
//       Class : GraphicsPipe
// Description : An object to create GraphicsOutputs that share a
//               particular 3-D API.  Normally, there will only be one
//               GraphicsPipe in an application, although it is
//               possible to have multiple of these at once if there
//               are multiple different API's available in the same
//               machine.
//
//               Often, the GraphicsPipe corresponds to a physical
//               output device, hence the term "pipe", but this is not
//               necessarily the case.
//
//               The GraphicsPipe is used by the GraphicsEngine object
//               to create and destroy windows; it keeps ownership of
//               the windows it creates.
//
//               M. Asad added new/interim functionality where GraphicsPipe
//               now contains a device interface to directx/opengl which
//               will be used to handle multiple windows from same device.
//
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GraphicsPipe : public TypedReferenceCount {
protected:
  GraphicsPipe();
private:
  GraphicsPipe(const GraphicsPipe &copy);
  void operator = (const GraphicsPipe &copy);

PUBLISHED:
  virtual ~GraphicsPipe();

  enum OutputTypes {
    OT_window            = 0x0001,
    OT_fullscreen_window = 0x0002,
    OT_buffer            = 0x0004,
    OT_texture_buffer    = 0x0008,
  };

  enum BufferCreationFlags {

    // How many RGBA aux bitplanes do you need?
    BF_need_aux_rgba_MASK  = 0x00000003,
    BF_need_0_aux_rgba     = 0x00000000,
    BF_need_1_aux_rgba     = 0x00000001,
    BF_need_2_aux_rgba     = 0x00000002,
    BF_need_3_aux_rgba     = 0x00000003,

    // How many half-float rgba aux bitplanes do you need?
    // This is not currently implemented.
    BF_need_aux_hrgba_MASK = 0x0000000C,
    BF_need_0_aux_hrgba    = 0x00000000,
    BF_need_1_aux_hrgba    = 0x00000004,
    BF_need_2_aux_hrgba    = 0x00000008,
    BF_need_3_aux_hrgba    = 0x0000000C,

    // How many full-float single-channel bitplanes do you need?
    // This is not currently implemented.
    BF_need_aux_float_MASK = 0x00000030,
    BF_need_0_aux_float    = 0x00000000,
    BF_need_1_aux_float    = 0x00000010,
    BF_need_2_aux_float    = 0x00000020,
    BF_need_3_aux_float    = 0x00000040,

    // Flags that control what type of output is returned.
    BF_refuse_parasite     = 0x00000100,
    BF_require_parasite    = 0x00000200,
    BF_refuse_window       = 0x00000400,
    BF_require_window      = 0x00000800,

    // Miscellaneous control flags.
    BF_can_bind_color      = 0x00010000, // Need capability: bind the color bitplane to a tex.
    BF_can_bind_every      = 0x00020000, // Need capability: bind all bitplanes to a tex.
    BF_size_track_host     = 0x00040000, // Buffer should track the host size.
    BF_no_new_gsg          = 0x00080000, // Do not create a new gsg, no matter what.
  };

  INLINE bool is_valid() const;
  INLINE int get_supported_types() const;
  INLINE bool supports_type(int flags) const;

  INLINE int get_display_width() const;
  INLINE int get_display_height() const;

  virtual string get_interface_name() const=0;

public:
  virtual int get_num_hw_channels();
  virtual HardwareChannel *get_hw_channel(GraphicsOutput *window, int index);

  INLINE GraphicsDevice *get_device() const;
  virtual PT(GraphicsDevice) make_device(void *scrn = NULL);

protected:
  // The make_output() and make_gsg() interfaces on GraphicsPipe are
  // protected; don't try to call them directly.  Instead, use
  // the interface on GraphicsEngine to make a new window or buffer.
  virtual PT(GraphicsStateGuardian) make_gsg(const FrameBufferProperties &properties,
                                             GraphicsStateGuardian *share_with);
  virtual void close_gsg(GraphicsStateGuardian *gsg);
  
  virtual PT(GraphicsOutput) make_output(const string &name,
                                         int x_size, int y_size, int flags,
                                         GraphicsStateGuardian *gsg,
                                         GraphicsOutput *host,
                                         int retry,
                                         bool precertify);
  
  Mutex _lock;

  bool _is_valid;
  int _supported_types;
  int _display_width;
  int _display_height;
  PT(GraphicsDevice) _device;

  static const int strip_properties[];

public:

  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "GraphicsPipe",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
  friend class GraphicsEngine;
};

#include "graphicsPipe.I"

#endif /* GRAPHICSPIPE_H */
