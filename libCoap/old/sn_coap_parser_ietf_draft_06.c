/**
 *\file sn_coap_parser_ietf_draft_06.c
 *
 * \brief CoAP Header parser
 *
 * Functionality: Parses CoAP Header
 *
 *  Created on: Jun 30, 2011
 *      Author: pekka_ext
 *
 * \note Supports draft-ietf-core-coap-06
 */

/* * * * * * * * * * * * * * */
/* * * * INCLUDE FILES * * * */
/* * * * * * * * * * * * * * */

#include <string.h> /* For memset() and memcpy() */

#include "pl_types.h"
#include "sn_nsdl.h"
#include "sn_coap_header.h"
#include "sn_coap_protocol.h"
#include "sn_coap_header_ietf_draft_06.h"
#include "sn_coap_protocol_ietf_draft_06.h"

/* * * * * * * * * * * * * * * * * * * * */
/* * * * LOCAL FUNCTION PROTOTYPES * * * */
/* * * * * * * * * * * * * * * * * * * * */

static void     sn_coap_parser_header_parse(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr, coap_version_e *coap_version_ptr);
static uint8_t  sn_coap_parser_options_parse(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr);
static int8_t   sn_coap_parser_options_parse_multiple_options(uint8_t **packet_data_pptr, uint8_t options_count_left, uint8_t *previous_option_number_ptr, uint8_t **dst_ptr, uint16_t *dst_len_ptr, sn_coap_option_numbers_e option);
static uint16_t sn_coap_parser_options_count_needed_memory_multiple_option(uint8_t *packet_data_ptr, uint8_t options_count_left, uint8_t previous_option_number, sn_coap_option_numbers_e option);
static void     sn_coap_parser_payload_parse(uint16_t packet_data_len, uint8_t *packet_data_ptr, uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr);

/* * * * * * * * * * * * * * * * * */
/* * * * GLOBAL DECLARATIONS * * * */
/* * * * * * * * * * * * * * * * * */

SN_MEM_ATTR_COAP_PARSER_DECL static uint8_t *base_packet_data_ptr; /* Base (= original) source Packet data pointer value */

/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC sn_coap_hdr_s *sn_coap_parser(uint16_t packet_data_len, uint8_t *packet_data_ptr, coap_version_e *coap_version_ptr, uint16_t *msg_id_ptr)
 *
 * \brief Parses CoAP message from given Packet data
 *
 * \param packet_data_len is length of given Packet data to be parsed to CoAP message
 *
 * \param *packet_data_ptr is source for Packet data to be parsed to CoAP message
 *
 * \param *coap_version_ptr is destination for parsed CoAP specification version
 *
 * \param *msg_id_ptr is destination for parsed Message ID
 *
 * \return Return value is pointer to parsed CoAP message.\n
 *         In following failure cases NULL is returned:\n
 *          -Failure in given pointer (= NULL)\n
 *          -Failure in memory allocation (malloc() returns NULL)
 */
