/**
 * \file sn_coap_header_ietf_draft_12.h
 *
 * \brief Header file for CoAP Header part
 *
 *  Created on: Jun 30, 2011
 *      Author: tero
 *
 * \note Supports draft-ietf-core-coap-12
 */

#ifndef SN_COAP_HEADER_IETF_DRAFT_09_H_
#define SN_COAP_HEADER_IETF_DRAFT_09_H_

/* * * * * * * * * * * * * * * * * */
/* * * * GLOBAL DECLARATIONS * * * */
/* * * * * * * * * * * * * * * * * */

SN_MEM_ATTR_COAP_PARSER_DECL extern void* (*sn_coap_malloc)(uint16_t); /* Function pointer for used malloc() function */
SN_MEM_ATTR_COAP_PARSER_DECL extern void  (*sn_coap_free)(void*);      /* Function pointer for used free()   function */

/* * * * * * * * * * * */
/* * * * DEFINES * * * */
/* * * * * * * * * * * */

#define COAP_VERSION                                COAP_VERSION_1 /* Tells which IETF CoAP specification version the CoAP message supports. */
                                                                   /* This value is written to CoAP message header part. */

/* CoAP Header defines */
#define COAP_HEADER_LENGTH                          4   /* Fixed Header length of CoAP message as bytes */
#define COAP_HEADER_VERSION_DATA                    *(base_packet_data_ptr + 0) /* CoAP Version */
#define COAP_HEADER_VERSION_MASK                    0xC0
#define COAP_HEADER_MSG_TYPE_DATA                   *(base_packet_data_ptr + 0) /* CoAP Message type */
#define COAP_HEADER_MSG_TYPE_MASK                   0x30
#define COAP_HEADER_OPTIONS_COUNT_DATA              *(base_packet_data_ptr + 0) /* CoAP Options count */
#define COAP_HEADER_OPTIONS_COUNT_MASK              0x0F
#define COAP_HEADER_MSG_CODE_DATA                   *(base_packet_data_ptr + 1) /* CoAP Message code */
#define COAP_HEADER_MSG_ID_DATA_MSB                 *(base_packet_data_ptr + 2)
#define COAP_HEADER_MSG_ID_DATA_LSB                 *(base_packet_data_ptr + 3)
#define COAP_HEADER_MSG_ID_MSB_SHIFT                8

/* CoAP Options defines */
#define COAP_OPTIONS_MAXIMUM_COUNT                  14  /* Maximum count of Options in one CoAP message */
#define COAP_OPTIONS_OPTION_MAXIMUM_LENGTH          1034 /* Maximum length of one Option in CoAP message as bytes */
#define COAP_OPTIONS_OPTION_NUMBER_SHIFT            4

/* * * * * * * * * * * * * * */
/* * * * ENUMERATIONS  * * * */
/* * * * * * * * * * * * * * */

/* * * * * * * * * * * * * */
/* * * * STRUCTURES  * * * */
/* * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * */
/* * * * EXTERNAL FUNCTION PROTOTYPES  * * * */
/* * * * * * * * * * * * * * * * * * * * * * */

extern int8_t         	sn_coap_header_validity_check(sn_coap_hdr_s *src_coap_msg_ptr, coap_version_e coap_version);
extern uint8_t 		 	sn_coap_builder_options_calculate_jump_need(sn_coap_hdr_s *src_coap_msg_ptr, uint8_t block_option);
extern uint16_t 		sn_coap_parser_option_jump_parse(uint8_t **packet_data_pptr);
extern uint16_t 		sn_coap_parser_option_number_len_parse(uint8_t **packet_data_pptr);

#endif /* SN_COAP_HEADER_IETF_DRAFT_09_H_ */