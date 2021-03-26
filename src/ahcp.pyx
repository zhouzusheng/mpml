cimport aho_corasick

cdef aho_corasick.ac_error_code append_result(void* data,
                                              aho_corasick.ac_result* result):
    result_list = <list>data
    result_tuple = (result.start, result.end, result.data)
    result_list.append(result_tuple)
    return aho_corasick.AC_SUCCESS

cdef text_as_bytes(text):
    if type(text) is unicode:
        return text.encode('utf8')
    else:
        return text

cdef class Index:
    cdef aho_corasick.ac_index _index
    def __cinit__(self):
        self._index = aho_corasick.ac_index_new()
        if self._index is NULL:
            raise MemoryError()
           
    def __dealloc__(self):
        aho_corasick.ac_index_free(self._index)

    def load(self, f):
        aho_corasick.ac_index_load(self._index, f)

    def loadFile(self, file):
        f = open(file, 'rb')
        aho_corasick.ac_index_load(self._index, f)
        f.close()

    def save(self, f):
        aho_corasick.ac_index_save(self._index, f)

    def saveFile(self, file):
        f = open(file, 'wb')
        aho_corasick.ac_index_save(self._index, f)
        f.close()

    def enter(self, keyword, data:int):
        
        keyword = text_as_bytes(keyword)

        if aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call enter after fix")

        if aho_corasick.ac_index_enter(self._index,
                                       keyword, len(keyword),
                                       data) != aho_corasick.AC_SUCCESS:
            raise MemoryError()

    def fix(self):
        if aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call fix repeatedly")

        if aho_corasick.ac_index_fix(self._index) != aho_corasick.AC_SUCCESS:
            raise MemoryError()

    def query(self, phrase):
        if not aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call query before fix")

        phrase = text_as_bytes(phrase)

        result_list = []

        status = aho_corasick.ac_index_query_cb(self._index,
                                          phrase,
                                          len(phrase),
                                          append_result,
                                          <void*>result_list)
        if status != aho_corasick.AC_SUCCESS:
            raise MemoryError()

        return result_list
