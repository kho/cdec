# csynctx.pyx
# Defines "syntactic context"

import cintlist
import gzip
import cbitlist
import sys

from libc.stdio cimport FILE, fopen, fread, fwrite, fclose
from libc.stdlib cimport malloc, realloc, free
from libc.string cimport memset, strcpy, strlen

cdef class HieroItem:
  cdef public b, e
  cdef public is_nt
  
  def __init__(self, b, e, is_nt):
    self.b = b
    self.e = e
    self.is_nt = is_nt

cdef class HieroPhrase:
  def __init__(self, gap_items, left_index, right_index):
    self.hiero_items = []
    self.left_index = left_index
    self.right_index = right_index
    
    cdef int item_index, gap_index, num_gap
    item_index = left_index
    gap_index = 0
    num_gap = len(gap_items)
    
    while gap_index < num_gap:
      while item_index < gap_items[gap_index][0]:
        self.add_item(item_index, item_index, 0)
        item_index = item_index + 1
      self.add_item(gap_items[gap_index][0], gap_items[gap_index][1], 1)
      item_index = gap_items[gap_index][1] + 1
      gap_index = gap_index + 1
      
    while item_index <= right_index:
      self.add_item(item_index, item_index, 0)
      item_index = item_index + 1

  def add_item(self, s, e, is_nt):
    self.hiero_items.append(HieroItem(s, e, is_nt))
    if s < self.left_index or self.left_index == -1 :
      self.left_index = s
    if e > self.right_index or self.right_index == -1:
      self.right_index = e

  def get_left(self):
    return self.left_index

  def get_right(self):
    return self.right_index

  def to_line(self):
    fields = []
    for i in xrange(len(self.hiero_items)):
      fields.append('(' + str(self.hiero_items[i].b) + ',' + str(self.hiero_items[i].e) + ',' + str(self.hiero_items[i].is_nt) + ')')
    return "".join(fields)

