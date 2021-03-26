#include "sregex_wrap.h"
#include "sregex.h"
#include "sre_palloc.h"

#include "../number_conv.h"
#include "sre_vm_bytecode.h"

struct regexp_wrap_s {
	sre_pool_t *pool;
	sre_program_t *prog;
	int multi;
};

struct regexp_multi_context {
	sre_pool_t *pool;

	regexp_result* result;
	int  result_size;
	int  result_capacity;
};

regexp_wrap regexp_wrap_new() {
	regexp_wrap self = malloc(sizeof(struct regexp_wrap_s));
	self->pool = sre_create_pool(1024);
	self->prog = 0;
	self->multi = 0;
	return self;
}

sre_int_t appendResult(sre_int_t id, sre_int_t start, sre_int_t end, void* userData) {
	regexp_result* insert;
	struct regexp_multi_context* ctx = (struct regexp_multi_context*)userData;
	if (!ctx->result) {
		ctx->result = sre_palloc(ctx->pool, 10 * sizeof(regexp_result));
		ctx->result_size = 0;
		ctx->result_capacity = 10;
	}
	else if((ctx->result_size +1)== ctx->result_capacity){
		int result_capacity;
		if (ctx->result_capacity < 40) {
			result_capacity = ctx->result_capacity * 2;
		}
		else {
			result_capacity = ctx->result_capacity + 20;
		}
		regexp_result* tmp = sre_palloc(ctx->pool, result_capacity * sizeof(regexp_result));
		memcpy(tmp, ctx->result, ctx->result_size * sizeof(regexp_result));
		sre_pfree(ctx->pool, ctx->result);
		ctx->result = tmp;
		ctx->result_capacity = result_capacity;
	}
	for (int i = 0; i < ctx->result_size; i++) {
		regexp_result* old = ctx->result + i;
		if (old->data == id && old->start == start) {
			if (old->end < end) {
				old->end = end;
			}
			return 0;
		}
	}
	insert = ctx->result + (ctx->result_size++);
	insert->start = start;
	insert->end = end;
	insert->data = id;
	return 0;
}

void regexp_wrap_free(regexp_wrap self) {
	if (self) {
		if (self->pool) {
			sre_destroy_pool(self->pool);
		}
		free(self);
	}
}

int regexp_wrap_compile(regexp_wrap self, const char* text) {
	sre_uint_t ncaps = 0;
	int flags = SRE_REGEX_CASELESS;
	sre_int_t err_offset = -1;
	sre_regex_t* re;
	sre_pool_t* ppool = sre_create_pool(1024);
	re = sre_regex_parse(ppool, (sre_char *)text, &ncaps,
		flags, &err_offset);
	if (!re) {
		sre_destroy_pool(ppool);
		return err_offset;
	}
	else {
		sre_reset_pool(self->pool);
		self->prog = sre_regex_compile(self->pool, re);
		self->multi = 0;
		sre_destroy_pool(ppool);
		return 0;
	}
}

int regexp_wrap_compile_multi(regexp_wrap self, char** text, int len) {
	sre_uint_t ncaps = 0;
	int* flags;
	sre_int_t err_offset = -1;
	sre_int_t err_id = 0;
	sre_regex_t* re;
	sre_pool_t* ppool = sre_create_pool(1024*4);
	flags = sre_pcalloc(ppool, len * sizeof(int));
	for (int i = 0; i < len; i++)
		flags[i] = SRE_REGEX_CASELESS;
	re = sre_regex_parse_multi(ppool, (sre_char **)text, len, &ncaps,
		flags, &err_offset, &err_id);
	if (!re) {
		sre_destroy_pool(ppool);
		return err_offset;
	}
	else {
		sre_reset_pool(self->pool);
		self->prog = sre_regex_compile(self->pool, re);
		self->multi = len;
		sre_destroy_pool(ppool);
		return 0;
	}
}

int utf_8_width(const char *str) {
	const unsigned char* data = (const unsigned char*)str;
	unsigned char ch = data[0];
	if (ch == 0) {
		return 0;
	}else if (ch < 0x80) {
		return 1;
	} else if (ch < 0xc0) {
		return 1;
	}
	else if (ch < 0xe0) {
		return 2;
	} if (ch < 0xf0) {
		return 3;
	}
	else {
		return 4;
	}

}