SN_MEM_ATTR_COAP_PARSER_FUNC
sn_coap_hdr_s *sn_coap_parser(uint16_t packet_data_len,
                              uint8_t *packet_data_ptr,
                              coap_version_e *coap_version_ptr)
{
    uint8_t       *data_temp_ptr                    = packet_data_ptr;
    sn_coap_hdr_s *parsed_and_returned_coap_msg_ptr = NULL;
    uint8_t        ret_status                       = 0;

    /* * * * * * * * * * * * * * * * * */
    /* * * * Check given pointer * * * */
    /* * * * * * * * * * * * * * * * * */

    if (packet_data_ptr == NULL)
    {
        return NULL;
    }

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    /* * * * Store base (= original) source Packet data pointer for later usage  * * * */
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

    base_packet_data_ptr = packet_data_ptr;

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    /* * * * Allocate memory for parsed and returned CoAP message  * * * */
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

    parsed_and_returned_coap_msg_ptr = sn_coap_malloc(sizeof(sn_coap_hdr_s));

    if (parsed_and_returned_coap_msg_ptr == NULL)
    {
        return NULL;
    }

    /* Initialize allocated memory with with zero values */
    memset(parsed_and_returned_coap_msg_ptr, 0x00, sizeof(sn_coap_hdr_s));

    /* * * * * * * * * * * * * * * */
    /* * * * Header parsing  * * * */
    /* * * * * * * * * * * * * * * */

    sn_coap_parser_header_parse(&data_temp_ptr, parsed_and_returned_coap_msg_ptr, coap_version_ptr);

    /* * * * * * * * * * * * * * * */
    /* * * * Options parsing * * * */
    /* * * * * * * * * * * * * * * */

    ret_status = sn_coap_parser_options_parse(&data_temp_ptr, parsed_and_returned_coap_msg_ptr);

    if (ret_status != 0)
    {
        /* Release memory of CoAP message */
        sn_coap_parser_release_allocated_coap_msg_mem(parsed_and_returned_coap_msg_ptr);

        return NULL;
    }

    /* * * * * * * * * * * * * * * */
    /* * * * Payload parsing * * * */
    /* * * * * * * * * * * * * * * */

    sn_coap_parser_payload_parse(packet_data_len, packet_data_ptr, &data_temp_ptr, parsed_and_returned_coap_msg_ptr);

    /* * * * * * * * * * * * * * * * * * * * * * */
    /* * * * Return parsed CoAP message  * * * * */
    /* * * * * * * * * * * * * * * * * * * * * * */

    return parsed_and_returned_coap_msg_ptr;
}

/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC void sn_coap_parser_release_allocated_coap_msg_mem(sn_coap_hdr_s *freed_coap_msg_ptr)
 *
 * \brief Releases memory of given CoAP message
 *
 *        Note!!! Does not release Payload part
 *
 * \param *freed_coap_msg_ptr is pointer to released CoAP message
 */
SN_MEM_ATTR_COAP_PARSER_FUNC
void sn_coap_parser_release_allocated_coap_msg_mem(sn_coap_hdr_s *freed_coap_msg_ptr)
{
    if (freed_coap_msg_ptr != NULL)
    {
        if (freed_coap_msg_ptr->uri_path_ptr != NULL)
        {
            sn_coap_free(freed_coap_msg_ptr->uri_path_ptr);
        }

        if (freed_coap_msg_ptr->token_ptr != NULL)
        {
            sn_coap_free(freed_coap_msg_ptr->token_ptr);
        }

        if (freed_coap_msg_ptr->content_type_ptr != NULL)
        {
            sn_coap_free(freed_coap_msg_ptr->content_type_ptr);
        }

        if (freed_coap_msg_ptr->options_list_ptr != NULL)
        {
            if (freed_coap_msg_ptr->options_list_ptr->max_age_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->max_age_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->proxy_uri_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->proxy_uri_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->etag_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->etag_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->uri_host_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->uri_host_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->location_path_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->location_path_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->uri_port_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->uri_port_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->location_query_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->location_query_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->observe_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->observe_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->uri_query_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->uri_query_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->block2_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->block2_ptr);
            }

            if (freed_coap_msg_ptr->options_list_ptr->block1_ptr != NULL)
            {
                sn_coap_free(freed_coap_msg_ptr->options_list_ptr->block1_ptr);
            }

            sn_coap_free(freed_coap_msg_ptr->options_list_ptr);
        }

        sn_coap_free(freed_coap_msg_ptr);
    }
}

/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC static void sn_coap_parser_header_parse(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr, coap_version_e *coap_version_ptr, uint16_t *msg_id_ptr)
 *
 * \brief Parses CoAP message's Header part from given Packet data
 *
 * \param **packet_data_ptr is source for Packet data to be parsed to CoAP message
 *
 * \param *dst_coap_msg_ptr is destination for parsed CoAP message
 *
 * \param *coap_version_ptr is destination for parsed CoAP specification version
 *
 * \param *msg_id_ptr is destination for parsed Message ID
 */
