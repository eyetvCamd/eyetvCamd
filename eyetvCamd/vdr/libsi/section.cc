/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: section.c 1.5 2006/04/14 10:53:44 kls Exp $
 *                                                                         *
 ***************************************************************************/

#include "section.h"
#include <stdio.h>

namespace SI {

/*********************** PAT ***********************/

void PAT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const pat>(s, offset);
   associationLoop.setData(data+offset, getLength()-offset-4);
}

int PAT::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int PAT::Association::getServiceId() const {
   return HILO(s->program_number);
}

int PAT::Association::getPid() const {
   return HILO(s->network_pid);
}

void PAT::Association::Parse() {
   s=data.getData<pat_prog>();
}

/*********************** CAT ***********************/

void CAT::Parse() {
   loop.setData(data+sizeof(cat), getLength()-sizeof(cat)-4);
}

/*********************** PMT ***********************/

void PMT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const pmt>(s, offset);
   commonDescriptors.setDataAndOffset(data+offset, HILO(s->program_info_length), offset);
   streamLoop.setData(data+offset, getLength()-offset-4);
}

int PMT::getServiceId() const {
   return HILO(s->program_number);
}

int PMT::getPCRPid() const {
   return HILO(s->PCR_PID);
}

int PMT::Stream::getPid() const {
   return HILO(s->elementary_PID);
}

int PMT::Stream::getStreamType() const {
   return s->stream_type;
}

void PMT::Stream::Parse() {
   int offset=0;
   data.setPointerAndOffset<const pmt_info>(s, offset);
   streamDescriptors.setData(data+offset, HILO(s->ES_info_length));
}

/*********************** TSDT ***********************/

void TSDT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const tsdt>(s, offset);
   transportStreamDescriptors.setDataAndOffset(data+offset, getLength()-offset-4, offset);
}

/*********************** NIT ***********************/

int NIT::getNetworkId() const {
   return HILO(s->network_id);
}

void NIT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const nit>(s, offset);
   commonDescriptors.setDataAndOffset(data+offset, HILO(s->network_descriptor_length), offset);
   const nit_mid *mid;
   data.setPointerAndOffset<const nit_mid>(mid, offset);
   transportStreamLoop.setData(data+offset, HILO(mid->transport_stream_loop_length));
}

int NIT::TransportStream::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int NIT::TransportStream::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

void NIT::TransportStream::Parse() {
   int offset=0;
   data.setPointerAndOffset<const ni_ts>(s, offset);
   transportStreamDescriptors.setData(data+offset, HILO(s->transport_descriptors_length));
}

/*********************** SDT ***********************/

void SDT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const sdt>(s, offset);
   serviceLoop.setData(data+offset, getLength()-offset-4); //4 is for CRC
}

int SDT::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int SDT::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int SDT::Service::getServiceId() const {
   return HILO(s->service_id);
}

int SDT::Service::getEITscheduleFlag() const {
   return s->eit_schedule_flag;
}

int SDT::Service::getEITpresentFollowingFlag() const {
   return s->eit_present_following_flag;
}

RunningStatus SDT::Service::getRunningStatus() const {
   return (RunningStatus)s->running_status;
}

int SDT::Service::getFreeCaMode() const {
   return s->free_ca_mode;
}

void SDT::Service::Parse() {
   int offset=0;
   data.setPointerAndOffset<const sdt_descr>(s, offset);
   serviceDescriptors.setData(data+offset, HILO(s->descriptors_loop_length));
}

/*********************** EIT ***********************/

int EIT::getServiceId() const {
   return HILO(s->service_id);
}

int EIT::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int EIT::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int EIT::getSegmentLastSectionNumber() const {
   return s->segment_last_section_number;
}

int EIT::getLastTableId() const {
   return s->last_table_id;
}

bool EIT::isPresentFollowing() const {
   return getTableId() == TableIdEIT_presentFollowing || getTableId() == TableIdEIT_presentFollowing_other;
}

bool EIT::isActualTS() const {
   return
       (getTableId() ==TableIdEIT_presentFollowing)
    || (TableIdEIT_schedule_first <= getTableId() && getTableId() <= TableIdEIT_schedule_last);
}

void EIT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const eit>(s, offset);
   //printf("%d %d %d %d %d\n", getServiceId(), getTransportStreamId(), getOriginalNetworkId(), isPresentFollowing(), isActualTS());
   eventLoop.setData(data+offset, getLength()-offset-4); //4 is for CRC
}

