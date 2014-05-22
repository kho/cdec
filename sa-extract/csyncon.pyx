# csyncon.pyx
# Defines "syntactic constraints"

import cintlist
import gzip
import cbitlist
import sys

cdef class SyntacticContraint:
  def __init__(self, filename=None):
    self.arr = []
    self.sent_id = cintlist.CIntList(1000,1000)
    if ( filename != None ):
        self.read_text(filename)
        self.switch_on = 1
    else:
        self.switch_on = 0
    
  def read_text(self, filename):
    print "Reading syntactic constraints from file %s" % filename
    if filename[-2:] == "gz":
        file = gzip.GzipFile(filename)
    else:
        file = open(filename)
    while 1:
        line = file.readline()
        if not line:
            break
        line.replace( '/n','' );
        num_word = int(line)
        size = len(self.arr);
        self.sent_id.append(size)
        for i in xrange(num_word):
            line = file.readline()
            line.replace( '/n', '' )
            size = len(self.arr);
            array = line.split()
            if len(array) == 0:
              bit_size = 1
            else:
              bit_size = int(array[-1]) - i + 1
            self.arr.append(cbitlist.CBitList(bit_size, bit_size))
            for word in line.split():
              j = int(word) - i
              assert j < bit_size
              self.arr[size].set(j, 1)
    self.sent_id.append(len(self.arr))
                
  cdef int _is_valid(self, int sent_id, int low, int high):
    if ( self.switch_on == 0 ):
        return 1
    
    if ( low == high ):
        return 1
    
    if ( len(self.arr) - 1 < sent_id ):
        return 0
    
    if ( self.sent_id[sent_id] + low >= self.sent_id[sent_id+1]):
        return 0
    
    if (high - low >= self.arr[self.sent_id[sent_id] + low].get_len()):
      return 0
  
    if self.arr[self.sent_id[sent_id] + low].get(high - low) == 1:
      return 1
    return 0

  def is_valid(self, sent_id, low, high):
    return self._is_valid(sent_id, low, high)

"""
  def append(self, int val):
    self.arr.append(cintlist.CIntList(10001,1000))
    self.arr[len(self.arr)].append( val )


  def read_text(self, filename):
    print "Reading syntactic constraints from file %s" % filename
    if filename[-2:] == "gz":
        file = gzip.GzipFile(filename)
    else:
        file = open(filename)
    while 1:
        line = file.readline()
        if not line:
            break
        line.replace( '/n','' );
        num_word = int(line)
        size = len(self.arr);
        self.sent_id.append(size)
        for i in xrange(num_word):
            line = file.readline()
            line.replace( '/n', '' )
            size = len(self.arr);
            self.arr.append(cintlist.CIntList(5, 5))
            for word in line.split():
                self.arr[size].append(int(word))
    self.sent_id.append(len(self.arr))
                
  cdef int _is_valid(self, int sent_id, int low, int high):
    if ( self.switch_on == 0 ):
        return 1
    
    if ( low == high ):
        return 1
    
    if ( len(self.arr) - 1 < sent_id ):
        return 0
    
    if ( self.sent_id[sent_id] + low >= self.sent_id[sent_id+1]):
        return 0
    
    for a_i in self.arr[self.sent_id[sent_id] + low]:
        if (high == a_i):
            return 1
        elif ( a_i > high ):
            return 0
    return 0

  def is_valid(self, sent_id, low, high):
    if ( self.switch_on == 0 ):
        return 1
    
    if ( low == high ):
        return 1
    
    if ( len(self.arr) - 1 < sent_id ):
        return 0
    
    if ( self.sent_id[sent_id] + low >= self.sent_id[sent_id+1]):
        return 0
    
    for a_i in self.arr[self.sent_id[sent_id] + low]:
        if (high == a_i):
            return 1
        elif ( a_i > high ):
            return 0
    return 0
"""
