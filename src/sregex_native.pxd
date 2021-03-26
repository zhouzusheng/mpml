cdef extern from "sregex/sregex_wrap.h":
    ctypedef struct regexp_wrap_s:
        pass
    ctypedef regexp_wrap_s* regexp_wrap
    ctypedef struct regexp_result:
        int start
        int end
        int data
    ctypedef int (*regexp_result_callback)(void* result_cb_data,
                                                 regexp_result* result)

    regexp_wrap regexp_wrap_new()
    void regexp_wrap_free(regexp_wrap self)
    int regexp_wrap_compile(regexp_wrap self, const char* text)
    int regexp_wrap_compile_multi(regexp_wrap self, const char** text, int len)
    int regexp_wrap_exec(regexp_wrap self, const char* text, int len, regexp_result_callback callback, void* userData)
    int regexp_wrap_save(regexp_wrap self, file);
    int regexp_wrap_load(regexp_wrap self, file);