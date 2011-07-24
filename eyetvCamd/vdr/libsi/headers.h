/***************************************************************************
 *                                                                         *
 *   (C) 2001-03 Rolf Hakenes <hakenes@hippomi.de>, under the              *
 *               GNU GPL with contribution of Oleg Assovski,               *
 *               www.satmania.com                                          *
 *               Adapted and extended by Marcel Wiesweg                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: headers.h 1.8 2006/09/02 20:25:16 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_HEADERS_H
#define LIBSI_HEADERS_H

#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

namespace SI {

typedef unsigned char u_char;

struct SectionHeader {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
};

struct ExtendedSectionHeader {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char table_id_extension_hi                  :8;
   u_char table_id_extension_lo                  :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
};

struct DescriptorHeader {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/*
 *
 *    ETSI ISO/IEC 13818-1 specifies SI which is referred to as PSI. The PSI
 *    data provides information to enable automatic configuration of the
 *    receiver to demultiplex and decode the various streams of programs
 *    within the multiplex. The PSI data is structured as four types of table.
 *    The tables are transmitted in sections.
 *
 *    1) Program Association Table (PAT):
 *
 *       - for each service in the multiplex, the PAT indicates the location
 *         (the Packet Identifier (PID) values of the Transport Stream (TS)
 *         packets) of the corresponding Program Map Table (PMT).
 *         It also gives the location of the Network Information Table (NIT).
 *
 */

#define PAT_LEN 8

struct pat {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char dummy                                  :1;        // has to be 0
   u_char                                        :2;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :2;
   u_char dummy                                  :1;        // has to be 0
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
};

#define PAT_PROG_LEN 4

struct pat_prog {
   u_char program_number_hi                      :8;
   u_char program_number_lo                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :3;
   u_char network_pid_hi                         :5;
#else
   u_char network_pid_hi                         :5;
   u_char                                        :3;
#endif
   u_char network_pid_lo                         :8;
   /* or program_map_pid (if prog_num=0)*/
};

/*
 *
 *    2) Conditional Access Table (CAT):
 *
 *       - the CAT provides information on the CA systems used in the
 *         multiplex; the information is private and dependent on the CA
 *         system, but includes the location of the EMM stream, when
 *         applicable.
 *
 */
#define CAT_LEN 8

struct cat {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char dummy                                  :1;        // has to be 0
   u_char                                        :2;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :2;
   u_char dummy                                  :1;        // has to be 0
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char reserved_1                             :8;
   u_char reserved_2                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
};

/*
 *
 *    3) Program Map Table (PMT):
 *
 *       - the PMT identifies and indicates the locations of the streams that
 *         make up each service, and the location of the Program Clock
 *         Reference fields for a service.
 *
 */

#define PMT_LEN 12

struct pmt {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char dummy                                  :1; // has to be 0
   u_char                                        :2;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :2;
   u_char dummy                                  :1; // has to be 0
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char program_number_hi                      :8;
   u_char program_number_lo                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :3;
   u_char PCR_PID_hi                             :5;
#else
   u_char PCR_PID_hi                             :5;
   u_char                                        :3;
#endif
   u_char PCR_PID_lo                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char program_info_length_hi                 :4;
#else
   u_char program_info_length_hi                 :4;
   u_char                                        :4;
#endif
   u_char program_info_length_lo                 :8;
   //descriptors
};

#define PMT_INFO_LEN 5

struct pmt_info {
   u_char stream_type                            :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :3;
   u_char elementary_PID_hi                      :5;
#else
   u_char elementary_PID_hi                      :5;
   u_char                                        :3;
#endif
   u_char elementary_PID_lo                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char ES_info_length_hi                      :4;
#else
   u_char ES_info_length_hi                      :4;
   u_char                                        :4;
#endif
   u_char ES_info_length_lo                      :8;
   // descriptors
};

/*
 *
 *    4) Transport Stream Description Table (TSDT):
 *
 *       - The TSDT carries a loop of descriptors that apply to
 *         the whole transport stream. The syntax and semantics
 *         of the TSDT are defined in newer versions of ISO/IEC 13818-1.
 *
 */

#define TSDT_LEN 8

struct tsdt {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char dummy                                  :1; // has to be 0
   u_char                                        :2;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :2;
   u_char dummy                                  :1; // has to be 0
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char                                        :8;
   u_char                                        :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
};

/*
 *
 *    5) Network Information Table (NIT):
 *
 *       - the NIT is intended to provide information about the physical
 *         network. The syntax and semantics of the NIT are defined in
 *         ETSI EN 300 468.
 *
 */