SN_MEM_ATTR_COAP_PARSER_FUNC
static void sn_coap_parser_header_parse(uint8_t **packet_data_pptr,
                                        sn_coap_hdr_s *dst_coap_msg_ptr,
                                        coap_version_e *coap_version_ptr)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * */
    /* * * * Parse Header part of CoAP message * * * */
    /* * * * * * * * * * * * * * * * * * * * * * * * */

    /* Parse CoAP Version */
    *coap_version_ptr = (coap_version_e)(COAP_HEADER_VERSION_DATA & COAP_HEADER_VERSION_MASK);

    /* Parse Message type */
    dst_coap_msg_ptr->msg_type = (sn_coap_msg_type_e)(COAP_HEADER_MSG_TYPE_DATA & COAP_HEADER_MSG_TYPE_MASK);

    /* Parse Message code */
    dst_coap_msg_ptr->msg_code = (sn_coap_msg_code_e)COAP_HEADER_MSG_CODE_DATA;

    /* Parse Message ID */
    dst_coap_msg_ptr->msg_id = COAP_HEADER_MSG_ID_DATA_LSB;
    dst_coap_msg_ptr->msg_id += (COAP_HEADER_MSG_ID_DATA_MSB << COAP_HEADER_MSG_ID_MSB_SHIFT);

    /* * * * * * * * * * * * * * * * * * * */
    /* * * * Examine length of Header  * * */
    /* * * * * * * * * * * * * * * * * * * */

    (*packet_data_pptr) += COAP_HEADER_LENGTH;
}

/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC static uint8_t sn_coap_parser_options_parse(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr)
 *
 * \brief Parses CoAP message's Options part from given Packet data
 *
 * \param **packet_data_pptr is source of Packet data to be parsed to CoAP message
 * \param *dst_coap_msg_ptr is destination for parsed CoAP message
 *
 * \return Return value is 0 in ok case and -1 in failure case
 */
