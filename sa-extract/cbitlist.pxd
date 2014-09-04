cdef class CBitList:
	cdef int size
	cdef int len
	cdef int byte_size
	cdef char* arr
	cdef void _clear(self)
	cdef void _set(self, int i, int val)
	cdef int _get(self, int i)

