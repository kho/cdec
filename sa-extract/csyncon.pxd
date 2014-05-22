cimport cintlist


cdef class SyntacticContraint:
	cdef arr
	cdef cintlist.CIntList sent_id
	cdef int switch_on
	cdef int _is_valid(self, int sent_id, int low, int high)