SN_MEM_ATTR_COAP_PARSER_FUNC
static uint8_t sn_coap_parser_options_parse(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr)
{
    uint8_t options_count          = 0;
    uint8_t previous_option_number = 0;
    uint8_t i                      = 0;
    int8_t  ret_status             = 0;

    /* Get Options count from CoAP header */
    options_count = (COAP_HEADER_OPTIONS_COUNT_DATA & COAP_HEADER_OPTIONS_COUNT_MASK);

    /* Loop all Options */
    for (i = 0; i < options_count; i++)
    {
        uint8_t  option_number     = (**packet_data_pptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT) + previous_option_number;
        uint16_t option_number_len = (**packet_data_pptr & 0x0F);

        /* Some options are handled independently in own functions */
        if (option_number != COAP_OPTION_URI_PATH && option_number != COAP_OPTION_URI_QUERY && option_number != COAP_OPTION_LOCATION_PATH)
        {
            previous_option_number = previous_option_number + (**packet_data_pptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT);

            (*packet_data_pptr)++;

            /* Check if 4 bits are enough for writing Option value length */
            if (option_number_len == 15)
            {
                option_number_len = 15 + **packet_data_pptr;

                (*packet_data_pptr)++;
            }
        }

        switch (option_number)
        {
            case COAP_OPTION_MAX_AGE:
            case COAP_OPTION_PROXY_URI:
            case COAP_OPTION_ETAG:
            case COAP_OPTION_URI_HOST:
            case COAP_OPTION_LOCATION_PATH:
            case COAP_OPTION_URI_PORT:
            case COAP_OPTION_LOCATION_QUERY:
            case COAP_OPTION_OBSERVE:
            case COAP_OPTION_URI_QUERY:
            case COAP_OPTION_BLOCK2:
            case COAP_OPTION_BLOCK1:
                if (dst_coap_msg_ptr->options_list_ptr == NULL)
                {
                    dst_coap_msg_ptr->options_list_ptr = sn_coap_malloc(sizeof(sn_coap_options_list_s));
                    if(NULL == dst_coap_msg_ptr->options_list_ptr)
                    {
                    	return -1;
                    }
                	memset(dst_coap_msg_ptr->options_list_ptr, 0, sizeof(sn_coap_options_list_s));
                }
                break;
        }

        switch (option_number)
        {
            case COAP_OPTION_CONTENT_TYPE:
                dst_coap_msg_ptr->content_type_len = option_number_len;

                dst_coap_msg_ptr->content_type_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->content_type_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->content_type_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_MAX_AGE:

                dst_coap_msg_ptr->options_list_ptr->max_age_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->max_age_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->max_age_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->max_age_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_PROXY_URI:
                dst_coap_msg_ptr->options_list_ptr->proxy_uri_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->proxy_uri_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->proxy_uri_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->proxy_uri_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_ETAG:
                dst_coap_msg_ptr->options_list_ptr->etag_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->etag_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->etag_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->etag_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_URI_HOST:
                dst_coap_msg_ptr->options_list_ptr->uri_host_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->uri_host_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->uri_host_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->uri_host_ptr, *packet_data_pptr, option_number_len);

                break;

                //todo: update
            case COAP_OPTION_LOCATION_PATH:
                /* This is managed independently because User gives this option in one character table */
            	ret_status = sn_coap_parser_options_parse_multiple_options(packet_data_pptr,
            	                                                                   options_count - i,
            	                                                                   &previous_option_number,
            	                                                                   &dst_coap_msg_ptr->options_list_ptr->location_path_ptr,
            	                                                                   &dst_coap_msg_ptr->options_list_ptr->location_path_len,
            	                                                                   COAP_OPTION_LOCATION_PATH);
                if (ret_status >= 0)
                {
                    i += (ret_status - 1); /* i += is because possible several Options are handled by sn_coap_parser_options_parse_uri_path() */
                }
                else
                {
                    return -1;
                }

                break;


            case COAP_OPTION_URI_PORT:
                dst_coap_msg_ptr->options_list_ptr->uri_port_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->uri_port_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->uri_port_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->uri_port_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_LOCATION_QUERY:
                dst_coap_msg_ptr->options_list_ptr->location_query_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->location_query_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->location_query_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->location_query_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_URI_PATH:
                /* Uri-Path is managed independently because User gives Uri-Path in one character table */
            	ret_status = sn_coap_parser_options_parse_multiple_options(packet_data_pptr,
            	                                                                   options_count - i,
            	                                                                   &previous_option_number,
            	                                                                   &dst_coap_msg_ptr->uri_path_ptr,
            	                                                                   &dst_coap_msg_ptr->uri_path_len,
            	                                                                   COAP_OPTION_URI_PATH);
                if (ret_status >= 0)
                {
                    i += (ret_status - 1); /* i += is because possible several Options are handled by sn_coap_parser_options_parse_uri_path() */
                }
                else
                {
                    return -1;
                }

                break;

            case COAP_OPTION_OBSERVE:

            	dst_coap_msg_ptr->options_list_ptr->observe = 1;

            	if(option_number_len)
            	{

					dst_coap_msg_ptr->options_list_ptr->observe_len = option_number_len;

					dst_coap_msg_ptr->options_list_ptr->observe_ptr = sn_coap_malloc(option_number_len);

					if (dst_coap_msg_ptr->options_list_ptr->observe_ptr == NULL)
					{
						return -1;
					}

					memcpy(dst_coap_msg_ptr->options_list_ptr->observe_ptr, *packet_data_pptr, option_number_len);

            	}

                break;

            case COAP_OPTION_TOKEN:
                dst_coap_msg_ptr->token_len = option_number_len;

                dst_coap_msg_ptr->token_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->token_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->token_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_URI_QUERY:

            	ret_status = sn_coap_parser_options_parse_multiple_options(packet_data_pptr,
            	                                                                   options_count - i,
            	                                                                   &previous_option_number,
            	                                                                   &dst_coap_msg_ptr->options_list_ptr->uri_query_ptr,
            	                                                                   &dst_coap_msg_ptr->options_list_ptr->uri_query_len,
            	                                                                   COAP_OPTION_URI_QUERY);

				if (ret_status >= 0)
				{
					i += (ret_status - 1); /* i += is because possible several Options are handled by sn_coap_parser_options_parse_uri_path() */
				}
				else
				{
					return -1;
				}

				break;

            case COAP_OPTION_BLOCK2:
                dst_coap_msg_ptr->options_list_ptr->block2_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->block2_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->block2_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->block2_ptr, *packet_data_pptr, option_number_len);

                break;

            case COAP_OPTION_BLOCK1:
                dst_coap_msg_ptr->options_list_ptr->block1_len = option_number_len;

                dst_coap_msg_ptr->options_list_ptr->block1_ptr = sn_coap_malloc(option_number_len);

                if (dst_coap_msg_ptr->options_list_ptr->block1_ptr == NULL)
                {
                    return -1;
                }

                memcpy(dst_coap_msg_ptr->options_list_ptr->block1_ptr, *packet_data_pptr, option_number_len);

                break;
        }

        /* If not Uri-Path option because Uri-Path option is handled independently in own function */
        if (option_number != COAP_OPTION_URI_PATH && option_number != COAP_OPTION_URI_QUERY && option_number != COAP_OPTION_LOCATION_PATH)
        {
            (*packet_data_pptr) += option_number_len;
        }
    }

    return 0;
}