cdef class SyntacticContext:
  def __init__(self, filename=None, from_binary=False):
    self.pos2id = {}
    self.id2pos = []
    self.pos_arr = cintlist.CIntList(10000,10000)
    self.head_arr = cintlist.CIntList(10000,10000)
    self.sent_id = cintlist.CIntList(10000,10000)

    if filename is not None:
      if from_binary:
        self.read_binary(filename)
      else:
        self.read_text(filename)
        
  def get_pos_id(self, word):
    if not word in self.pos2id:
      self.pos2id[word] = len(self.id2pos)
      self.id2pos.append(word)
    return self.pos2id[word]


  def get_pos(self, id):
    return self.id2pos[id]

  #pivots are words inside the span
  def get_pivots(self, sentence_id, s, e):
    pivots = []
    for i from s <= i <= e:
      j = self.head_arr[self.sent_id[sentence_id] + i]
      if j < s or j > e:
        pivots.append(i)
    return pivots

  #heads are words outside the span
  def get_heads(self, sentence_id, s, e):
    heads = []
    for i from s <= i <= e:
      j = self.head_arr[self.sent_id[sentence_id] + i]
      if (j < s or j > e) and j not in heads:
        heads.append(j)
      
    return heads
        
    
  def read_text(self, filename):
    print "Reading syntactic context (dependency) from file %s" % filename
    
    if filename[-2:] == "gz":
        file = gzip.GzipFile(filename)
    else:
        file = open(filename)
    while 1:
        line = file.readline()
        if not line:
            break
        line.replace( '/n','' );
        heads = line.split()
        line = file.readline()
        line.replace( '/n','' );
        pos_tags = line.split()
        assert len(pos_tags) == len(heads)
        size = len(self.head_arr);
        self.sent_id.append(size)
        for pos in pos_tags:
          self.pos_arr.append(self.get_pos_id(pos))
        for head in heads:
          self.head_arr.append(int(head))
    self.sent_id.append(len(self.head_arr))
    
  def get_pos_tag(self, sent_id, index):
    return self.get_pos(self.pos_arr[self.sent_id[sent_id] + index]) 
   
  def get_syn_ctx_by_hiero_phrase(self, sent_id, hiero_phrase):
    #print "hiero_phrase: ", hiero_phrase.to_line()
    pivots_array = []
    heads_array = []
    for hiero_item in hiero_phrase.hiero_items:
      #print "span[%d, %d]" % (hiero_item.b, hiero_item.e)
      pivots = self.get_pivots(sent_id, hiero_item.b, hiero_item.e)
      #print "".join(map(str, pivots))
      pivots_array.append(pivots)
      heads = self.get_heads(sent_id, hiero_item.b, hiero_item.e)
      #print ",".join(map(str, heads))
      heads_array.append(heads)

    span_heads = self.get_heads(sent_id, hiero_phrase.get_left(), hiero_phrase.get_right())
    #print ".".join(map(str, span_heads))
    
    size_item = len(hiero_phrase.hiero_items)
    s = ""
    fields = []
    #fields.append("&".join(map(str, span_heads))) #the heads of this span
    pos = []
    for i in span_heads:
      if i == -1:
        pos.append("ROOT")
      else:
        pos.append(self.get_pos_tag(sent_id, i))
    fields.append("&".join(pos)) #the heads' postags of this span
    
    for i in xrange(size_item):
      curr_size_pivot = len(pivots_array[i])
      curr_pivot_postags = []
      for j in xrange(curr_size_pivot):
        curr_pivot_postags.append(self.get_pos_tag(sent_id, pivots_array[i][j]))
      fields.append("&".join(curr_pivot_postags)) #the pivots' postags of this span
    
    for i in xrange(size_item):
      curr_heads = []
      curr_size_head = len(heads_array[i])
      for j in xrange(curr_size_head):
        if heads_array[i][j] in span_heads:
          #head of this span
          curr_heads.append(-1)
          continue
        for k in xrange(size_item):
          if k == i:
            continue
          if heads_array[i][j] in pivots_array[k]:
            if k not in curr_heads:
              curr_heads.append(k)
              break
      fields.append("&".join(map(str, curr_heads))) #the heads of this item in terms of the position in the span
    #print "|".join(fields)
    return "|".join(fields)
   

  def get_syn_ctx(self, sent_id, gap_items, left_index, right_index):
    #print "sent_id: %d, span: [%d, %d], gap_items: " % (sent_id, left_index, right_index)
    #for i in xrange(len(gap_items)):
      #print "gap_items[%d]: [%d, %d]" % (i, gap_items[i][0], gap_items[i][1])
    hiero_phrase = HieroPhrase(gap_items, left_index, right_index)
    return self.get_syn_ctx_by_hiero_phrase(sent_id, hiero_phrase)

  cdef void write_handle(self, FILE* f):
    cdef int pos_len
    cdef int num_pos
    cdef char* c_pos

    self.head_arr.write_handle(f)
    self.pos_arr.write_handle(f)
    self.sent_id.write_handle(f)
    
    num_pos = len(self.id2pos)
    fwrite(&(num_pos), sizeof(int), 1, f)
    for pos in self.id2pos:
      c_pos = pos
      pos_len = strlen(c_pos) + 1
      fwrite(&(pos_len), sizeof(int), 1, f)
      fwrite(c_pos, sizeof(char), pos_len, f)

  def write_binary(self, filename):
    cdef FILE* f
    cdef bytes bfilename = filename
    cdef char* cfilename = bfilename
    f = fopen(cfilename, "w")
    self.write_handle(f)
    fclose(f)


  cdef void read_handle(self, FILE* f):
    cdef int num_pos
    cdef int pos_len
    cdef char* c_pos
    cdef bytes py_pos
    
    self.head_arr.read_handle(f)
    self.pos_arr.read_handle(f)
    self.sent_id.read_handle(f)
    fread(&(num_pos), sizeof(int), 1, f)
    for i in xrange(num_pos):
      fread(&(pos_len), sizeof(int), 1, f)
      c_pos = <char*> malloc (pos_len * sizeof(char))
      fread(c_pos, sizeof(char), pos_len, f)
      py_pos = c_pos
      free(c_pos)
      self.pos2id[py_pos] = len(self.id2pos)
      self.id2pos.append(py_pos)

  def read_binary(self, filename):
    cdef FILE* f
    cdef bytes bfilename = filename
    cdef char* cfilename = bfilename
    f = fopen(cfilename, "r")
    self.read_handle(f)
    fclose(f)

cdef class ContextRule:
  def __init__(self, f, f_context, e, score):
    self.f = f
    self.e = e
    self.f_context = f_context
    self.score = score

  def to_line(self):
    fields = [str(self.f), str(self.e), self.f_context, str(self.score)]
      
    return " ||| ".join(fields)