int utf_8_index(const char *str, int len, int* idx) {
	const unsigned char* data = (const unsigned char*)str;
	int pos = 0;
	int count = 0;
	while (pos < len) {
		unsigned char ch = data[pos];
		if (ch == 0) {
			idx[pos] = count++;
			break;
		}
		else if (ch < 0x80) {
			idx[pos++] = count++;
		}
		else if (ch < 0xc0) {
			//invalid
			return -1;
			//idx[pos++] = count++;
		}
		else if (ch < 0xe0) {
			idx[pos++] = count;
			idx[pos++] = count++;
		}
		else if (ch < 0xf0) {
			idx[pos++] = count;
			idx[pos++] = count;
			idx[pos++] = count++;
		}
		else {
			idx[pos++] = count;
			idx[pos++] = count;
			idx[pos++] = count;
			idx[pos++] = count++;
		}
	}
	if (pos == len) {
		idx[pos] = count++;
	}
	return 0;
}

void regexp_wrap_save_instruction(FILE *fp, sre_instruction_t *pc,
	sre_instruction_t *start) {
	sre_uint_t              i;
	sre_vm_range_t         *range;

	unsigned char byte;
	uint32  data32;

	byte = pc->opcode;
	fwrite(&byte, sizeof(byte), 1, fp);
	switch (pc->opcode) {
	case SRE_OPCODE_SPLIT:
		data32 = (int)(pc->x - start);
		data32 = htonl(data32);
		fwrite(&data32, sizeof(data32), 1, fp);
		data32 = (int)(pc->y - start);
		data32 = htonl(data32);
		fwrite(&data32, sizeof(data32), 1, fp);
		break;
	case SRE_OPCODE_JMP:
		data32 = (int)(pc->x - start);
		data32 = htonl(data32);
		fwrite(&data32, sizeof(data32), 1, fp);
		break;
	case SRE_OPCODE_CHAR:
		byte = pc->v.ch;
		fwrite(&byte, sizeof(byte), 1, fp);
		break;
	case SRE_OPCODE_IN:
		data32 = htonl(pc->v.ranges->count);
		fwrite(&data32, sizeof(data32), 1, fp);
		for (i = 0; i < pc->v.ranges->count; i++) {
			range = &pc->v.ranges->head[i];
			byte = range->from;
			fwrite(&byte, sizeof(byte), 1, fp);
			byte = range->to;
			fwrite(&byte, sizeof(byte), 1, fp);
		}
		break;
	case SRE_OPCODE_NOTIN:
		data32 = htonl(pc->v.ranges->count);
		fwrite(&data32, sizeof(data32), 1, fp);
		for (i = 0; i < pc->v.ranges->count; i++) {
			range = &pc->v.ranges->head[i];
			byte = range->from;
			fwrite(&byte, sizeof(byte), 1, fp);
			byte = range->to;
			fwrite(&byte, sizeof(byte), 1, fp);
		}
		break;

	case SRE_OPCODE_ANY:
		break;
	case SRE_OPCODE_MATCH:
		data32 = htonl(pc->v.regex_id);
		fwrite(&data32, sizeof(data32), 1, fp);
		break;
	case SRE_OPCODE_SAVE:
		data32 = htonl(pc->v.group);
		fwrite(&data32, sizeof(data32), 1, fp);
		break;
	case SRE_OPCODE_ASSERT:
		data32 = htonl(pc->v.assertion);
		fwrite(&data32, sizeof(data32), 1, fp);
		break;
	default:
		break;
	}
}