#define NIT_LEN 10

struct nit {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char network_id_hi                          :8;
   u_char network_id_lo                          :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char network_descriptor_length_hi           :4;
#else
   u_char network_descriptor_length_hi           :4;
   u_char                                        :4;
#endif
   u_char network_descriptor_length_lo           :8;
  /* descriptors */
};

#define SIZE_NIT_MID 2

struct nit_mid {                                 // after descriptors
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char transport_stream_loop_length_hi        :4;
#else
   u_char transport_stream_loop_length_hi        :4;
   u_char                                        :4;
#endif
   u_char transport_stream_loop_length_lo        :8;
};

#define SIZE_NIT_END 4

struct nit_end {
   long CRC;
};

#define NIT_TS_LEN 6

struct ni_ts {
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char transport_descriptors_length_hi        :4;
#else
   u_char transport_descriptors_length_hi        :4;
   u_char                                        :4;
#endif
   u_char transport_descriptors_length_lo        :8;
   /* descriptors  */
};

/*
 *
 *    In addition to the PSI, data is needed to provide identification of
 *    services and events for the user. In contrast with the PAT, CAT, and
 *    PMT of the PSI, which give information only for the multiplex in which
 *    they are contained (the actual multiplex), the additional information
 *    defined within the present document can also provide information on
 *    services and events carried by different multiplexes, and even on other
 *    networks. This data is structured as nine tables:
 *
 *    1) Bouquet Association Table (BAT):
 *
 *       - the BAT provides information regarding bouquets. As well as giving
 *         the name of the bouquet, it provides a list of services for each
 *         bouquet.
 *
 */
/* SEE NIT (It has the same structure but has different allowed descriptors) */
/*
 *
 *    2) Service Description Table (SDT):
 *
 *       - the SDT contains data describing the services in the system e.g.
 *         names of services, the service provider, etc.
 *
 */

#define SDT_LEN 11

struct sdt {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char                                        :8;
};

#define GetSDTTransportStreamId(x) (HILO(((sdt_t *) x)->transport_stream_id))
#define GetSDTOriginalNetworkId(x) (HILO(((sdt_t *) x)->original_network_id))

#define SDT_DESCR_LEN 5

struct sdt_descr {
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :6;
   u_char eit_schedule_flag                      :1;
   u_char eit_present_following_flag             :1;
   u_char running_status                         :3;
   u_char free_ca_mode                           :1;
   u_char descriptors_loop_length_hi             :4;
#else
   u_char eit_present_following_flag             :1;
   u_char eit_schedule_flag                      :1;
   u_char                                        :6;
   u_char descriptors_loop_length_hi             :4;
   u_char free_ca_mode                           :1;
   u_char running_status                         :3;
#endif
   u_char descriptors_loop_length_lo             :8;
};

/*
 *
 *    3) Event Information Table (EIT):
 *
 *       - the EIT contains data concerning events or programmes such as event
 *         name, start time, duration, etc.; - the use of different descriptors
 *         allows the transmission of different kinds of event information e.g.
 *         for different service types.
 *
 */

#define EIT_LEN 14

struct eit {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char segment_last_section_number            :8;
   u_char last_table_id                          :8;
};

#define EIT_EVENT_LEN 12

struct eit_event {
   u_char event_id_hi                            :8;
   u_char event_id_lo                            :8;
   u_char mjd_hi                                 :8;
   u_char mjd_lo                                 :8;
   u_char start_time_h                           :8;
   u_char start_time_m                           :8;
   u_char start_time_s                           :8;
   u_char duration_h                             :8;
   u_char duration_m                             :8;
   u_char duration_s                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char running_status                         :3;
   u_char free_ca_mode                           :1;
   u_char descriptors_loop_length_hi             :4;
#else
   u_char descriptors_loop_length_hi             :4;
   u_char free_ca_mode                           :1;
   u_char running_status                         :3;
#endif
   u_char descriptors_loop_length_lo             :8;
};

/*
 *
 *    4) Running Status Table (RST):
 *
 *       - the RST gives the status of an event (running/not running). The RST
 *         updates this information and allows timely automatic switching to
 *         events.
 *
 */

struct rst {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
};

struct rst_info {
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
   u_char event_id_hi                            :8;
   u_char event_id_lo                            :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :5;
   u_char running_status                         :3;
#else
   u_char running_status                         :3;
   u_char                                        :5;
#endif
};