time_t EIT::Event::getStartTime() const {
   return DVBTime::getTime(s->mjd_hi, s->mjd_lo, s->start_time_h, s->start_time_m, s->start_time_s);
}

time_t EIT::Event::getDuration() const {
   return DVBTime::getDuration(s->duration_h, s->duration_m, s->duration_s);
}

int EIT::Event::getEventId() const {
   return HILO(s->event_id);
}

int EIT::Event::getMJD() const {
   return HILO(s->mjd);
}

int EIT::Event::getStartTimeHour() const {
   return DVBTime::bcdToDec(s->start_time_h);
}

int EIT::Event::getStartTimeMinute() const {
   return DVBTime::bcdToDec(s->start_time_m);
}

int EIT::Event::getStartTimeSecond() const {
   return DVBTime::bcdToDec(s->start_time_s);
}

int EIT::Event::getDurationHour() const {
   return DVBTime::bcdToDec(s->duration_h);
}

int EIT::Event::getDurationMinute() const {
   return DVBTime::bcdToDec(s->duration_m);
}

int EIT::Event::getDurationSecond() const {
   return DVBTime::bcdToDec(s->duration_s);
}

RunningStatus EIT::Event::getRunningStatus() const {
   return (RunningStatus)s->running_status;
}

int EIT::Event::getFreeCaMode() const {
   return s->free_ca_mode;
}

void EIT::Event::Parse() {
   int offset=0;
   data.setPointerAndOffset<const eit_event>(s, offset);
   //printf("%d %d %d\n", getStartTime(), getDuration(), getRunningStatus());
   eventDescriptors.setData(data+offset, HILO(s->descriptors_loop_length));
}

/*********************** TDT ***********************/

time_t TDT::getTime() const {
   return DVBTime::getTime(s->utc_mjd_hi, s->utc_mjd_lo, s->utc_time_h, s->utc_time_m, s->utc_time_s);
}

void TDT::Parse() {
   s=data.getData<const tdt>();
}

/*********************** TOT ***********************/

time_t TOT::getTime() const {
   return DVBTime::getTime(s->utc_mjd_hi, s->utc_mjd_lo, s->utc_time_h, s->utc_time_m, s->utc_time_s);
}

void TOT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const tot>(s, offset);
   descriptorLoop.setData(data+offset, getLength()-offset-4);
}

/*********************** RST ***********************/

void RST::Parse() {
   int offset=0;
   const rst *s;
   data.setPointerAndOffset<const rst>(s, offset);
   infoLoop.setData(data+offset, getLength()-offset);
}

int RST::RunningInfo::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int RST::RunningInfo::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int RST::RunningInfo::getServiceId() const {
   return HILO(s->service_id);
}

int RST::RunningInfo::getEventId() const {
   return HILO(s->event_id);
}

RunningStatus RST::RunningInfo::getRunningStatus() const {
   return (RunningStatus)s->running_status;
}

void RST::RunningInfo::Parse() {
   s=data.getData<const rst_info>();
}

/*********************** AIT ***********************/

int AIT::getApplicationType() const {
   return HILO(first->application_type);
}

int AIT::getAITVersion() const {
   return first->version_number;
}

void AIT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const ait>(first, offset);
   commonDescriptors.setDataAndOffset(data+offset, HILO(first->common_descriptors_length), offset);
   const ait_mid *mid;
   data.setPointerAndOffset<const ait_mid>(mid, offset);
   applicationLoop.setData(data+offset, HILO(mid->application_loop_length));
}

long AIT::Application::getOrganisationId() const {
   return data.FourBytes(0);
}

int AIT::Application::getApplicationId() const {
   return HILO(s->application_id);
}

int AIT::Application::getControlCode() const {
   return s->application_control_code;
}

void AIT::Application::Parse() {
   int offset=0;
   data.setPointerAndOffset<const ait_app>(s, offset);
   applicationDescriptors.setData(data+offset, HILO(s->application_descriptors_length));
}

/******************* PremiereCIT *******************/

void PremiereCIT::Parse() {
   int offset=0;
   data.setPointerAndOffset<const pcit>(s, offset);
   eventDescriptors.setData(data+offset, HILO(s->descriptors_loop_length));
}

int PremiereCIT::getContentId() const {
   return (HILO(s->contentId_hi) << 16) | HILO(s->contentId_lo);
}

time_t PremiereCIT::getDuration() const {
   return DVBTime::getDuration(s->duration_h, s->duration_m, s->duration_s);
}

} //end of namespace
