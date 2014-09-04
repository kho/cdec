# cintlist.pyx
# defines bit arrays in C (only support some simple functions)

from libc.stdlib cimport malloc, realloc, free
from libc.string cimport memset, memcpy

cdef class CBitList:
  
  def __cinit__(self, size=127, initial_len=0):
    if initial_len > size:
      size = initial_len
    cdef int char_size #byte size: 1 byte contains 8 bits
    char_size = size / 8
    if (size % 8 > 0):
      char_size = char_size + 1
    
    self.arr = <char*> malloc(char_size*sizeof(char))
    memset(self.arr, 0, char_size*sizeof(char))
    
  def __init__(self, size=127, initial_len=0):
    self.size = size
    if initial_len > size:
      self.size = initial_len
    self.len = initial_len
    
    cdef int char_size #byte size: 1 byte contains 8 bits
    char_size = size / 8
    if (size % 8 > 0):
      char_size = char_size + 1
    self.byte_size = char_size
    
  def __dealloc__(self):
    free(self.arr)  
    
  def get_len(self):
    return self.len
    
  def get(self, int index):
    return self._get(index)
  
  cdef int _get(self, int index):
    cdef int i
    
    i = index
    if i < 0:
      i = self.len + i
    if i < 0 or i >= self.len:
      raise IndexError("Requested index %d of %d-length CByteList" % (index, self.len))
    
    cdef int byte_index, bit_index
    byte_index = index/8
    bit_index = index%8
    cdef char res
    if bit_index == 0:
      res = self.arr[byte_index] & 0x01
    elif bit_index == 1:
      res = self.arr[byte_index] & 0x02
    elif bit_index == 2:
      res = self.arr[byte_index] & 0x04
    elif bit_index == 3:
      res = self.arr[byte_index] & 0x08
    elif bit_index == 4:
      res = self.arr[byte_index] & 0x10
    elif bit_index == 5:
      res = self.arr[byte_index] & 0x20
    elif bit_index == 6:
      res = self.arr[byte_index] & 0x40
    elif bit_index == 7:
      res = self.arr[byte_index] & 0x80
    else:
      raise IndexError("")
    if res != 0:
      return 1
    else:
      return 0
  
  def set(self, int i, int val):
    self._set(i, val)
  
  cdef void _set(self, int i, int val):
    j = i
    if i < 0:
      j = self.len + i
    if j < 0 or j >=self.len:
      raise IndexError("Requested index %d of %d-length BitList" % (i, self.len))
    if type(val) != int:
      raise TypeError
    if val != 0 and val != 1:
      raise TypeError
    cdef int byte_index, bit_index
    byte_index = i/8
    bit_index = i%8
    cdef char res
    if bit_index == 0:
      if val == 0:
        res = self.arr[byte_index] & 0xFE
      else:
        res = self.arr[byte_index] | 0x01
    elif bit_index == 1:
      if val == 0:
        res = self.arr[byte_index] & 0xFD
      else:
        res = self.arr[byte_index] | 0x02
    elif bit_index == 2:
      if val == 0:
        res = self.arr[byte_index] & 0xFB
      else:
        res = self.arr[byte_index] | 0x04
    elif bit_index == 3:
      if val == 0:
        res = self.arr[byte_index] & 0xF7
      else:
        res = self.arr[byte_index] | 0x08
    elif bit_index == 4:
      if val == 0:
        res = self.arr[byte_index] & 0xEF
      else:
        res = self.arr[byte_index] | 0x10
    elif bit_index == 5:
      if val == 0:
        res = self.arr[byte_index] & 0xDF
      else:
        res = self.arr[byte_index] | 0x20
    elif bit_index == 6:
      if val == 0:
        res = self.arr[byte_index] & 0xBF
      else:
        res = self.arr[byte_index] | 0x40
    elif bit_index == 7:
      if val == 0:
        res = self.arr[byte_index] & 0x7F
      else:
        res = self.arr[byte_index] | 0x80
    else:
      raise IndexError("")
    self.arr[byte_index] = res
    
  def __setitem__(self, i, val):
    self.set(i, val)
    
  def __len__(self):
    return self.len

  def getSize(self):
    return self.size

  cdef void _clear(self):
    free(self.arr)
    self.len = 0
    self.size = 0
    self.arr = <char*> malloc(0)
