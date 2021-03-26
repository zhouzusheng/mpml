#ifndef SREGEX_WRAP_H
#define SREGEX_WRAP_H

#include <Python.h>

typedef struct regexp_wrap_s *regexp_wrap;
regexp_wrap regexp_wrap_new();

typedef struct regexp_result {
	/**
	 * The offset of the first symbol in the matching substring of the
	 * query phrase.
	 */
	int start;

	/** The offset of the symbol after the last symbol in the matching
	 * substring of the query phrase.
	 */
	int end;

	/** Pointer associated with the keyword. */
	int data;
} regexp_result;

typedef int(*regexp_result_callback)(void *userData,
	regexp_result *result);

void regexp_wrap_free(regexp_wrap self);
int regexp_wrap_compile(regexp_wrap self, const char* text);
int regexp_wrap_compile_multi(regexp_wrap self, const char** text, int len);
int regexp_wrap_exec(regexp_wrap self, const char* text, int len, regexp_result_callback callback, void* userData);
int regexp_wrap_save(regexp_wrap self, PyObject* file);
int regexp_wrap_load(regexp_wrap self, PyObject* file);
#endif