/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC static int8_t sn_coap_parser_options_parse_uri_query(uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr, uint8_t options_count_left, uint8_t *previous_option_number_ptr)
 *
 * \brief Parses CoAP message's Uri-query options
 *
 * \param **packet_data_pptr is source for Packet data to be parsed to CoAP message
 *
 * \param *dst_coap_msg_ptr is destination for parsed CoAP message
 *
 * \param options_count_left tells how many options are unhandled in Packet data
 *
 * \param *previous_option_number_ptr is pointer to used and returned previous Option number
 *
 * \return Return value is count of Uri-query optios parsed. In failure case -1 is returned.
*/
SN_MEM_ATTR_COAP_PARSER_FUNC
static int8_t sn_coap_parser_options_parse_multiple_options(uint8_t **packet_data_pptr,
                                                    uint8_t options_count_left,
                                                    uint8_t *previous_option_number_ptr,
                                                    uint8_t **dst_ptr,
                                                    uint16_t *dst_len_ptr,
                                                    sn_coap_option_numbers_e option)
{
    uint16_t    uri_query_needed_heap       = sn_coap_parser_options_count_needed_memory_multiple_option(*packet_data_pptr, options_count_left, *previous_option_number_ptr, option);
    uint8_t     option_number               = (**packet_data_pptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT) + *previous_option_number_ptr;
    uint8_t    *temp_parsed_uri_query_ptr   = NULL;
    uint8_t     returned_option_counter     = 0;

    *dst_ptr = sn_coap_malloc(uri_query_needed_heap);

    if (*dst_ptr == NULL)
    {
        return -1;
    }

    *dst_len_ptr = uri_query_needed_heap;

    temp_parsed_uri_query_ptr = *dst_ptr;

    *previous_option_number_ptr = *previous_option_number_ptr + (**packet_data_pptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT);

    /* Loop all Uri-Query options */
    while (options_count_left > 0 &&
           option_number == option)
    {
        uint16_t option_number_len = (**packet_data_pptr & 0x0F);

        /* Check if this is first Uri-Query option */
        if (returned_option_counter > 0)
        {
            /* Uri-Query is modified to following format: temp1'\0'temp2'\0'temp3 i.e.  */
            /* Uri-Path is modified to following format: temp1\temp2\temp3 i.e.  */
        	if(option == COAP_OPTION_URI_QUERY)
            	memset(temp_parsed_uri_query_ptr, '&', 1);
            else if(option == COAP_OPTION_URI_PATH)
            	memset(temp_parsed_uri_query_ptr, '/', 1);

            temp_parsed_uri_query_ptr++;
        }

        returned_option_counter++;

        (*packet_data_pptr)++;

        /* Check if 4 bits are enough for writing Option value length todo: is this needed? */
        if (option_number_len == 15)
        {
            option_number_len = 15 + **packet_data_pptr;

            (*packet_data_pptr)++;
        }

        memcpy(temp_parsed_uri_query_ptr, *packet_data_pptr, option_number_len);

        (*packet_data_pptr) += option_number_len;
        temp_parsed_uri_query_ptr += option_number_len;

       	option_number = (**packet_data_pptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT) + *previous_option_number_ptr;

        options_count_left--;
    }

    return returned_option_counter;
}