/*
 *
 *    5) Time and Date Table (TDT):
 *
 *       - the TDT gives information relating to the present time and date.
 *         This information is given in a separate table due to the frequent
 *         updating of this information.
 *
 */

#define TDT_LEN 8

struct tdt {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char utc_mjd_hi                             :8;
   u_char utc_mjd_lo                             :8;
   u_char utc_time_h                             :8;
   u_char utc_time_m                             :8;
   u_char utc_time_s                             :8;
};

/*
 *
 *    6) Time Offset Table (TOT):
 *
 *       - the TOT gives information relating to the present time and date and
 *         local time offset. This information is given in a separate table due
 *         to the frequent updating of the time information.
 *
 */
#define TOT_LEN 10

struct tot {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char utc_mjd_hi                             :8;
   u_char utc_mjd_lo                             :8;
   u_char utc_time_h                             :8;
   u_char utc_time_m                             :8;
   u_char utc_time_s                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char descriptors_loop_length_hi             :4;
#else
   u_char descriptors_loop_length_hi             :4;
   u_char                                        :4;
#endif
   u_char descriptors_loop_length_lo             :8;
};

/*
 *
 *    7) Stuffing Table (ST):
 *
 *       - the ST is used to invalidate existing sections, for example at
 *         delivery system boundaries.
 *
 */
    /* TO BE DONE */
/*
 *
 *    8) Selection Information Table (SIT):
 *
 *       - the SIT is used only in "partial" (i.e. recorded) bitstreams. It
 *         carries a summary of the SI information required to describe the
 *         streams in the partial bitstream.
 *
 */
    /* TO BE DONE */
/*
 *
 *    9) Discontinuity Information Table (DIT):
 *
 *       - the DIT is used only in "partial" (i.e. recorded) bitstreams.
 *         It is inserted where the SI information in the partial bitstream may
 *         be discontinuous. Where applicable the use of descriptors allows a
 *         flexible approach to the organization of the tables and allows for
 *         future compatible extensions.
 *
 */
    /* TO BE DONE */

/*
 *
 *    3) Application Information Table (AIT):
 *
 *       - the AIT contains data concerning MHP application broadcast by a service.
 *
 */

#define AIT_LEN 10

struct ait {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :3;
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char application_type_hi                    :8;
   u_char application_type_lo                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char common_descriptors_length_hi           :4;
#else
   u_char common_descriptors_length_hi           :4;
   u_char                                        :4;
#endif
   u_char common_descriptors_length_lo           :8;
};

#define SIZE_AIT_MID 2

struct ait_mid {                                 // after descriptors
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char application_loop_length_hi             :4;
#else
   u_char application_loop_length_hi             :4;
   u_char                                        :4;
#endif
   u_char application_loop_length_lo             :8;
};

#define SIZE_AIT_END 4

struct ait_end {
   long CRC;
};

#define AIT_APP_LEN 9

struct ait_app {
   //how to deal with 32 bit fields?

   u_char organisation_id_hi_hi                  :8;
   u_char organisation_id_hi_lo                  :8;
   u_char organisation_id_lo_hi                  :8;
   u_char organisation_id_lo_lo                  :8;

   //long organisation_id                          :32;
   u_char application_id_hi                      :8;
   u_char application_id_lo                      :8;
   u_char application_control_code               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char application_descriptors_length_hi      :4;
#else
   u_char application_descriptors_length_hi      :4;
   u_char                                        :4;
#endif
   u_char application_descriptors_length_lo      :8;
   /* descriptors  */
};

/* Premiere Content Information Table */

#define PCIT_LEN 17

struct pcit {
   u_char table_id                               :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char section_syntax_indicator               :1;
   u_char dummy                                  :1; // has to be 0
   u_char                                        :2;
   u_char section_length_hi                      :4;
#else
   u_char section_length_hi                      :4;
   u_char                                        :2;
   u_char dummy                                  :1; // has to be 0
   u_char section_syntax_indicator               :1;
#endif
   u_char section_length_lo                      :8;
   u_char                                        :8;
   u_char                                        :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
#else
   u_char current_next_indicator                 :1;
   u_char version_number                         :5;
   u_char                                        :2;
#endif
   u_char section_number                         :8;
   u_char last_section_number                    :8;

   u_char contentId_hi_hi                        :8;
   u_char contentId_hi_lo                        :8;
   u_char contentId_lo_hi                        :8;
   u_char contentId_lo_lo                        :8;

