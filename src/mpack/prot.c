#include <stddef.h>

#define DILL_DISABLE_RAW_NAMES
#include <libdillimpl.h>

#include "prot.h"
#include "errtrace.h"

#define cont(ptr, type, member) \
	((type *)(((char *) ptr) - offsetof(type, member)))

static const size_t INITIAL_BUFFER_SIZE = 250;



typedef struct mp_sock {
	struct dill_hvfs hvfs;
// 	struct dill_msock_vfs mvfs;
	int u;
	int64_t timeout;

	cw_pack_context pack_ctx;
	cw_unpack_context unpack_ctx;

	size_t unpack_buffer_len;
} mp_sock;

static const char mp_sock_type_placeholder = 0;
static const char mp_pack_ctx_type = 0;
static const char mp_unpack_ctx_type = 0;

static void *mp_hquery(struct dill_hvfs *vfs, const void *type) {
	mp_sock *self = cont(vfs, mp_sock, hvfs);

	if (type == &mp_sock_type_placeholder) return self;
// 	if (type == dill_msock_type) return &self->mvfs;
	if (type == &mp_pack_ctx_type) return &self->pack_ctx;
	if (type == &mp_unpack_ctx_type) return &self->unpack_ctx;

	errno = ENOTSUP;
	return NULL;
}

static void mp_hclose(struct dill_hvfs *vfs) {
	mp_sock *self = cont(vfs, mp_sock, hvfs);

	dill_hclose(self->u);
	free(self->unpack_ctx.start);
	free(self->pack_ctx.start);
	free(self);
}

static int64_t deadline(int64_t timeout) {
	return timeout <= 0 ? timeout : dill_now() + timeout;
}

static int handle_socket_pack_overflow(cw_pack_context *ctx, unsigned long more) {
	mp_sock *self = cont(ctx, mp_sock, pack_ctx);

	if (ctx->current > ctx->start) {
		int rc = dill_bsend(self->u, ctx->start, ctx->current - ctx->start, deadline(self->timeout));
		if (rc != 0) {
			ctx->err_no = errno;
			return Tv(CWP_RC_ERROR_IN_HANDLER);
		}
	}

	size_t buffer_length = ctx->end - ctx->start;
	if (buffer_length < more) {
		while (buffer_length < more)
			buffer_length = 2 * buffer_length;

		void *new_buffer = malloc(buffer_length);
		if (!new_buffer) return Tv(CWP_RC_BUFFER_OVERFLOW);

		free(ctx->start);
		ctx->start = (uint8_t *)new_buffer;
		ctx->end = ctx->start + buffer_length;
	}
	ctx->current = ctx->start;

	return CWP_RC_OK;
}

static int do_read(cw_unpack_context *ctx, int s, unsigned long count, int64_t deadline) {
	int rc = dill_brecv(s, ctx->end, count, deadline);
	if (rc != 0) {
		ctx->err_no = errno;
		return Tv(errno == EPIPE ? CWP_RC_END_OF_INPUT : CWP_RC_ERROR_IN_HANDLER);
	}

	ctx->end = ctx->current + count;
	return CWP_RC_OK;
}

static int handle_socket_unpack_underflow(cw_unpack_context *ctx, unsigned long more) {
	mp_sock *self = cont(ctx, mp_sock, unpack_ctx);

	uint8_t *nxt_end = ctx->current + more;
	if (nxt_end < ctx->start + self->unpack_buffer_len) {
		// enough allocated space, read without moving
		return do_read(ctx, self->u, nxt_end - ctx->end, deadline(self->timeout));
	}

	// move unread portion to the start
	ptrdiff_t remains = ctx->end - ctx->current;
	memmove(ctx->start, ctx->current, remains);

	if (self->unpack_buffer_len < more) {
		// realloc
		while (self->unpack_buffer_len < more) {
			self->unpack_buffer_len = 2 * self->unpack_buffer_len;
		}

		void *new_buffer = realloc(ctx->start, self->unpack_buffer_len);
		if (!new_buffer) return Tv(CWP_RC_BUFFER_UNDERFLOW);

		ctx->start = (uint8_t *)new_buffer;
	}

	ctx->current = ctx->start;
	ctx->end = ctx->current + remains;

	return do_read(ctx, self->u, more - remains, deadline(self->timeout));
}

int mp_attach(int s, mp_flags flags, int64_t timeout) {
	mp_sock *self = calloc(1, sizeof(mp_sock));
	if (!self) {
		errno = ENOMEM;
		Tf(error);
	}

	self->hvfs.query = mp_hquery;
	self->hvfs.close = mp_hclose;
// 	self->mvfs.msendl = NULL;

	self->u = dill_hown(s);
	self->timeout = timeout;

	int h = dill_hmake(&self->hvfs);
	if (h < 0) Tf(error);

	if (!(flags & mpf_NoEncoding)) {
		void *buffer = malloc(INITIAL_BUFFER_SIZE);
		if (buffer == NULL) Tf(error);

		int rc = cw_pack_context_init(&self->pack_ctx, buffer, INITIAL_BUFFER_SIZE, handle_socket_pack_overflow);
		if (rc != CWP_RC_OK) Tf(error);
	}

	if (!(flags & mpf_NoDecoding)) {
		void *buffer = malloc(INITIAL_BUFFER_SIZE);
		if (buffer == NULL) Tf(error);

		self->unpack_buffer_len = INITIAL_BUFFER_SIZE;
		int rc = cw_unpack_context_init(&self->unpack_ctx, buffer, 0, handle_socket_unpack_underflow);
		if (rc != CWP_RC_OK) Tf(error);
	}

	return h;

error:
	free(self->unpack_ctx.start);
	free(self->pack_ctx.start);
	free(self);
	dill_hclose(s);
	return -1;
}

cw_unpack_context *mp_unpack_ctx(int s) {
	mp_sock *self = dill_hquery(s, &mp_unpack_ctx_type);
	if (!self) return Tv(NULL);

	return &self->unpack_ctx;
}
