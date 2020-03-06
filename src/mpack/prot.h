#pragma once

#include <stdint.h>
#include <cwpack/cwpack.h>

typedef enum mp_flags {
	mpf_None = 0,
	mpf_NoDecoding = 1,
	mpf_NoEncoding = 2,
} mp_flags;

/**
 * mp_attach creates a MessagePack based protocol
 * on top of an underlying bytestream socket
 */
int mp_attach(int s, mp_flags flags, int64_t timeout);


/**
 * mp_unpack_ctx returns a pointer to a cw_unpack_context
 * usable for cw_* functions
 */
cw_unpack_context *mp_unpack_ctx(int s);

/**
 * mp_pack_ctx returns a pointer to a cw_pack_context
 * usable for cw_* functions
 */
cw_pack_context *mp_pack_ctx(int s);

/**
 * mp_flush makes sure all msgPack objects have been
 * pushed to the socket
 */
void mp_flush(int s);
