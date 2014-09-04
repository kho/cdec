cimport cintlist
cimport rule
from libc.stdio cimport FILE

cdef class HieroPhrase:
    cdef public hiero_items
    cdef left_index, right_index
    

cdef class SyntacticContext:
    cdef pos2id
    cdef id2pos
    cdef cintlist.CIntList head_arr
    cdef cintlist.CIntList pos_arr
    cdef cintlist.CIntList sent_id 
    cdef void write_handle(self, FILE* f)
    cdef void read_handle(self, FILE* f)
	
cdef class ContextRule:
    cdef rule.Phrase f, e
    cdef f_context
    cdef float score
    
