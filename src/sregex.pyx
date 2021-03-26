cimport sregex_native
from libc.stdlib cimport malloc, free

cdef int append_result(void* data, sregex_native.regexp_result* result):
    result_list = <list>data
    result_tuple = (result.start, result.end, result.data)
    result_list.append(result_tuple)
    return 1

cdef text_as_bytes(text):
    if type(text) is unicode:
        return text.encode('utf8')
    else:
        return text

cdef class RegExp:
    cdef sregex_native.regexp_wrap _regexp_wrap
    def __cinit__(self):
        self._regexp_wrap = sregex_native.regexp_wrap_new()
        if self._regexp_wrap is NULL:
            raise MemoryError()

    def __dealloc__(self):
        sregex_native.regexp_wrap_free(self._regexp_wrap)

    def compile(self, text):
         bytes = text_as_bytes(text)
         if sregex_native.regexp_wrap_compile(self._regexp_wrap, bytes) < 0:
            raise MemoryError()

    def load(self, f):
        sregex_native.regexp_wrap_load(self._regexp_wrap, f)

    def loadFile(self, file):
        f = open(file, 'rb')
        sregex_native.regexp_wrap_load(self._regexp_wrap, f)
        f.close()

    def save(self, f):
        sregex_native.regexp_wrap_save(self._regexp_wrap, f)
    def saveFile(self, file):
        f = open(file, 'wb')
        sregex_native.regexp_wrap_save(self._regexp_wrap, f)
        f.close()

    def compile_multi(self, texts):
        count = len(texts)
        bytes = [text_as_bytes(str) for (str) in texts]
        cdef char** data
        data = <char **> malloc(count * 8)
        for i in range(0, count):
            data[i] = bytes[i]
        if sregex_native.regexp_wrap_compile_multi(self._regexp_wrap, data, count) < 0:
            free(data)
            raise MemoryError()
        free(data)

    def query(self, phrase):
        bytes = text_as_bytes(phrase)
        result_list = []
        rc = sregex_native.regexp_wrap_exec(self._regexp_wrap, bytes, len(bytes), append_result,<void*>result_list)
        return result_list