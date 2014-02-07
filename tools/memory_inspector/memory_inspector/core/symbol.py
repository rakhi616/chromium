# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Symbols(object):
  """A dictionary of symbols indexed by the key 'exec_path+0xoffset'."""

  def __init__(self):
    self.dict = {}

  def Add(self, exec_file_rel_path, offset, symbol):
    assert(isinstance(symbol, Symbol))
    self.dict[Symbols._GetKey(exec_file_rel_path, offset)] = symbol

  def Lookup(self, exec_file_rel_path, offset):
    return self.dict.get(Symbols._GetKey(exec_file_rel_path, offset))

  def Merge(self, other):
    assert(isinstance(other, Symbols))
    self.dict.update(other.dict)  # pylint: disable=W0212

  @property
  def length(self):
    return len(self.dict)

  @staticmethod
  def _GetKey(exec_file_rel_path, offset):
    return '%s+0x%x' % (exec_file_rel_path, offset)


class Symbol(object):
  """Debug information relative to a symbol.

  Note: a symbol can have more than one source line associated to it.
  """

  def __init__(self, name, source_file_path, line_number):
    self.name = name
    self.source_info = []
    self.AddSourceLineInfo(source_file_path, line_number)

  def AddSourceLineInfo(self, source_file_path, line_number):
    self.source_info += [SourceInfo(source_file_path, line_number)]

  def __str__(self):
    return '%s %s' % (
      self.name,
      self.source_info[0] if len(self.source_info) else '')


class SourceInfo(object):
  """Source file + line information for a given |Symbol|."""

  def __init__(self, source_file_path, line_number):
    assert(isinstance(line_number, int))
    self.source_file_path = source_file_path
    self.line_number = line_number

  def __str__(self):
    return '%s:%d' % (self.source_file_path, self.line_number)