   u_char duration_h                             :8;
   u_char duration_m                             :8;
   u_char duration_s                             :8;

#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :4;
   u_char descriptors_loop_length_hi             :4;
#else
   u_char descriptors_loop_length_hi             :4;
   u_char                                        :4;
#endif
   u_char descriptors_loop_length_lo             :8;
};

/*
 *
 *    The following describes the different descriptors that can be used within
 *    the SI.
 *
 *    The following semantics apply to all the descriptors defined in this
 *    subclause:
 *
 *    descriptor_tag: The descriptor tag is an 8-bit field which identifies
 *                    each descriptor. Those values with MPEG-2 normative
 *                    meaning are described in ISO/IEC 13818-1. The values of
 *                    descriptor_tag are defined in 'libsi.h'
 *    descriptor_length: The descriptor length is an 8-bit field specifying the
 *                       total number of bytes of the data portion of the
 *                       descriptor following the byte defining the value of
 *                       this field.
 *
 */

#define DESCR_GEN_LEN 2
struct descr_gen {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define GetDescriptorTag(x) (((descr_gen_t *) x)->descriptor_tag)
#define GetDescriptorLength(x) (((descr_gen_t *) x)->descriptor_length+DESCR_GEN_LEN)

/* 0x09 ca_descriptor */

#define DESCR_CA_LEN 6
struct descr_ca {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char CA_type_hi                             :8;
   u_char CA_type_lo                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char reserved                               :3;
   u_char CA_PID_hi                              :5;
#else
   u_char CA_PID_hi                              :5;
   u_char reserved                               :3;
#endif
   u_char CA_PID_lo                              :8;
};

/* 0x0A iso_639_language_descriptor */

#define DESCR_ISO_639_LANGUAGE_LEN 5
struct descr_iso_639_language {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

struct descr_iso_639_language_loop {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char audio_type                             :8;
};

/* 0x13 carousel_identifier_descriptor */

#define DESCR_CAROUSEL_IDENTIFIER_LEN 7
struct descr_carousel_identifier {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char carousel_id_hi_hi                      :8;
   u_char carousel_id_hi_lo                      :8;
   u_char carousel_id_lo_hi                      :8;
   u_char carousel_id_lo_lo                      :8;
   u_char FormatId                               :8;
   /* FormatSpecifier follows */
};

/* 0x40 network_name_descriptor */

#define DESCR_NETWORK_NAME_LEN 2
struct descr_network_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/* 0x41 service_list_descriptor */

#define DESCR_SERVICE_LIST_LEN 2
struct descr_service_list {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define DESCR_SERVICE_LIST_LOOP_LEN 3
struct descr_service_list_loop {
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
   u_char service_type                           :8;
};

/* 0x42 stuffing_descriptor */

#define DESCR_STUFFING_LEN XX
struct descr_stuffing {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x43 satellite_delivery_system_descriptor */

#define DESCR_SATELLITE_DELIVERY_SYSTEM_LEN 13
struct descr_satellite_delivery_system {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char frequency_hi_hi                        :8;
   u_char frequency_hi_lo                        :8;
   u_char frequency_lo_hi                        :8;
   u_char frequency_lo_lo                        :8;
   u_char orbital_position_hi                    :8;
   u_char orbital_position_lo                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char west_east_flag                         :1;
   u_char polarization                           :2;
   u_char modulation                             :5;
#else
   u_char modulation                             :5;
   u_char polarization                           :2;
   u_char west_east_flag                         :1;
#endif
   u_char symbol_rate_hi_hi                      :8;
   u_char symbol_rate_hi_lo                      :8;
   u_char symbol_rate_lo_1                       :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char symbol_rate_lo_2                       :4;
   u_char fec_inner                              :4;
#else
   u_char fec_inner                              :4;
   u_char symbol_rate_lo_2                       :4;
#endif
};

/* 0x44 cable_delivery_system_descriptor */

#define DESCR_CABLE_DELIVERY_SYSTEM_LEN 13
struct descr_cable_delivery_system {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char frequency_hi_hi                        :8;
   u_char frequency_hi_lo                        :8;
   u_char frequency_lo_hi                        :8;
   u_char frequency_lo_lo                        :8;
   u_char reserved1                              :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char reserved2                              :4;
   u_char fec_outer                              :4;
#else
   u_char fec_outer                              :4;
   u_char reserved2                              :4;
#endif
   u_char modulation                             :8;
   u_char symbol_rate_hi_hi                      :8;
   u_char symbol_rate_hi_lo                      :8;
   u_char symbol_rate_lo_1                       :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char symbol_rate_lo_2                       :4;
   u_char fec_inner                              :4;
#else
   u_char fec_inner                              :4;
   u_char symbol_rate_lo_2                       :4;
#endif
};

/* 0x45 vbi_data_descriptor */

#define DESCR_VBI_DATA_LEN XX
struct descr_vbi_data {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x46 vbi_teletext_descriptor */

#define DESCR_VBI_TELETEXT_LEN XX
struct descr_vbi_teletext {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x47 bouquet_name_descriptor */

#define DESCR_BOUQUET_NAME_LEN 2
struct descr_bouquet_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/* 0x48 service_descriptor */

#define DESCR_SERVICE_LEN  4
struct descr_service {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char service_type                           :8;
   u_char provider_name_length                   :8;
};

struct descr_service_mid {
   u_char service_name_length                   :8;
};

/* 0x49 country_availability_descriptor */

#define DESCR_COUNTRY_AVAILABILITY_LEN 3
struct descr_country_availability {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char country_availability_flag              :1;
   u_char reserved                               :7;
#else
   u_char reserved                               :7;
   u_char country_availability_flag              :1;
#endif
};

/* 0x4A linkage_descriptor */

#define DESCR_LINKAGE_LEN 9
struct descr_linkage {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
   u_char linkage_type                           :8;
};

#define DESCR_LINKAGE_8_LEN 3
struct descr_linkage_8 {
#if BYTE_ORDER == BIG_ENDIAN
   u_char hand_over_type                         :4;
   u_char reserved                               :3;
   u_char origin_type                            :1;
#else
   u_char origin_type                            :1;
   u_char reserved                               :3;
   u_char hand_over_type                         :4;
#endif
   u_char id_hi                                  :8;
   u_char id_lo                                  :8;
};

/* 0x4B nvod_reference_descriptor */

#define DESCR_NVOD_REFERENCE_LEN 2
struct descr_nvod_reference {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define ITEM_NVOD_REFERENCE_LEN 6
struct item_nvod_reference {
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
};

/* 0x4C time_shifted_service_descriptor */

#define DESCR_TIME_SHIFTED_SERVICE_LEN 4
struct descr_time_shifted_service {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char reference_service_id_hi                :8;
   u_char reference_service_id_lo                :8;
};

/* 0x4D short_event_descriptor */

#define DESCR_SHORT_EVENT_LEN 6
struct descr_short_event {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char event_name_length                      :8;
};

struct descr_short_event_mid {
   u_char text_length                      :8;
};

/* 0x4E extended_event_descriptor */

#define DESCR_EXTENDED_EVENT_LEN 7
struct descr_extended_event {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
#if BYTE_ORDER == BIG_ENDIAN
   u_char descriptor_number                      :4;
   u_char last_descriptor_number                 :4;
#else
   u_char last_descriptor_number                 :4;
   u_char descriptor_number                      :4;
#endif
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char length_of_items                        :8;
};

struct descr_extended_event_mid {
   u_char text_length                            :8;
};

#define ITEM_EXTENDED_EVENT_LEN 1
struct item_extended_event {
   u_char item_description_length                :8;
};

struct item_extended_event_mid {
   u_char item_length                            :8;
};

/* 0x4F time_shifted_event_descriptor */

#define DESCR_TIME_SHIFTED_EVENT_LEN 6
struct descr_time_shifted_event {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char reference_service_id_hi                :8;
   u_char reference_service_id_lo                :8;
   u_char reference_event_id_hi                  :8;
   u_char reference_event_id_lo                  :8;
};

/* 0x50 component_descriptor */

#define DESCR_COMPONENT_LEN  8
struct descr_component {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char reserved                               :4;
   u_char stream_content                         :4;
#else
   u_char stream_content                         :4;
   u_char reserved                               :4;
#endif
   u_char component_type                         :8;
   u_char component_tag                          :8;
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
};

/* 0x51 mosaic_descriptor */

#define DESCR_MOSAIC_LEN XX
struct descr_mosaic {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x52 stream_identifier_descriptor */

#define DESCR_STREAM_IDENTIFIER_LEN 3
struct descr_stream_identifier {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char component_tag                          :8;
};

/* 0x53 ca_identifier_descriptor */

#define DESCR_CA_IDENTIFIER_LEN 2
struct descr_ca_identifier {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/* 0x54 content_descriptor */

#define DESCR_CONTENT_LEN 2
struct descr_content {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

struct nibble_content {
#if BYTE_ORDER == BIG_ENDIAN
   u_char content_nibble_level_1                 :4;
   u_char content_nibble_level_2                 :4;
#else
   u_char content_nibble_level_2                 :4;
   u_char content_nibble_level_1                 :4;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   u_char user_nibble_1                          :4;
   u_char user_nibble_2                          :4;
#else
   u_char user_nibble_2                          :4;
   u_char user_nibble_1                          :4;
#endif
};

/* 0x55 parental_rating_descriptor */

#define DESCR_PARENTAL_RATING_LEN 2
struct descr_parental_rating {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define PARENTAL_RATING_LEN 4
struct parental_rating {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char rating                                 :8;
};

/* 0x56 teletext_descriptor */

#define DESCR_TELETEXT_LEN 2
struct descr_teletext {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define ITEM_TELETEXT_LEN 5
struct item_teletext {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char type                                   :5;
   u_char magazine_number                        :3;
#else
   u_char magazine_number                        :3;
   u_char type                                   :5;
#endif
   u_char page_number                            :8;
};

/* 0x57 telephone_descriptor */

#define DESCR_TELEPHONE_LEN XX
struct descr_telephone {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x58 local_time_offset_descriptor */

#define DESCR_LOCAL_TIME_OFFSET_LEN 2
struct descr_local_time_offset {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define LOCAL_TIME_OFFSET_ENTRY_LEN 15
struct local_time_offset_entry {
   u_char country_code1                          :8;
   u_char country_code2                          :8;
   u_char country_code3                          :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char country_region_id                      :6;
   u_char                                        :1;
   u_char local_time_offset_polarity             :1;
#else
   u_char local_time_offset_polarity             :1;
   u_char                                        :1;
   u_char country_region_id                      :6;
#endif
   u_char local_time_offset_h                    :8;
   u_char local_time_offset_m                    :8;
   u_char time_of_change_mjd_hi                  :8;
   u_char time_of_change_mjd_lo                  :8;
   u_char time_of_change_time_h                  :8;
   u_char time_of_change_time_m                  :8;
   u_char time_of_change_time_s                  :8;
   u_char next_time_offset_h                     :8;
   u_char next_time_offset_m                     :8;
};

/* 0x59 subtitling_descriptor */

#define DESCR_SUBTITLING_LEN 2
struct descr_subtitling {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define ITEM_SUBTITLING_LEN 8
struct item_subtitling {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char subtitling_type                        :8;
   u_char composition_page_id_hi                 :8;
   u_char composition_page_id_lo                 :8;
   u_char ancillary_page_id_hi                   :8;
   u_char ancillary_page_id_lo                   :8;
};

/* 0x5A terrestrial_delivery_system_descriptor */

#define DESCR_TERRESTRIAL_DELIVERY_SYSTEM_LEN XX
struct descr_terrestrial_delivery {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char frequency_hi_hi                        :8;
   u_char frequency_hi_lo                        :8;
   u_char frequency_lo_hi                        :8;
   u_char frequency_lo_lo                        :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char bandwidth                              :3;
   u_char reserved1                              :5;
#else
   u_char reserved1                              :5;
   u_char bandwidth                              :3;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   u_char constellation                          :2;
   u_char hierarchy                              :3;
   u_char code_rate_HP                           :3;
#else
   u_char code_rate_HP                           :3;
   u_char hierarchy                              :3;
   u_char constellation                          :2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   u_char code_rate_LP                           :3;
   u_char guard_interval                         :2;
   u_char transmission_mode                      :2;
   u_char other_frequency_flag                   :1;
#else
   u_char other_frequency_flag                   :1;
   u_char transmission_mode                      :2;
   u_char guard_interval                         :2;
   u_char code_rate_LP                           :3;
#endif
   u_char reserver2                              :8;
   u_char reserver3                              :8;
   u_char reserver4                              :8;
   u_char reserver5                              :8;
};

/* 0x5B multilingual_network_name_descriptor */

#define DESCR_MULTILINGUAL_NETWORK_NAME_LEN XX
struct descr_multilingual_network_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

struct entry_multilingual_name {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char text_length                            :8;
};

/* 0x5C multilingual_bouquet_name_descriptor */

#define DESCR_MULTILINGUAL_BOUQUET_NAME_LEN XX
struct descr_multilingual_bouquet_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/* 0x5D multilingual_service_name_descriptor */

#define DESCR_MULTILINGUAL_SERVICE_NAME_LEN XX
struct descr_multilingual_service_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

struct entry_multilingual_service_name_mid {
   u_char service_name_length                    :8;
};

/* 0x5E multilingual_component_descriptor */

#define DESCR_MULTILINGUAL_COMPONENT_LEN XX
struct descr_multilingual_component {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char component_tag                          :8;
};

/* 0x5F private_data_specifier_descriptor */

#define DESCR_PRIVATE_DATA_SPECIFIER_LEN XX
struct descr_private_data_specifier {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char private_data_specifier_hi_hi           :8;
   u_char private_data_specifier_hi_lo           :8;
   u_char private_data_specifier_lo_hi           :8;
   u_char private_data_specifier_lo_lo           :8;
};

/* 0x60 service_move_descriptor */

#define DESCR_SERVICE_MOVE_LEN XX
struct descr_service_move {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char new_original_network_id_hi             :8;
   u_char new_original_network_id_lo             :8;
   u_char new_transport_stream_id_hi             :8;
   u_char new_transport_stream_id_lo             :8;
   u_char new_service_id_hi                      :8;
   u_char new_service_id_lo                      :8;
};

/* 0x61 short_smoothing_buffer_descriptor */

#define DESCR_SHORT_SMOOTHING_BUFFER_LEN XX
struct descr_short_smoothing_buffer {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x62 frequency_list_descriptor */

#define DESCR_FREQUENCY_LIST_LEN XX
struct descr_frequency_list {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :6;
   u_char coding_type                            :2;
#else
   u_char coding_type                            :2;
   u_char                                        :6;
#endif
};

/* 0x63 partial_transport_stream_descriptor */

#define DESCR_PARTIAL_TRANSPORT_STREAM_LEN XX
struct descr_partial_transport_stream {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x64 data_broadcast_descriptor */

#define DESCR_DATA_BROADCAST_LEN XX
struct descr_data_broadcast {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x65 ca_system_descriptor */

#define DESCR_CA_SYSTEM_LEN XX
struct descr_ca_system {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x66 data_broadcast_id_descriptor */

#define DESCR_DATA_BROADCAST_ID_LEN XX
struct descr_data_broadcast_id {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x67 transport_stream_descriptor */

#define DESCR_TRANSPORT_STREAM_LEN XX
struct descr_transport_stream {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x68 dsng_descriptor */

#define DESCR_DSNG_LEN XX
struct descr_dsng {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x69 pdc_descriptor */

#define DESCR_PDC_LEN 5
struct descr_pdc {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char pil0                                   :8;
   u_char pil1                                   :8;
   u_char pil2                                   :8;
};

/* 0x6A ac3_descriptor */

#define DESCR_AC3_LEN 3
struct descr_ac3 {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char ac3_type_flag                          :1;
   u_char bsid_flag                              :1;
   u_char mainid_flag                            :1;
   u_char asvc_flag                              :1;
   u_char reserved                               :4;
#else
   u_char reserved                               :4;
   u_char asvc_flag                              :1;
   u_char mainid_flag                            :1;
   u_char bsid_flag                              :1;
   u_char ac3_type_flag                          :1;
#endif
   u_char ac3_type                               :8;
   u_char bsid                                   :8;
   u_char mainid                                 :8;
   u_char asvc                                   :8;
};

/* 0x6B ancillary_data_descriptor */

#define DESCR_ANCILLARY_DATA_LEN 3
struct descr_ancillary_data {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char ancillary_data_identifier              :8;
};

/* 0x6C cell_list_descriptor */

#define DESCR_CELL_LIST_LEN XX
struct descr_cell_list {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x6D cell_frequency_link_descriptor */

#define DESCR_CELL_FREQUENCY_LINK_LEN XX
struct descr_cell_frequency_link {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x6E announcement_support_descriptor */

#define DESCR_ANNOUNCEMENT_SUPPORT_LEN XX
struct descr_announcement_support {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   /* TBD */
};

/* 0x6F application_signalling_descriptor */

#define DESCR_APPLICATION_SIGNALLING_LEN 2
struct descr_application_signalling {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define APPLICATION_SIGNALLING_ENTRY_LEN 3
struct application_signalling_entry {
   u_char application_type_hi                    :8;
   u_char application_type_lo                    :8;
#if BYTE_ORDER == BIG_ENDIAN
   u_char                                        :3;
   u_char AIT_version_number                     :5;
#else
   u_char AIT_version_number                     :5;
   u_char                                        :3;
#endif
};

/* 0x71 service_identifier_descriptor (ETSI TS 102 812, MHP) */

struct descr_service_identifier {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

/* MHP 0x00 application_descriptor */

#define DESCR_APPLICATION_LEN 3

struct descr_application {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char application_profiles_length            :8;
};

#define DESCR_APPLICATION_END_LEN 2

struct descr_application_end {
#if BYTE_ORDER == BIG_ENDIAN
   u_char service_bound_flag                     :1;
   u_char visibility                             :2;
   u_char                                        :5;
#else
   u_char                                        :5;
   u_char visibility                             :2;
   u_char service_bound_flag                     :1;
#endif
   u_char application_priority                   :8;
/*now follow 8bit transport_protocol_label fields to the end */
};

#define APPLICATION_PROFILE_ENTRY_LEN 5

struct application_profile_entry {
   u_char application_profile_hi                 :8;
   u_char application_profile_lo                 :8;
   u_char version_major                          :8;
   u_char version_minor                          :8;
   u_char version_micro                          :8;
};

/* MHP 0x01 application_name_desriptor */

#define DESCR_APPLICATION_NAME_LEN 2

struct descr_application_name {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define APPLICATION_NAME_ENTRY_LEN 4

struct descr_application_name_entry {
   u_char lang_code1                             :8;
   u_char lang_code2                             :8;
   u_char lang_code3                             :8;
   u_char application_name_length                :8;
   /* application name string */
};

/* MHP 0x02 transport_protocol_descriptor */

#define DESCR_TRANSPORT_PROTOCOL_LEN 5

struct descr_transport_protocol {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char protocol_id_hi                         :8;
   u_char protocol_id_lo                         :8;
   u_char transport_protocol_label               :8;
   /* protocol_id-specific selector bytes follow */
};

#define TRANSPORT_VIA_OC_LEN 1

struct transport_via_oc {
#if BYTE_ORDER == BIG_ENDIAN
   u_char remote                                 :1;
   u_char                                        :7;
#else
   u_char                                        :7;
   u_char remote                                 :1;
#endif
};

//if remote is true, transport_via_oc_remote_end_t follows,
// else transport_via_oc_end_t.

#define TRANSPORT_VIA_OC_REMOTE_END_LEN 7

struct transport_via_oc_remote_end {
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
   u_char component_tag                          :8;
};

#define TRANSPORT_VIA_OC_END_LEN 1

struct transport_via_oc_end {
   u_char component_tag                          :8;
};

/* 0x03 dvb_j_application_descriptor() */

#define DESCR_DVBJ_APPLICATION_LEN 2

struct descr_dvbj_application {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
};

#define DESCR_DVBJ_APPLICATION_ENTRY_LEN 1

struct descr_dvbj_application_entry {
   u_char parameter_length                       :8;
   /* parameter string */
};

/* 0x04 dvb_j_application_location_descriptor */

#define DESCR_DVBJ_APPLICATION_LOCATION_LEN 3

struct descr_dvbj_application_location {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char base_directory_length                  :8;
   /* base directory string */
};

#define DESCR_DVBJ_APPLICATION_LOCATION_MID_LEN 1

struct descr_dvbj_application_location_mid {
   u_char classpath_extension_length                  :8;
};

/* 0x0B application_icons_descriptor */

#define DESCR_APPLICATION_ICONS_LEN 3

struct descr_application_icons_descriptor {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char icon_locator_length                    :8;
   /* icon locator */
};

#define DESCR_APPLICATION_ICONS_END_LEN 2

struct descr_application_icons_descriptor_end {
   u_char icon_flags_hi                          :8;
   u_char icon_flags_lo                          :8;
};

// Private DVB Descriptor  Premiere.de
// 0xF2  Content Transmission Descriptor
// http://dvbsnoop.sourceforge.net/examples/example-private-section.html

#define DESCR_PREMIERE_CONTENT_TRANSMISSION_LEN 8

struct descr_premiere_content_transmission {
   u_char descriptor_tag                         :8;
   u_char descriptor_length                      :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
};

#define ITEM_PREMIERE_CONTENT_TRANSMISSION_DAY_LEN 3

struct item_premiere_content_transmission_day {
   u_char mjd_hi                                 :8;
   u_char mjd_lo                                 :8;
   u_char start_time_loop                        :8;
};

#define ITEM_PREMIERE_CONTENT_TRANSMISSION_TIME_LEN 3

struct item_premiere_content_transmission_time {
   u_char start_time_h                           :8;
   u_char start_time_m                           :8;
   u_char start_time_s                           :8;
};

} //end of namespace

#endif //LIBSI_HEADERS_H