/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC static uint16_t sn_coap_parser_options_count_needed_memory_uri_query(uint8_t *packet_data_ptr, uint8_t options_count_left, uint8_t previous_option_number)
 *
 * \brief Counts needed memory for uri query option
 *
 * \param *packet_data_ptr is start of source for Packet data to be parsed to CoAP message
 *
 * \param options_count_left tells how many options are unhandled in Packet data
 *
 * \param previous_option_number is previous Option number
 */
SN_MEM_ATTR_COAP_PARSER_FUNC
static uint16_t sn_coap_parser_options_count_needed_memory_multiple_option(uint8_t *packet_data_ptr,
                                                                    uint8_t options_count_left,
                                                                    uint8_t previous_option_number, sn_coap_option_numbers_e option)
{
    uint8_t  option_number          = (*packet_data_ptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT) + previous_option_number;
    uint16_t ret_value              = 0;

    /* Loop all Uri-Query options */
    while (options_count_left > 0 &&
           option_number == option)
    {
        uint16_t option_number_len = (*packet_data_ptr & 0x0F);

        packet_data_ptr++;

        /* Check if 4 bits are enough for writing Option value length */
        if (option_number_len == 15)
        {
            option_number_len = 15 + *packet_data_ptr;

            packet_data_ptr++;
        }

        packet_data_ptr += option_number_len;
        ret_value += option_number_len + 1; /* + 1 is for separator */

        option_number = (*packet_data_ptr >> COAP_OPTIONS_OPTION_NUMBER_SHIFT) + option_number;

        options_count_left--;
    }

    return (ret_value - 1); /* -1 because last Part path does not include separator */
}

/**
 * \fn SN_MEM_ATTR_COAP_PARSER_FUNC static void sn_coap_parser_payload_parse(uint16_t packet_data_len, uint8_t *packet_data_ptr, uint8_t **packet_data_pptr, sn_coap_hdr_s *dst_coap_msg_ptr)
 *
 * \brief Parses CoAP message's Payload part from given Packet data
 *
 * \param packet_data_len is length of given Packet data to be parsed to CoAP message
 *
 * \param *packet_data_ptr is start of source for Packet data to be parsed to CoAP message
 *
 * \param **packet_data_pptr is source for Packet data to be parsed to CoAP message
 *
 * \param *dst_coap_msg_ptr is destination for parsed CoAP message
 *****************************************************************************/
SN_MEM_ATTR_COAP_PARSER_FUNC
static void sn_coap_parser_payload_parse(uint16_t packet_data_len,
                                         uint8_t *packet_data_ptr,
                                         uint8_t **packet_data_pptr,
                                         sn_coap_hdr_s *dst_coap_msg_ptr)
{
    /* Parse Payload length */
    dst_coap_msg_ptr->payload_len = packet_data_len - (*packet_data_pptr - packet_data_ptr);

    /* If there is payload */
    if (dst_coap_msg_ptr->payload_len > 0)
    {
        /* Parse Payload by setting CoAP message's payload_ptr to point Payload in Packet data */
    	dst_coap_msg_ptr->payload_ptr = *packet_data_pptr;
    }
}