void regexp_wrap_load_instruction(sre_pool_t *pool, FILE* fp, sre_instruction_t *pc,
	sre_instruction_t *start) {
	sre_uint_t              i;
	sre_vm_range_t         *range;

	unsigned char byte;
	uint32  data32;

	fread(&byte, sizeof(byte), 1, fp);
	pc->opcode = byte;
	switch(pc->opcode) {
	case SRE_OPCODE_SPLIT:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->x = start + data32;
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->y = start + data32;
		break;
	case SRE_OPCODE_JMP:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->x = start + data32;
		break;
	case SRE_OPCODE_CHAR:
		fread(&byte, sizeof(byte), 1, fp);
		pc->v.ch = byte;
		break;
	case SRE_OPCODE_IN:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->v.ranges = sre_pnalloc(pool, sizeof(sre_vm_ranges_t));
		pc->v.ranges->count = data32;
		pc->v.ranges->head = sre_pnalloc(pool, data32 * sizeof(sre_vm_range_t));
		for (i = 0; i < pc->v.ranges->count; i++) {
			range = &pc->v.ranges->head[i];
			fread(&byte, sizeof(byte), 1, fp);
			range->from = byte;
			fread(&byte, sizeof(byte), 1, fp);
			range->to = byte;
		}
		break;
	case SRE_OPCODE_NOTIN:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->v.ranges = sre_pnalloc(pool, sizeof(sre_vm_ranges_t));
		pc->v.ranges->count = data32;
		pc->v.ranges->head = sre_pnalloc(pool, data32 * sizeof(sre_vm_range_t));
		for (i = 0; i < pc->v.ranges->count; i++) {
			range = &pc->v.ranges->head[i];
			fread(&byte, sizeof(byte), 1, fp);
			range->from = byte;
			fread(&byte, sizeof(byte), 1, fp);
			range->to = byte;
		}
		break;

	case SRE_OPCODE_ANY:
		break;
	case SRE_OPCODE_MATCH:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->v.regex_id = data32;
		break;
	case SRE_OPCODE_SAVE:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->v.group = data32;
		break;
	case SRE_OPCODE_ASSERT:
		fread(&data32, sizeof(data32), 1, fp);
		data32 = ntohl(data32);
		pc->v.assertion = data32;
		break;
	default:
		break;
	}
}

void regexp_wrap_save_program(sre_program_t *prog, int multi, FILE* fp) {
	uint32 data;
	sre_instruction_t      *pc, *start, *end;
	
	data = htonl(multi);
	fwrite(&data, sizeof(data), 1, fp);

	//save prog

	//为了方便读出来， 先写两个变长数据
	data = htonl(prog->nregexes);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->len);
	fwrite(&data, sizeof(data), 1, fp);

	//save other

	data = htonl(prog->tag);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->uniq_threads);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->dup_threads);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->lookahead_asserts);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->nullable);
	fwrite(&data, sizeof(data), 1, fp);

	// not write leading_bytes

	data = htonl(prog->leading_byte);
	fwrite(&data, sizeof(data), 1, fp);

	data = htonl(prog->ovecsize);
	fwrite(&data, sizeof(data), 1, fp);

	for (int i = 0; i < prog->nregexes; i++) {
		data = htonl(prog->multi_ncaps[i]);
		fwrite(&data, sizeof(data), 1, fp);
	}

	//save inst

	start = prog->start;
	end = prog->start + prog->len;

	for (pc = start; pc < end; pc++) {
		regexp_wrap_save_instruction(fp, pc, start);
	}
}

int regexp_wrap_save(regexp_wrap self, PyObject* file)
{
	FILE* fp;
	int fileno = PyObject_AsFileDescriptor(file);
	if (fileno == -1)
		return -1;
	fileno = dup(fileno);
	fp = fdopen(fileno, "wb");
	regexp_wrap_save_program(self->prog, self->multi, fp);
	fclose(fp);
	return 0;
}

extern sre_int_t
sre_program_get_leading_bytes(sre_pool_t *pool, sre_program_t *prog,
	sre_chain_t **res);
