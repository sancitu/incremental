// Filename: pixelBuffer.cxx
// Created by:  mike (09Jan97)
//
////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////
#include "pixelBuffer.h"
#include "config_gobj.h"

////////////////////////////////////////////////////////////////////
// Static variables
////////////////////////////////////////////////////////////////////
TypeHandle PixelBuffer::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: config 
//       Access:
//  Description:
////////////////////////////////////////////////////////////////////
void PixelBuffer::config(void)
{
  ImageBuffer::config();
}

////////////////////////////////////////////////////////////////////
//     Function: read
//       Access:
//  Description:
////////////////////////////////////////////////////////////////////
bool PixelBuffer::read(const string& name)
{
  PNMImage pnmimage;

  if (!pnmimage.read(name)) {
    gobj_cat.error()
      << "PixelBuffer::read() - couldn't read: " << name << endl;
    return false;
  }

  set_name(name);
  return load(pnmimage);
}

////////////////////////////////////////////////////////////////////
//     Function: write
//       Access:
//  Description:
////////////////////////////////////////////////////////////////////
bool PixelBuffer::write( const string& name ) const
{
  PNMImage pnmimage;
  if (!store(pnmimage)) {
    return false;
  }

  string tname;
  if (name.empty()) {
    tname = get_name();
  } else {
    tname = name;
  }

  if (!pnmimage.write(tname)) {
    gobj_cat.error()
      << "PixelBuffer::write() - couldn't write: " << name << endl;
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: read
//       Access:
//  Description:
////////////////////////////////////////////////////////////////////
bool PixelBuffer::load(const PNMImage& pnmimage)
{
  _xsize = pnmimage.get_x_size();
  _ysize = pnmimage.get_y_size();
  _components = pnmimage.get_num_channels();

  xelval maxval = pnmimage.get_maxval();

  // Come up with a default format based on the number of channels.
  switch (pnmimage.get_color_type()) {
  case PNMImage::CT_grayscale:
    _format = F_luminance;
    break;

  case PNMImage::CT_two_channel:
    _format = F_luminance_alpha;
    break;

  case PNMImage::CT_color:
    // Choose a suitable default format based on the depth of the
    // image components.
    if (maxval > 255) {
      _format = F_rgb12;
    } else if (maxval > 31) {
      _format = F_rgb8;
    } else {
      _format = F_rgb5;
    }
    break;

  case PNMImage::CT_four_channel:
    // Choose a suitable default format based on the depth of the
    // image components.
    if (maxval > 255) {
      _format = F_rgba12;
    } else if (maxval > 15) {
      _format = F_rgba8;
    } else {
      _format = F_rgba4;
    }
    break;

  default:
    // Eh?
    nassertr(false, false);
    _format = F_rgb;    
  };

  bool has_alpha = pnmimage.has_alpha();
  bool is_grayscale = pnmimage.is_grayscale();

  if (maxval > 255) {
    // Wide pixels; we need to use a short for each component.
    _type = T_unsigned_short;
    _image = PTA_uchar(_xsize * _ysize * _components * 2);
    int idx = 0;
    
    for (int j = _ysize-1; j >= 0; j--) {
      for (int i = 0; i < _xsize; i++) {
	if (is_grayscale) {
	  store_unsigned_short(idx, pnmimage.get_gray(i, j));
	} else {
	  store_unsigned_short(idx, pnmimage.get_red(i, j));
	  store_unsigned_short(idx, pnmimage.get_green(i, j));
	  store_unsigned_short(idx, pnmimage.get_blue(i, j));
	}
	if (has_alpha) {
	  store_unsigned_short(idx, pnmimage.get_alpha(i, j));
	}
      }
    }
  } else {
    // Normal pixels: a byte per component will do.
    _type = T_unsigned_byte;
    _image = PTA_uchar(_xsize * _ysize * _components);
    int idx = 0;
    
    for (int j = _ysize-1; j >= 0; j--) {
      for (int i = 0; i < _xsize; i++) {
	if (is_grayscale) {
	  store_unsigned_byte(idx, pnmimage.get_gray(i, j));
	} else {
	  store_unsigned_byte(idx, pnmimage.get_red(i, j));
	  store_unsigned_byte(idx, pnmimage.get_green(i, j));
	  store_unsigned_byte(idx, pnmimage.get_blue(i, j));
	}
	if (has_alpha) {
	  store_unsigned_byte(idx, pnmimage.get_alpha(i, j));
	}
      }
    }
  }

  config();
  return true;
}


////////////////////////////////////////////////////////////////////
//     Function: store
//       Access:
//  Description:
////////////////////////////////////////////////////////////////////
bool PixelBuffer::
store(PNMImage &pnmimage) const {
  if (_type == T_unsigned_byte) {
    pnmimage.clear(_xsize, _ysize, _components);
    bool has_alpha = pnmimage.has_alpha();
    bool is_grayscale = pnmimage.is_grayscale();

    int idx = 0;
    for (int j = _ysize-1; j >= 0; j--) {
      for (int i = 0; i < _xsize; i++) {
	if (is_grayscale) {
	  pnmimage.set_gray(i, j, get_unsigned_byte(idx));
	} else {
	  pnmimage.set_red(i, j, get_unsigned_byte(idx));
	  pnmimage.set_green(i, j, get_unsigned_byte(idx));
	  pnmimage.set_blue(i, j, get_unsigned_byte(idx));
	}
	if (has_alpha)
	  pnmimage.set_alpha(i, j, get_unsigned_byte(idx));
      }
    }
    return true;

  } else if (_type == T_unsigned_short) {
    pnmimage.clear(_xsize, _ysize, _components, 65535);
    bool has_alpha = pnmimage.has_alpha();
    bool is_grayscale = pnmimage.is_grayscale();

    int idx = 0;
    for (int j = _ysize-1; j >= 0; j--) {
      for (int i = 0; i < _xsize; i++) {
	if (is_grayscale) {
	  pnmimage.set_gray(i, j, get_unsigned_short(idx));
	} else {
	  pnmimage.set_red(i, j, get_unsigned_short(idx));
	  pnmimage.set_green(i, j, get_unsigned_short(idx));
	  pnmimage.set_blue(i, j, get_unsigned_short(idx));
	}
	if (has_alpha)
	  pnmimage.set_alpha(i, j, get_unsigned_short(idx));
      }
    }
    return true;
  }    

  gobj_cat.error()
    << "Couldn't write image for " << get_name()
    << "; inappropriate type " << _type << ".\n";
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: copy 
//       Access:
//  Description: Deep copy of pixel buffer
////////////////////////////////////////////////////////////////////
void PixelBuffer::
copy(const PixelBuffer *pb) {
  nassertv(pb != NULL);
  _xorg = pb->_xorg;
  _yorg = pb->_yorg;
  _xsize = pb->_xsize;
  _ysize = pb->_ysize;
  _border = pb->_border;
  _components = pb->_components;
  _format = pb->_format;
  _image = PTA_uchar(0);
  if (!pb->_image.empty())
    _image.v() = pb->_image.v(); 
}

void PixelBuffer::copy(GraphicsStateGuardianBase *gsg, const DisplayRegion *dr) {
  gsg->copy_pixel_buffer(this, dr);
}

void PixelBuffer::copy(GraphicsStateGuardianBase *gsg, const DisplayRegion *dr,
		       const RenderBuffer &rb) {
  gsg->copy_pixel_buffer(this, dr, rb);
}

void PixelBuffer::draw(GraphicsStateGuardianBase *) {
  gobj_cat.error()
    << "DisplayRegion required to draw pixel buffer.\n";
}

void PixelBuffer::draw(GraphicsStateGuardianBase *gsg, const DisplayRegion *dr) {
  gsg->draw_pixel_buffer(this, dr);
}

void PixelBuffer::draw(GraphicsStateGuardianBase *gsg, const DisplayRegion *dr,
			const RenderBuffer &rb) {
  gsg->draw_pixel_buffer(this, dr, rb);
}

