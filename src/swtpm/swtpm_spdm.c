#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <libtpms/tpm_library.h>

#include "swtpm_spdm.h"
#include "hal/library/debuglib.h"
#include "industry_standard/spdm.h"
#include "internal/libspdm_common_lib.h"
#include "library/spdm_common_lib.h"
#include "src/swtpm_setup/swtpm.h"
#include "swtpm_io.h"

spdm_connection_node null_connection = { NULL, NULL };
spdm_connection_node *g_spdm_connection_list = &null_connection;

static
spdm_connection_node*
create_spdm_connection_node(spdm_connection *connection)
{
    spdm_connection_node *node = (spdm_connection_node *)malloc(sizeof(spdm_connection_node));

    node->spdm_connection = connection;
    node->next_connection = NULL;

    return node;
}

static
bool record_spdm_connection(spdm_connection *connection)
{
    spdm_connection_node *node = create_spdm_connection_node(connection);
    spdm_connection_node *curr = g_spdm_connection_list;

    if (curr->spdm_connection == NULL) {
        curr = node;

        return true;
    }

    while (curr != NULL) {
        if (curr->next_connection == NULL) {
            curr->next_connection = node;

            return true;
        }

        curr = curr->next_connection;
    }

    return false;
}

// static
// bool delete_spdm_connection(spdm_connection *connection)
// {
//     spdm_connection_node *prev = g_spdm_connection_list;
//     spdm_connection_node *curr = prev->next_connection;
//
//     if (prev->spdm_connection == connection) {
//         g_spdm_connection_list = curr;
//         free(prev->spdm_connection);
//
//         return true;
//     }
//
//     while (curr != NULL) {
//         if (curr->spdm_connection == connection) {
//             prev->next_connection = curr->next_connection;
//             free(curr->spdm_connection);
//
//             return true;
//         }
//
//         prev = curr;
//         curr = curr->next_connection;
//     }
//
//     return false;
// }

spdm_connection*
get_spdm_connection_from_context(void *context)
{
    spdm_connection_node *curr = g_spdm_connection_list;

    while (curr != NULL) {
        if (curr->spdm_connection->context == context) {
            return curr->spdm_connection;
        }

        curr = curr->next_connection;
    }

    return NULL;
}

void libspdm_dump_hex_str(const uint8_t *buffer, size_t buffer_size)
{
    size_t index;

    for (index = 0; index < buffer_size; index++) {
        printf("%02x", buffer[index]);
    }
}

bool libspdm_read_input_file(const char *file_name, void **file_data,
                             size_t *file_size)
{
    FILE *fp_in;
    size_t temp_result;

    if ((fp_in = fopen(file_name, "rb")) == NULL) {
        printf("Unable to open file %s\n", file_name);
        *file_data = NULL;
        return false;
    }

    fseek(fp_in, 0, SEEK_END);
    *file_size = ftell(fp_in);
    if (*file_size == -1) {
        printf("Unable to get the file size %s\n", file_name);
        *file_data = NULL;
        fclose(fp_in);
        return false;
    }

    *file_data = (void *)malloc(*file_size);
    if (NULL == *file_data) {
        printf("No sufficient memory to allocate %s\n", file_name);
        fclose(fp_in);
        return false;
    }

    fseek(fp_in, 0, SEEK_SET);
    temp_result = fread(*file_data, 1, *file_size, fp_in);
    if (temp_result != *file_size) {
        printf("Read input file error %s", file_name);
        free((void *)*file_data);
        fclose(fp_in);
        return false;
    }

    fclose(fp_in);

    return true;
}

bool libspdm_write_output_file(const char *file_name, const void *file_data,
                               size_t file_size)
{
    FILE *fp_out;

    if ((fp_out = fopen(file_name, "w+b")) == NULL) {
        printf("Unable to open file %s\n", file_name);
        return false;
    }

    if (file_size != 0) {
        if ((fwrite(file_data, 1, file_size, fp_out)) != file_size) {
            printf("Write output file error %s\n", file_name);
            fclose(fp_out);
            return false;
        }
    }

    fclose(fp_out);

    return true;
}

libspdm_return_t swtpm_spdm_send_message(void *context, size_t response_size,
                                        const void *response, uint64_t timeout)
{
    // spdm_connection *connection = get_spdm_connection_from_context(context);
    // TPM_CONNECTION_FD *fd = connection->connection_fd;

    return LIBSPDM_STATUS_SUCCESS;
}

libspdm_return_t swtpm_spdm_receive_message(void *context, size_t *request_size,
                                           void **request, uint64_t timeout)
{
    // spdm_connection *connection = get_spdm_connection_from_context(context);
    // TPM_CONNECTION_FD *fd = connection->connection_fd;

    return LIBSPDM_STATUS_SUCCESS;
}

libspdm_return_t swtpm_spdm_acquire_buffer(void *context, void **msg_buf_ptr)
{
    spdm_connection *connection = get_spdm_connection_from_context(context);

    LIBSPDM_ASSERT(!connection->acquired_buffer);
    connection->buffer = malloc(LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE);
    connection->acquired_buffer = true;

    return LIBSPDM_STATUS_SUCCESS;
}

void swtpm_spdm_release_buffer(void *context, const void *msg_buf_ptr)
{
    spdm_connection *connection = get_spdm_connection_from_context(context);

    LIBSPDM_ASSERT(connection->acquired_buffer);
    free(connection->buffer);
    connection->acquired_buffer = false;
}