int regexp_wrap_load_program(regexp_wrap self, FILE* fp)
{
	sre_program_t *prog;
	sre_instruction_t *pc;

	uint32 data;
	int nregexes;
	int proglen;

	int alloc_size;
	int multi_ncaps_size;

	if (fread(&data, sizeof(data), 1, fp) != 1) {
		return -1;
	}
	self->multi = ntohl(data);

	if (fread(&data, sizeof(data), 1, fp) != 1) {
		return -2;
	}
	nregexes = ntohl(data);

	if (fread(&data, sizeof(data), 1, fp) != 1) {
		return -3;
	}
	proglen = ntohl(data);


	multi_ncaps_size = (nregexes - 1) * sizeof(sre_uint_t);

	alloc_size = sizeof(sre_program_t) + multi_ncaps_size
		+ proglen * sizeof(sre_instruction_t);

	sre_reset_pool(self->pool);
	self->prog = prog = sre_pnalloc(self->pool, alloc_size);
	prog->nregexes = nregexes;
	prog->len = proglen;

	//load other

	fread(&data, sizeof(data), 1, fp);
	prog->tag = ntohl(data);

	fread(&data, sizeof(data), 1, fp);
	prog->uniq_threads = ntohl(data);

	fread(&data, sizeof(data), 1, fp);
	prog->dup_threads = ntohl(data);

	fread(&data, sizeof(data), 1, fp);
	prog->lookahead_asserts = ntohl(data);


	fread(&data, sizeof(data), 1, fp);
	prog->nullable = ntohl(data);

	// not write leading_bytes
	//TODO:

	fread(&data, sizeof(data), 1, fp);
	prog->leading_byte = ntohl(data);

	fread(&data, sizeof(data), 1, fp);
	prog->ovecsize = ntohl(data);

	for (int i = 0; i < prog->nregexes; i++) {
		fread(&data, sizeof(data), 1, fp);
		prog->multi_ncaps[i] = ntohl(data);
	}
	
	pc = prog->start = (sre_instruction_t *)(((char*)prog) + sizeof(sre_program_t)
		+ multi_ncaps_size);
	for (int i = 0; i < proglen; i++) {
		regexp_wrap_load_instruction(self->pool, fp, pc + i, pc);
	}
	prog->leading_bytes = NULL;
	prog->leading_byte = -1;
	sre_program_get_leading_bytes(self->pool, prog, &prog->leading_bytes);
	if (prog->leading_bytes && prog->leading_bytes->next == NULL) {
		pc = prog->leading_bytes->data;
		if (pc->opcode == SRE_OPCODE_CHAR) {
			prog->leading_byte = pc->v.ch;
		}
	}
	return 0;
}

int regexp_wrap_load(regexp_wrap self, PyObject* file)
{
	FILE* fp;
	int fileno = PyObject_AsFileDescriptor(file);
	if (fileno == -1)
		return -1;
	fileno = dup(fileno);
	fileno = dup(fileno);
	fp = fdopen(fileno, "rb");
	regexp_wrap_load_program(self, fp);
	fseek(fp, ftell(fp), SEEK_SET);
	fclose(fp);
	return 0;
}

int regexp_wrap_exec(regexp_wrap self, const char* text, int len, regexp_result_callback callback, void* userData) {
	sre_vm_pike_ctx_t* pctx;
	sre_int_t ovector[2];
	sre_int_t rc = 0;
	int* index = 0;
	int off = 0;
	if (self == 0 || self->prog == 0) {
		return -1;
	}
	sre_pool_t* pool = sre_create_pool(1024);

	pctx = sre_vm_pike_create_ctx(pool, self->prog, ovector, 2);
	if (!pctx) {
		sre_destroy_pool(pool);
		return -2;
	}
	index = sre_palloc(pool, (len+1) * sizeof(int));
	if (utf_8_index(text, len, index) < 0) {
		sre_destroy_pool(pool);
		return -3;
	}
	if (self->multi == 0) {
		//single reg
		regexp_result result;
		while (len > 0 && (rc = sre_vm_pike_exec(pctx, text, len, 1 /* eof */, NULL, NULL, NULL)) == 0) {
			int next = ovector[1];
			if (next < 1) {
				next = 1;
			}
			result.start = index[off + ovector[0]];
			result.end = index[off + ovector[1]];
			result.data = rc;
			callback(userData, &result);

			off += next;
			text += next;
			len -= next;
			sre_vm_pike_reset_ctx(pctx);
		}
	}
	else {
		//这个比较复杂
		//先排序一下看看
		struct regexp_multi_context match_context;
		match_context.pool = self->pool;
		match_context.result = 0;
		match_context.result_size = 0;
		match_context.result_capacity = 0;
		regexp_result* result;
		while (len > 0 && (rc = sre_vm_pike_exec(pctx, text, len, 1 /* eof */, NULL, appendResult, &match_context)) >= 0) {
			int nextStart = len;
			////
			for (int i = 0; i < match_context.result_size; i++) {
				result = match_context.result + i;
				if (nextStart > result->start)
					nextStart = result->start;
				result->start = index[off + result->start];
				result->end = index[off + result->end];
				callback(userData, result);
			}
			nextStart+= utf_8_width(text + nextStart);

			match_context.result_size = 0;
			off += nextStart;
			text += nextStart;
			len -= nextStart;
			sre_vm_pike_reset_ctx(pctx);
		}
	}
	sre_destroy_pool(pool);
	return rc;
}