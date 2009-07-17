// Filename: tinyXGraphicsWindow.cxx
// Created by:  drose (03May08)
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

#include "pandabase.h"

#ifdef HAVE_X11

#include "tinyXGraphicsWindow.h"
#include "tinyGraphicsStateGuardian.h"
#include "tinyXGraphicsPipe.h"
#include "config_tinydisplay.h"

#include "graphicsPipe.h"
#include "keyboardButton.h"
#include "mouseButton.h"
#include "clockObject.h"
#include "pStatTimer.h"
#include "textEncoder.h"
#include "throw_event.h"
#include "lightReMutexHolder.h"

TypeHandle TinyXGraphicsWindow::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
TinyXGraphicsWindow::
TinyXGraphicsWindow(GraphicsEngine *engine, GraphicsPipe *pipe, 
                    const string &name,
                    const FrameBufferProperties &fb_prop,
                    const WindowProperties &win_prop,
                    int flags,
                    GraphicsStateGuardian *gsg,
                    GraphicsOutput *host) :
  x11GraphicsWindow(engine, pipe, name, fb_prop, win_prop, flags, gsg, host)
{
  _gc = (GC)NULL;

  _reduced_frame_buffer = NULL;
  _full_frame_buffer = NULL;
  _ximage = NULL;
  update_pixel_factor();
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::Destructor
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
TinyXGraphicsWindow::
~TinyXGraphicsWindow() {
  if (_gc != NULL && _display != NULL) {
    XFreeGC(_display, _gc);
  }
  if (_ximage != NULL) {
    XDestroyImage(_ximage);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::begin_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               before beginning rendering for a given frame.  It
//               should do whatever setup is required, and return true
//               if the frame should be rendered, or false if it
//               should be skipped.
////////////////////////////////////////////////////////////////////
bool TinyXGraphicsWindow::
begin_frame(FrameMode mode, Thread *current_thread) {
  PStatTimer timer(_make_current_pcollector, current_thread);

  if (_xwindow == (Window)NULL) {
    return false;
  }

  begin_frame_spam(mode);
  if (_gsg == (GraphicsStateGuardian *)NULL) {
    return false;
  }
  if (_awaiting_configure) {
    // Don't attempt to draw while we have just reconfigured the
    // window and we haven't got the notification back yet.
    return false;
  }

  TinyGraphicsStateGuardian *tinygsg;
  DCAST_INTO_R(tinygsg, _gsg, false);

  if (_reduced_frame_buffer != (ZBuffer *)NULL) {
    tinygsg->_current_frame_buffer = _reduced_frame_buffer;
  } else {
    tinygsg->_current_frame_buffer = _full_frame_buffer;
  }
  tinygsg->reset_if_new();
  
  _gsg->set_current_properties(&get_fb_properties());
  return _gsg->begin_frame(current_thread);
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::end_frame
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after rendering is completed for a given frame.  It
//               should do whatever finalization is required.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
end_frame(FrameMode mode, Thread *current_thread) {
  end_frame_spam(mode);
  nassertv(_gsg != (GraphicsStateGuardian *)NULL);

  if (mode == FM_render) {
    // end_render_texture();
    copy_to_textures();
  }

  _gsg->end_frame(current_thread);

  if (mode == FM_render) {
    trigger_flip();
    if (_one_shot) {
      prepare_for_deletion();
    }
    clear_cube_map_selection();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::begin_flip
//       Access: Public, Virtual
//  Description: This function will be called within the draw thread
//               after end_frame() has been called on all windows, to
//               initiate the exchange of the front and back buffers.
//
//               This should instruct the window to prepare for the
//               flip at the next video sync, but it should not wait.
//
//               We have the two separate functions, begin_flip() and
//               end_flip(), to make it easier to flip all of the
//               windows at the same time.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
begin_flip() {
  if (_xwindow == (Window)NULL) {
    return;
  }

  if (_reduced_frame_buffer != (ZBuffer *)NULL) {
    // Zoom the reduced buffer onto the full buffer.
    ZB_zoomFrameBuffer(_full_frame_buffer, 0, 0, 
                       _full_frame_buffer->xsize, _full_frame_buffer->ysize,
                       _reduced_frame_buffer, 0, 0,
                       _reduced_frame_buffer->xsize, _reduced_frame_buffer->ysize);
  }

  // We can't just point the XPutImage directly at our own framebuffer
  // data, even if the bytes_per_pixel matches, because some X
  // displays will respect the alpha channel and make the window
  // transparent there.  We don't want transparent windows where the
  // alpha data happens to less than 1.0.
  ZB_copyFrameBufferNoAlpha(_full_frame_buffer, _ximage->data, _pitch);

  XPutImage(_display, _xwindow, _gc, _ximage, 0, 0, 0, 0,
            _full_frame_buffer->xsize, _full_frame_buffer->ysize);
  XFlush(_display);
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::supports_pixel_zoom
//       Access: Published, Virtual
//  Description: Returns true if a call to set_pixel_zoom() will be
//               respected, false if it will be ignored.  If this
//               returns false, then get_pixel_factor() will always
//               return 1.0, regardless of what value you specify for
//               set_pixel_zoom().
//
//               This may return false if the underlying renderer
//               doesn't support pixel zooming, or if you have called
//               this on a DisplayRegion that doesn't have both
//               set_clear_color() and set_clear_depth() enabled.
////////////////////////////////////////////////////////////////////
bool TinyXGraphicsWindow::
supports_pixel_zoom() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::process_events
//       Access: Public, Virtual
//  Description: Do whatever processing is necessary to ensure that
//               the window responds to user events.  Also, honor any
//               requests recently made via request_properties()
//
//               This function is called only within the window
//               thread.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
process_events() {
  LightReMutexHolder holder(TinyXGraphicsPipe::_x_mutex);

  GraphicsWindow::process_events();

  if (_xwindow == (Window)0) {
    return;
  }
  
  poll_raw_mice();
  
  XEvent event;
  XKeyEvent keyrelease_event;
  bool got_keyrelease_event = false;

  while (XCheckIfEvent(_display, &event, check_event, (char *)this)) {
    if (XFilterEvent(&event, None)) {
      continue;
    }

    if (got_keyrelease_event) {
      // If a keyrelease event is immediately followed by a matching
      // keypress event, that's just key repeat and we should treat
      // the two events accordingly.  It would be nice if X provided a
      // way to differentiate between keyrepeat and explicit
      // keypresses more generally.
      got_keyrelease_event = false;

      if (event.type == KeyPress &&
          event.xkey.keycode == keyrelease_event.keycode &&
          (event.xkey.time - keyrelease_event.time <= 1)) {
        // In particular, we only generate down messages for the
        // repeated keys, not down-and-up messages.
        handle_keystroke(event.xkey);

        // We thought about not generating the keypress event, but we
        // need that repeat for backspace.  Rethink later.
        handle_keypress(event.xkey);
        continue;

      } else {
        // This keyrelease event is not immediately followed by a
        // matching keypress event, so it's a genuine release.
        handle_keyrelease(keyrelease_event);
      }
    }

    WindowProperties properties;
    ButtonHandle button;

    switch (event.type) {
    case ReparentNotify:
      break;

    case ConfigureNotify:
      _awaiting_configure = false;
      if (_properties.get_fixed_size()) {
        // If the window properties indicate a fixed size only, undo
        // any attempt by the user to change them.  In X, there
        // doesn't appear to be a way to universally disallow this
        // directly (although we do set the min_size and max_size to
        // the same value, which seems to work for most window
        // managers.)
        WindowProperties current_props = get_properties();
        if (event.xconfigure.width != current_props.get_x_size() ||
            event.xconfigure.height != current_props.get_y_size()) {
          XWindowChanges changes;
          changes.width = current_props.get_x_size();
          changes.height = current_props.get_y_size();
          int value_mask = (CWWidth | CWHeight);
          XConfigureWindow(_display, _xwindow, value_mask, &changes);
        }

      } else {
        // A normal window may be resized by the user at will.
        properties.set_size(event.xconfigure.width, event.xconfigure.height);
        system_changed_properties(properties);
        ZB_resize(_full_frame_buffer, NULL, _properties.get_x_size(), _properties.get_y_size());
        _pitch = (_full_frame_buffer->xsize * _bytes_per_pixel + 3) & ~3;
        create_reduced_frame_buffer();
        create_ximage();
      }
      break;

    case ButtonPress:
      // This refers to the mouse buttons.
      button = get_mouse_button(event.xbutton);
      _input_devices[0].set_pointer_in_window(event.xbutton.x, event.xbutton.y);
      _input_devices[0].button_down(button);
      break;
      
    case ButtonRelease:
      button = get_mouse_button(event.xbutton);
      _input_devices[0].set_pointer_in_window(event.xbutton.x, event.xbutton.y);
      _input_devices[0].button_up(button);
      break;

    case MotionNotify:
      _input_devices[0].set_pointer_in_window(event.xmotion.x, event.xmotion.y);
      break;

    case KeyPress:
      handle_keystroke(event.xkey);
      handle_keypress(event.xkey);
      break;

    case KeyRelease:
      // The KeyRelease can't be processed immediately, because we
      // have to check first if it's immediately followed by a
      // matching KeyPress event.
      keyrelease_event = event.xkey;
      got_keyrelease_event = true;
      break;

    case EnterNotify:
      _input_devices[0].set_pointer_in_window(event.xcrossing.x, event.xcrossing.y);
      break;

    case LeaveNotify:
      _input_devices[0].set_pointer_out_of_window();
      break;

    case FocusIn:
      properties.set_foreground(true);
      system_changed_properties(properties);
      break;

    case FocusOut:
      properties.set_foreground(false);
      system_changed_properties(properties);
      break;

    case UnmapNotify:
      properties.set_minimized(true);
      system_changed_properties(properties);
      break;

    case MapNotify:
      properties.set_minimized(false);
      system_changed_properties(properties);

      // Auto-focus the window when it is mapped.
      XSetInputFocus(_display, _xwindow, RevertToPointerRoot, CurrentTime);
      break;

    case ClientMessage:
      if ((Atom)(event.xclient.data.l[0]) == _wm_delete_window) {
        // This is a message from the window manager indicating that
        // the user has requested to close the window.
        string close_request_event = get_close_request_event();
        if (!close_request_event.empty()) {
          // In this case, the app has indicated a desire to intercept
          // the request and process it directly.
          throw_event(close_request_event);

        } else {
          // In this case, the default case, the app does not intend
          // to service the request, so we do by closing the window.

          // TODO: don't release the gsg in the window thread.
          close_window();
          properties.set_open(false);
          system_changed_properties(properties);
        }
      }
      break;

    case DestroyNotify:
      // Apparently, we never get a DestroyNotify on a toplevel
      // window.  Instead, we rely on hints from the window manager
      // (see above).
      tinydisplay_cat.info()
        << "DestroyNotify\n";
      break;

    default:
      tinydisplay_cat.error()
        << "unhandled X event type " << event.type << "\n";
    }
  }

  if (got_keyrelease_event) {
    // This keyrelease event is not immediately followed by a
    // matching keypress event, so it's a genuine release.
    handle_keyrelease(keyrelease_event);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::close_window
//       Access: Protected, Virtual
//  Description: Closes the window right now.  Called from the window
//               thread.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
close_window() {
  if (_gsg != (GraphicsStateGuardian *)NULL) {
    TinyGraphicsStateGuardian *tinygsg;
    DCAST_INTO_V(tinygsg, _gsg);
    tinygsg->_current_frame_buffer = NULL;
    _gsg.clear();
    _active = false;
  }
  
  x11GraphicsWindow::close_window();
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::open_window
//       Access: Protected, Virtual
//  Description: Opens the window right now.  Called from the window
//               thread.  Returns true if the window is successfully
//               opened, or false if there was a problem.
////////////////////////////////////////////////////////////////////
bool TinyXGraphicsWindow::
open_window() {
  TinyXGraphicsPipe *tinyx_pipe;
  DCAST_INTO_R(tinyx_pipe, _pipe, false);

  // GSG Creation/Initialization
  TinyGraphicsStateGuardian *tinygsg;
  if (_gsg == 0) {
    // There is no old gsg.  Create a new one.
    tinygsg = new TinyGraphicsStateGuardian(_engine, _pipe, NULL);
    _gsg = tinygsg;
  } else {
    DCAST_INTO_R(tinygsg, _gsg, false);
  }

  XVisualInfo vinfo_template;
  vinfo_template.screen = _screen;
  vinfo_template.depth = 32;
  vinfo_template.c_class = TrueColor;

  // Try to get each of these properties in turn.
  int try_masks[] = {
    VisualScreenMask | VisualDepthMask | VisualClassMask,
    VisualScreenMask | VisualClassMask,
    VisualScreenMask | VisualDepthMask,
    VisualScreenMask,
    0,
  };

  int i = 0;
  int num_vinfos = 0;
  XVisualInfo *vinfo_array;
  while (try_masks[i] != 0 && num_vinfos == 0) {
    vinfo_array = 
      XGetVisualInfo(_display, try_masks[i], &vinfo_template, &num_vinfos);
    ++i;
  }

  if (num_vinfos == 0) {
    // No suitable X visual.
    tinydisplay_cat.error()
      << "No suitable X Visual available; cannot open window.\n";
    return false;
  }
  XVisualInfo *visual_info = &vinfo_array[0];

  _visual = visual_info->visual;
  _depth = visual_info->depth;
  _bytes_per_pixel = _depth / 8;
  if (_bytes_per_pixel == 3) {
    // Seems to be a special case.
    _bytes_per_pixel = 4;
  }
  tinydisplay_cat.info()
    << "Got X Visual with depth " << _depth << " (bpp " << _bytes_per_pixel << ") and class ";
  switch (visual_info->c_class) {
  case TrueColor:
    tinydisplay_cat.info(false) << "TrueColor\n";
    break;
      
  case DirectColor:
    tinydisplay_cat.info(false) << "DirectColor\n";
    break;

  case StaticColor:
    tinydisplay_cat.info(false) << "StaticColor\n";
    break;

  case StaticGray:
    tinydisplay_cat.info(false) << "StaticGray\n";
    break;

  case GrayScale:
    tinydisplay_cat.info(false) << "GrayScale\n";
    break;

  case PseudoColor:
    tinydisplay_cat.info(false) << "PseudoColor\n";
    break;
  }

  if (!_properties.has_origin()) {
    _properties.set_origin(0, 0);
  }
  if (!_properties.has_size()) {
    _properties.set_size(100, 100);
  }

  Window root_window;
  if (!_properties.has_parent_window()) {
    root_window = tinyx_pipe->get_root();
  } else {
    root_window = (Window) (int) _properties.get_parent_window();
  }
  setup_colormap(visual_info);

  _event_mask =
    ButtonPressMask | ButtonReleaseMask |
    KeyPressMask | KeyReleaseMask |
    EnterWindowMask | LeaveWindowMask |
    PointerMotionMask |
    FocusChangeMask |
    StructureNotifyMask;

  // Initialize window attributes
  XSetWindowAttributes wa;
  wa.background_pixel = XBlackPixel(_display, _screen);
  wa.border_pixel = 0;
  wa.colormap = _colormap;
  wa.event_mask = _event_mask;

  unsigned long attrib_mask = 
    CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  _xwindow = XCreateWindow
    (_display, root_window,
     _properties.get_x_origin(), _properties.get_y_origin(),
     _properties.get_x_size(), _properties.get_y_size(),
     0, _depth, InputOutput, _visual, attrib_mask, &wa);

  if (_xwindow == (Window)0) {
    tinydisplay_cat.error()
      << "failed to create X window.\n";
    return false;
  }
  set_wm_properties(_properties, false);

  // We don't specify any fancy properties of the XIC.  It would be
  // nicer if we could support fancy IM's that want preedit callbacks,
  // etc., but that can wait until we have an X server that actually
  // supports these to test it on.
  XIM im = tinyx_pipe->get_im();
  _ic = NULL;
  if (im) {
    _ic = XCreateIC
      (im,
       XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
       NULL);
    if (_ic == (XIC)NULL) {
      tinydisplay_cat.warning()
        << "Couldn't create input context.\n";
    }
  }

  if (_properties.get_cursor_hidden()) {
    XDefineCursor(_display, _xwindow, tinyx_pipe->get_hidden_cursor());
  }

  _gc = XCreateGC(_display, _xwindow, 0, NULL);

  create_full_frame_buffer();
  if (_full_frame_buffer == NULL) {
    tinydisplay_cat.error()
      << "Could not create frame buffer.\n";
    return false;
  }
  create_reduced_frame_buffer();
  create_ximage();
  nassertr(_ximage != NULL, false);

  tinygsg->_current_frame_buffer = _full_frame_buffer;
  
  tinygsg->reset_if_new();
  if (!tinygsg->is_valid()) {
    close_window();
    return false;
  }
  
  XMapWindow(_display, _xwindow);

  if (_properties.get_raw_mice()) {
    open_raw_mice();
  } else {
    if (tinydisplay_cat.is_debug()) {
      tinydisplay_cat.debug()
        << "Raw mice not requested.\n";
    }
  }
  
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::pixel_factor_changed
//       Access: Protected, Virtual
//  Description: Called internally when the pixel factor changes.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
pixel_factor_changed() {
  x11GraphicsWindow::pixel_factor_changed();
  create_reduced_frame_buffer();
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::create_full_frame_buffer
//       Access: Private
//  Description: Creates a suitable frame buffer for the current
//               window size.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
create_full_frame_buffer() {
  if (_full_frame_buffer != NULL) {
    ZB_close(_full_frame_buffer);
    _full_frame_buffer = NULL;
  }

  int mode;
  switch (_bytes_per_pixel) {
  case  1:
    tinydisplay_cat.error()
      << "Palette images are currently not supported.\n";
    return;

  case 2:
    mode = ZB_MODE_5R6G5B;
    break;
  case 4:
    mode = ZB_MODE_RGBA;
    break;

  default:
    return;
  }

  _full_frame_buffer = ZB_open(_properties.get_x_size(), _properties.get_y_size(), mode, 0, 0, 0, 0);
  _pitch = (_full_frame_buffer->xsize * _bytes_per_pixel + 3) & ~3;
}

////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::create_reduced_frame_buffer
//       Access: Private
//  Description: Creates a suitable frame buffer for the current
//               window size and pixel zoom.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
create_reduced_frame_buffer() {
  if (_reduced_frame_buffer != NULL) {
    ZB_close(_reduced_frame_buffer);
    _reduced_frame_buffer = NULL;
  }

  int x_size = get_fb_x_size();
  int y_size = get_fb_y_size();

  if (x_size == _full_frame_buffer->xsize) {
    // No zooming is necessary.

  } else {
    // The reduced size is different, so we need a separate buffer to
    // render into.
    _reduced_frame_buffer = ZB_open(x_size, y_size, _full_frame_buffer->mode, 0, 0, 0, 0);
  }
}


////////////////////////////////////////////////////////////////////
//     Function: TinyXGraphicsWindow::create_ximage
//       Access: Private
//  Description: Creates a suitable XImage for the current
//               window size.
////////////////////////////////////////////////////////////////////
void TinyXGraphicsWindow::
create_ximage() {
  if (_ximage != NULL) {
    PANDA_FREE_ARRAY(_ximage->data);
    _ximage->data = NULL;
    XDestroyImage(_ximage);
    _ximage = NULL;
  }

  int image_size = _full_frame_buffer->ysize * _pitch;
  char *data = (char *)PANDA_MALLOC_ARRAY(image_size);

  _ximage = XCreateImage(_display, _visual, _depth, ZPixmap, 0, data,
                         _full_frame_buffer->xsize, _full_frame_buffer->ysize,
                         32, 0);
}

#endif  // HAVE_X11

