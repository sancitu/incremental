// Filename: animInterface.cxx
// Created by:  drose (20Sep05)
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

#include "animInterface.h"
#include "clockObject.h"
#include "bamReader.h"
#include "bamWriter.h"
#include "datagram.h"
#include "datagramIterator.h"

TypeHandle AnimInterface::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
AnimInterface::
AnimInterface() :
  _num_frames(0)
{
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::Copy Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
AnimInterface::
AnimInterface(const AnimInterface &copy) :
  _num_frames(copy._num_frames),
  _cycler(copy._cycler)
{
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::Destructor
//       Access: Published, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
AnimInterface::
~AnimInterface() {
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::get_num_frames
//       Access: Published, Virtual
//  Description: Returns the number of frames in the animation.  This
//               is a property of the animation and may not be
//               directly adjusted by the user (although it may change
//               without warning with certain kinds of animations,
//               since this is a virtual method that may be
//               overridden).
////////////////////////////////////////////////////////////////////
int AnimInterface::
get_num_frames() const {
  return _num_frames;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::output
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void AnimInterface::
output(ostream &out) const {
  CDReader cdata(_cycler);
  cdata->output(out);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::animation_activated
//       Access: Protected, Virtual
//  Description: This is provided as a callback method for when the
//               user calls one of the play/loop/pose type methods to
//               start the animation playing.
////////////////////////////////////////////////////////////////////
void AnimInterface::
animation_activated() {
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void AnimInterface::
write_datagram(BamWriter *manager, Datagram &dg) {
  dg.add_int32(_num_frames);
  manager->write_cdata(dg, _cycler);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new AnimInterface.
////////////////////////////////////////////////////////////////////
void AnimInterface::
fillin(DatagramIterator &scan, BamReader *manager) {
  _num_frames = scan.get_int32();
  manager->read_cdata(scan, _cycler);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
AnimInterface::CData::
CData() :
  _frame_rate(0.0),
  _play_mode(PM_pose),
  _start_time(0.0),
  _start_frame(0.0),
  _play_frames(0.0),
  _from_frame(0),
  _to_frame(0),
  _play_rate(1.0),
  _effective_frame_rate(0.0),
  _paused(true),
  _paused_f(0.0)
{
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::Copy Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
AnimInterface::CData::
CData(const AnimInterface::CData &copy) :
  _frame_rate(copy._frame_rate),
  _play_mode(copy._play_mode),
  _start_time(copy._start_time),
  _start_frame(copy._start_frame),
  _play_frames(copy._play_frames),
  _from_frame(copy._from_frame),
  _to_frame(copy._to_frame),
  _play_rate(copy._play_rate),
  _effective_frame_rate(copy._effective_frame_rate),
  _paused(copy._paused),
  _paused_f(copy._paused_f)
{
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::make_copy
//       Access: Public, Virtual
//  Description:
////////////////////////////////////////////////////////////////////
CycleData *AnimInterface::CData::
make_copy() const {
  return new CData(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
write_datagram(BamWriter *, Datagram &dg) const {
  dg.add_float32(_frame_rate);
  dg.add_uint8(_play_mode);
  dg.add_float32(_start_time);
  dg.add_float32(_start_frame);
  dg.add_float32(_play_frames);
  dg.add_float32(_from_frame);
  dg.add_float32(_to_frame);
  dg.add_float32(_play_rate);
  dg.add_bool(_paused);
  dg.add_float32(_paused_f);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::fillin
//       Access: Public, Virtual
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new AnimInterface.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
fillin(DatagramIterator &scan, BamReader *) {
  _frame_rate = scan.get_float32();
  _play_mode = (PlayMode)scan.get_uint8();
  _start_time = scan.get_float32();
  _start_frame = scan.get_float32();
  _play_frames = scan.get_float32();
  _from_frame = scan.get_float32();
  _to_frame = scan.get_float32();
  _play_rate = scan.get_float32();
  _effective_frame_rate = _frame_rate * _play_rate;
  _paused = scan.get_bool();
  _paused_f = scan.get_float32();
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::play
//       Access: Public
//  Description: Runs the animation from the frame "from" to and
//               including the frame "to", at which point the
//               animation is stopped.  Both "from" and "to" frame
//               numbers may be outside the range (0,
//               get_num_frames()) and the animation will follow the
//               range correctly, reporting numbers modulo
//               get_num_frames().  For instance, play(0,
//               get_num_frames() * 2) will play the animation twice
//               and then stop.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
play(double from, double to) {
  if (from >= to) {
    pose((int)from);
    return;
  }

  _play_mode = PM_play;
  _start_time = ClockObject::get_global_clock()->get_frame_time();
  _start_frame = from;
  _play_frames = to - from + 1.0;
  _from_frame = (int)floor(from);
  _to_frame = (int)floor(to);
  _paused_f = 0.0;

  if (_effective_frame_rate < 0.0) {
    // If we'll be playing backward, start at the end.
    _start_time -= _play_frames / _effective_frame_rate;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::loop
//       Access: Public
//  Description: Loops the animation from the frame "from" to and
//               including the frame "to", indefinitely.  If restart
//               is true, the animation is restarted from the
//               beginning; otherwise, it continues from the current
//               frame.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
loop(bool restart, double from, double to) {
  if (from >= to) {
    pose((int)from);
    return;
  }

  double fframe = get_full_fframe();

  _play_mode = PM_loop;
  _start_time = ClockObject::get_global_clock()->get_frame_time();
  _start_frame = from;
  _play_frames = to - from + 1.0;
  _from_frame = (int)floor(from);
  _to_frame = (int)floor(to);
  _paused_f = 0.0;

  if (!restart) {
    if (_paused) {
      _paused_f = fframe - _start_frame;
    } else {
      _start_time -= fframe / _effective_frame_rate;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::pingpong
//       Access: Public
//  Description: Loops the animation from the frame "from" to and
//               including the frame "to", and then back in the
//               opposite direction, indefinitely.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
pingpong(bool restart, double from, double to) {
  if (from >= to) {
    pose((int)from);
    return;
  }

  double fframe = get_full_fframe();

  _play_mode = PM_pingpong;
  _start_time = ClockObject::get_global_clock()->get_frame_time();
  _start_frame = from;
  _play_frames = to - from + 1.0;
  _from_frame = (int)floor(from);
  _to_frame = (int)floor(to);
  _paused_f = 0.0;

  if (!restart) {
    if (_paused) {
      _paused_f = fframe - _start_frame;
    } else {
      _start_time -= fframe / _effective_frame_rate;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::pose
//       Access: Public
//  Description: Sets the animation to the indicated frame and holds
//               it there.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
pose(int frame) {
  _play_mode = PM_pose;
  _start_time = ClockObject::get_global_clock()->get_frame_time();
  _start_frame = (double)frame;
  _play_frames = 0.0;
  _from_frame = frame;
  _to_frame = frame;
  _paused_f = 0.0;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::get_full_frame
//       Access: Public
//  Description: Returns the current integer frame number.
//
//               Unlike the value returned by get_frame(), this frame
//               number may extend beyond the range of
//               get_num_frames() if the frame range passed to play(),
//               loop(), etc. did.
//
//               Unlike the value returned by get_full_fframe(), this
//               return value will never exceed the value passed to
//               to_frame in the play() method.
////////////////////////////////////////////////////////////////////
int AnimInterface::CData::
get_full_frame() const {
  int frame = (int)floor(get_full_fframe());
  if (_play_mode == PM_play) {
    // In play mode, we never let the return value exceed
    // (_from_frame, _to_frame).
    frame = min(max(frame, _from_frame), _to_frame);
  }
  return frame;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::get_full_fframe
//       Access: Public
//  Description: Returns the current floating-point frame number.
//
//               Unlike the value returned by get_frame(), this frame
//               number may extend beyond the range of
//               get_num_frames() if the frame range passed to play(),
//               loop(), etc. did.
//
//               Unlike the value returned by get_full_frame(), this
//               return value may equal (to_frame + 1.0), when the
//               animation has played to its natural end.  However, in
//               this case the return value of get_full_frame() will
//               be to_frame, not (to_frame + 1).
////////////////////////////////////////////////////////////////////
double AnimInterface::CData::
get_full_fframe() const {
  switch (_play_mode) {
  case PM_pose:
    return _start_frame;

  case PM_play:
    return min(max(get_f(), 0.0), _play_frames) + _start_frame;

  case PM_loop:
    nassertr(_play_frames >= 0.0, 0.0);
    return cmod(get_f(), _play_frames) + _start_frame;

  case PM_pingpong:
    {
      nassertr(_play_frames >= 0.0, 0.0);
      double f = cmod(get_f(), _play_frames * 2.0);
      if (f > _play_frames) {
	return (_play_frames * 2.0 - f) + _start_frame;
      } else {
	return f + _start_frame;
      }
    }
  }

  return _start_frame;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::is_playing
//       Access: Public
//  Description: Returns true if the animation is currently playing,
//               false if it is stopped (e.g. because stop() or pose()
//               was called, or because it reached the end of the
//               animation after play() was called).
////////////////////////////////////////////////////////////////////
bool AnimInterface::CData::
is_playing() const {
  switch (_play_mode) {
  case PM_pose:
    return false;

  case PM_play:
    return get_f() < _play_frames;

  case PM_loop:
  case PM_pingpong:
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::output
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
output(ostream &out) const {
  switch (_play_mode) {
  case PM_pose:
    out << "pose, frame " << get_full_frame();
    return;

  case PM_play:
    out << "play, frame " << get_full_frame();
    return;

  case PM_loop:
    out << "loop, frame " << get_full_frame();
    return;

  case PM_pingpong:
    out << "pingpong, frame " << get_full_frame();
    return;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::internal_set_rate
//       Access: Public
//  Description: Called internally to adjust either or both of the
//               frame_rate or play_rate without changing the current
//               frame number if the animation is already playing.
////////////////////////////////////////////////////////////////////
void AnimInterface::CData::
internal_set_rate(double frame_rate, double play_rate) {
  double f = get_f();
  
  _frame_rate = frame_rate;
  _play_rate = play_rate;
  _effective_frame_rate = frame_rate * play_rate;

  if (_effective_frame_rate == 0.0) {
    _paused_f = f;
    _paused = true;

  } else {
    // Compute a new _start_time that will keep f the same value with
    // the new play_rate.
    double new_elapsed = f / _effective_frame_rate;
    double now = ClockObject::get_global_clock()->get_frame_time();
    _start_time = now - new_elapsed;
    _paused = false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimInterface::CData::get_f
//       Access: Public
//  Description: Returns the current floating-point frame number,
//               elapsed since _start_frame.
////////////////////////////////////////////////////////////////////
double AnimInterface::CData::
get_f() const {
  if (_paused) {
    return _paused_f;

  } else {
    double now = ClockObject::get_global_clock()->get_frame_time();
    double elapsed = now - _start_time;
    return (elapsed * _effective_frame_rate);
  }
}