void *init_spdm (TPM_CONNECTION_FD *connection_fd)
{
    uint8_t data8;
    uint16_t data16;
    uint32_t data32;
    libspdm_data_parameter_t parameter;
    spdm_version_number_t version;
    void *context;
    void *scratch_buffer;
    void *requester_cert_chain_buffer;
    size_t scratch_buffer_size;
    spdm_connection *connection;

    connection = malloc(sizeof(spdm_connection));
    if (!connection)
        return NULL;


    connection->connection_fd = connection_fd;

    context = malloc(libspdm_get_context_size());
    libspdm_init_context(context);

    connection->context = context;

    libspdm_register_device_io_func(context,
                                    swtpm_spdm_send_message,
                                    swtpm_spdm_receive_message);

    libspdm_register_transport_layer_func(
        context,
        LIBSPDM_MAX_SPDM_MSG_SIZE,
        LIBSPDM_MCTP_TRANSPORT_HEADER_SIZE,
        LIBSPDM_MCTP_TRANSPORT_TAIL_SIZE,
        libspdm_transport_mctp_encode_message,
        libspdm_transport_mctp_decode_message
        );

    libspdm_register_device_buffer_func(context,
                                        LIBSPDM_SENDER_BUFFER_SIZE,
                                        LIBSPDM_RECEIVER_BUFFER_SIZE, 
                                        swtpm_spdm_acquire_buffer,
                                        swtpm_spdm_release_buffer, 
                                        swtpm_spdm_acquire_buffer, 
                                        swtpm_spdm_release_buffer);
    scratch_buffer_size = libspdm_get_sizeof_required_scratch_buffer(context);
    scratch_buffer = malloc(scratch_buffer_size);
    libspdm_set_scratch_buffer(context, 
                               scratch_buffer, 
                               scratch_buffer_size);

    requester_cert_chain_buffer = malloc(SPDM_MAX_CERTIFICATE_CHAIN_SIZE);
    libspdm_register_cert_chain_buffer(context,
                                       requester_cert_chain_buffer,
                                       SPDM_MAX_CERTIFICATE_CHAIN_SIZE);

    if (!libspdm_check_context(context)) {
        return NULL;
    }

    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    version = SPDM_MESSAGE_VERSION_13 << SPDM_VERSION_NUMBER_SHIFT_BIT;
    libspdm_set_data(context, LIBSPDM_DATA_SPDM_VERSION,
                     &parameter, &version, sizeof(version));

    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
    version = (SECURED_SPDM_VERSION_12 | SECURED_SPDM_VERSION_11) << SPDM_VERSION_NUMBER_SHIFT_BIT;
    libspdm_set_data(context, LIBSPDM_DATA_SECURED_MESSAGE_VERSION,
                     &parameter, &version, sizeof(version));

    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;

    data8 = 0;
    libspdm_set_data(context, LIBSPDM_DATA_CAPABILITY_CT_EXPONENT,
                     &parameter, &data8, sizeof(data8));

    data32 = SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_13_MASK;
    libspdm_set_data(context, LIBSPDM_DATA_CAPABILITY_FLAGS,
                     &parameter, &data32, sizeof(data32));

    data8 = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
    libspdm_set_data(context, LIBSPDM_DATA_MEASUREMENT_SPEC,
                     &parameter, &data8, sizeof(data8));

    data32 = SPDM_ALGORITHMS_MEASUREMENT_HASH_ALGO_TPM_ALG_SHA_256;
    libspdm_set_data(context, LIBSPDM_DATA_MEASUREMENT_HASH_ALGO,
                     &parameter, &data32, sizeof(data32));

    data32 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_2048;
    libspdm_set_data(context, LIBSPDM_DATA_BASE_ASYM_ALGO,
                     &parameter, &data32, sizeof(data32));

    data32 = SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256;
    libspdm_set_data(context, LIBSPDM_DATA_BASE_HASH_ALGO,
                     &parameter, &data32, sizeof(data32));

    data16 = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1;
    libspdm_set_data(context, LIBSPDM_DATA_DHE_NAME_GROUP,
                     &parameter, &data16, sizeof(data16));

    data16 = SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
    libspdm_set_data(context, LIBSPDM_DATA_AEAD_CIPHER_SUITE, 
                     &parameter, &data16, sizeof(data16));

    data16 = SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_2048;
    libspdm_set_data(context, LIBSPDM_DATA_REQ_BASE_ASYM_ALG,
                     &parameter, &data16, sizeof(data16));

    data16 = SPDM_ALGORITHMS_KEY_SCHEDULE_HMAC_HASH;
    libspdm_set_data(context, LIBSPDM_DATA_KEY_SCHEDULE,
                     &parameter, &data16, sizeof(data16));

    data8 = SPDM_ALGORITHMS_OPAQUE_DATA_FORMAT_1 | SPDM_ALGORITHMS_MULTI_KEY_CONN;
    libspdm_set_data(context, LIBSPDM_DATA_OTHER_PARAMS_SUPPORT,
                     &parameter, &data8, sizeof(data8));

    data8 = SPDM_MEL_SPECIFICATION_DMTF;
    libspdm_set_data(context, LIBSPDM_DATA_MEL_SPEC,
                     &parameter, &data8, sizeof(data8));

    data8 = 0xF0;
    libspdm_set_data(context, LIBSPDM_DATA_HEARTBEAT_PERIOD,
                     &parameter, &data8, sizeof(data8));

    record_spdm_connection(connection);

    return context;
}

