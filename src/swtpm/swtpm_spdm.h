#ifndef SWTPM_SPDM_H
#define SWTPM_SPDM_H

#include <stdbool.h>
#include <stdint.h>

#include <hal/base.h>
#include <hal/library/memlib.h>
#include <industry_standard/spdm.h>
#include <industry_standard/spdm_secured_message.h>
#include <internal/libspdm_common_lib.h>
#include <library/spdm_common_lib.h>
#include <library/spdm_requester_lib.h>
#include <library/spdm_responder_lib.h>
#include <library/spdm_transport_mctp_lib.h>

#include "mainloop.h"
#include "src/swtpm_setup/swtpm.h"
#include "swtpm_io.h"

/* Maximum size of a large SPDM message.
 * If chunk is unsupported, it must be same as DATA_TRANSFER_SIZE.
 * If chunk is supported, it must be larger than DATA_TRANSFER_SIZE.
 * It matches MaxSPDMmsgSize in SPDM specification. */
#ifndef LIBSPDM_MAX_SPDM_MSG_SIZE
#define LIBSPDM_MAX_SPDM_MSG_SIZE 0x2200
#endif

#define LIBSPDM_TRANSPORT_HEADER_SIZE 64
#define LIBSPDM_TRANSPORT_TAIL_SIZE 64

/* define common LIBSPDM_TRANSPORT_ADDITIONAL_SIZE. It should be the biggest one. */
#define LIBSPDM_TRANSPORT_ADDITIONAL_SIZE \
    (LIBSPDM_TRANSPORT_HEADER_SIZE + LIBSPDM_TRANSPORT_TAIL_SIZE)

#ifndef LIBSPDM_SENDER_BUFFER_SIZE
#define LIBSPDM_SENDER_BUFFER_SIZE (0x1100 + \
                                    LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif
#ifndef LIBSPDM_RECEIVER_BUFFER_SIZE
#define LIBSPDM_RECEIVER_BUFFER_SIZE (0x1200 + \
                                      LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif

/* Maximum size of a single SPDM message.
 * It matches DataTransferSize in SPDM specification. */
#define LIBSPDM_SENDER_DATA_TRANSFER_SIZE (LIBSPDM_SENDER_BUFFER_SIZE - \
                                           LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#define LIBSPDM_RECEIVER_DATA_TRANSFER_SIZE (LIBSPDM_RECEIVER_BUFFER_SIZE - \
                                             LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#define LIBSPDM_DATA_TRANSFER_SIZE LIBSPDM_RECEIVER_DATA_TRANSFER_SIZE

#if (LIBSPDM_SENDER_BUFFER_SIZE > LIBSPDM_RECEIVER_BUFFER_SIZE)
#define LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE LIBSPDM_SENDER_BUFFER_SIZE
#else
#define LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE LIBSPDM_RECEIVER_BUFFER_SIZE
#endif

typedef struct spdm_connection spdm_connection;
typedef struct spdm_connection_node spdm_connection_node;

struct spdm_connection {
  void *context;
  TPM_CONNECTION_FD *connection_fd;

  uint8_t *buffer;
  bool acquired_buffer;
};

struct spdm_connection_node {
    spdm_connection *spdm_connection;
    spdm_connection_node *next_connection;
};

extern spdm_connection_node *g_spdm_connection_list;

spdm_connection *get_spdm_connection_from_context(void *context);

void *init_spdm (TPM_CONNECTION_FD *connection_fd);
libspdm_return_t swtpm_spdm_send_message(void *context, size_t response_size,
                                        const void *response, uint64_t timeout);
libspdm_return_t swtpm_spdm_receive_message(void *context, size_t *request_size,
                                           void **request, uint64_t timeout);
libspdm_return_t swtpm_spdm_acquire_buffer(void *context, void **msg_buf_ptr);
void swtpm_spdm_release_buffer(void *context, const void *msg_buf_ptr);

#endif
