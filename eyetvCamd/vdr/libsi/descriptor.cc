/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: descriptor.c 1.21 2006/05/28 14:25:30 kls Exp $
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include "descriptor.h"

namespace SI {

void ShortEventDescriptor::Parse() {
   int offset=0;
   const descr_short_event *s;
   data.setPointerAndOffset<const descr_short_event>(s, offset);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
   name.setDataAndOffset(data+offset, s->event_name_length, offset);
   const descr_short_event_mid *mid;
   data.setPointerAndOffset<const descr_short_event_mid>(mid, offset);
   text.setData(data+offset, mid->text_length);
}

int ExtendedEventDescriptor::getDescriptorNumber() {
   return s->descriptor_number;
}

int ExtendedEventDescriptor::getLastDescriptorNumber() {
   return s->last_descriptor_number;
}

void ExtendedEventDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_extended_event>(s, offset);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
   itemLoop.setDataAndOffset(data+offset, s->length_of_items, offset);
   const descr_extended_event_mid *mid;
   data.setPointerAndOffset<const descr_extended_event_mid>(mid, offset);
   text.setData(data+offset, mid->text_length);
}

void ExtendedEventDescriptor::Item::Parse() {
   int offset=0;
   const item_extended_event *first;
   data.setPointerAndOffset<const item_extended_event>(first, offset);
   itemDescription.setDataAndOffset(data+offset, first->item_description_length, offset);
   const item_extended_event_mid *mid;
   data.setPointerAndOffset<const item_extended_event_mid>(mid, offset);
   item.setData(data+offset, mid->item_length);
}

/*int ExtendedEventDescriptors::getTextLength() {
   int ret=0;
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;
      ret+=d->text.getLength();
      ExtendedEventDescriptor::Item item;
      for (Loop::Iterator it; d->itemLoop.hasNext(it);   ) {
         item=d->itemLoop.getNext(it);
         ret+=item.item.getLength();
         ret+=item.itemDescription.getLength();
         ret+=2; //the blanks
      }
   }
   return ret;
}*/

int ExtendedEventDescriptors::getMaximumTextLength(const char *separation1, const char *separation2) {
   //add length of plain text, of itemized text with separators, and for one separator between the two fields.
   return getMaximumTextPlainLength()+getMaximumTextItemizedLength(separation1, separation2)+strlen(separation2);
}

char *ExtendedEventDescriptors::getText(const char *separation1, const char *separation2) {
   int size = getMaximumTextLength(separation1, separation2);
   char *text=new char[size];
   return getText(text, size, separation1, separation2);
}

char *ExtendedEventDescriptors::getText(char *buffer, int size, const char *separation1, const char *separation2) {
   int index=0, len;
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;
      d->text.getText(buffer+index, size);
      len = strlen(buffer+index);
      index += len;
      size -= len;
   }

   int sepLen1 = strlen(separation1);
   int sepLen2 = strlen(separation2);
   bool separated = false;
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;

      ExtendedEventDescriptor::Item item;
      for (Loop::Iterator it; d->itemLoop.getNext(item, it);   ) {
         if (!separated && size > sepLen2) {
            strcpy(buffer+index, separation2); // let's have a separator between the long text and the items
            index += sepLen2;
            size -= sepLen2;
            separated = true;
         }

         item.itemDescription.getText(buffer+index, size);
         len = strlen(buffer+index);
         index += len;
         size -= len;
         if (size > sepLen1) {
            strcpy(buffer+index, separation1);
            index += sepLen1;
            size -= sepLen1;
         }

         item.item.getText(buffer+index, size);
         len = strlen(buffer+index);
         index += len;
         size -= len;
         if (size > sepLen2) {
            strcpy(buffer+index, separation2);
            index += sepLen2;
            size -= sepLen2;
         }
      }
   }

   buffer[index]='\0';
   return buffer;
}

int ExtendedEventDescriptors::getMaximumTextPlainLength() {
   int ret=0;
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;
      ret+=d->text.getLength();
   }
   return ret;
}

char *ExtendedEventDescriptors::getTextPlain() {
   int size = getMaximumTextPlainLength();
   char *text=new char[size];
   return getTextPlain(text, size);
}

char *ExtendedEventDescriptors::getTextPlain(char *buffer, int size) {
   int index=0, len;
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;
      d->text.getText(buffer+index, size);
      len = strlen(buffer+index);
      index += len;
      size -= len;
   }
   buffer[index]='\0';
   return buffer;
}

int ExtendedEventDescriptors::getMaximumTextItemizedLength(const char *separation1, const char *separation2) {
   int ret=0;
   int sepLength=strlen(separation1)+strlen(separation2);
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;
      //The length includes two 8-bit length fields which have already been subtracted from sepLength //XXX kls 2004-06-06: what does this mean???
      ret+=d->itemLoop.getLength()+sepLength;
   }
   return ret;
}

char *ExtendedEventDescriptors::getTextItemized(const char *separation1, const char *separation2) {
   int size = getMaximumTextItemizedLength(separation1, separation2);
   char *text=new char[size];
   return getTextItemized(text, size, separation1, separation2);
}

char *ExtendedEventDescriptors::getTextItemized(char *buffer, int size, const char *separation1, const char *separation2) {
   int index=0, len;
   int sepLen1 = strlen(separation1);
   int sepLen2 = strlen(separation2);
   for (int i=0;i<length;i++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[i];
      if (!d)
         continue;

      ExtendedEventDescriptor::Item item;
      for (Loop::Iterator it; d->itemLoop.getNext(item, it);   ) {
         item.itemDescription.getText(buffer+index, size);
         len = strlen(buffer+index);
         index += len;
         size -= len;
         if (size > sepLen1) {
            strcpy(buffer+index, separation1);
            index += sepLen1;
            size -= sepLen1;
         }

         item.item.getText(buffer+index, size);
         len = strlen(buffer+index);
         index += len;
         size -= len;
         if (size > sepLen2) {
            strcpy(buffer+index, separation2);
            index += sepLen2;
            size -= sepLen2;
         }
      }
   }
   buffer[index]='\0';
   return buffer;
}

//returns the itemized text pair by pair. Maximum length for buffers is 256.
//Return value is false if and only if the end of the list is reached.
bool ExtendedEventDescriptors::getTextItemized(Loop::Iterator &it, bool &valid, char *itemDescription, char *itemText, int sizeItemDescription, int sizeItemText) {
   //The iterator has to store two values: The descriptor index (4bit)
   //and the item loop index (max overall length 256, min item length 16 => max number 128 => 7bit)
   valid=false;

   int index=(it.i & 0x780) >> 7; // 0x780 == 1111 000 0000
   it.i &= 0x7F; //0x7F == 111 1111

   for (;index<length;index++) {
      ExtendedEventDescriptor *d=(ExtendedEventDescriptor *)array[index];
      if (!d)
         continue;

      ExtendedEventDescriptor::Item item;
      if (d->itemLoop.getNext(item, it)) {
         item.item.getText(itemDescription, sizeItemDescription);
         item.itemDescription.getText(itemText, sizeItemText);
         valid=true;
         break;
      } else {
         it.reset();
         continue;
      }
   }

   it.i &= 0x7F;
   it.i |= (index & 0xF) << 7; //0xF == 1111

   return index<length;
}

int TimeShiftedEventDescriptor::getReferenceServiceId() const {
   return HILO(s->reference_service_id);
}

int TimeShiftedEventDescriptor::getReferenceEventId() const {
   return HILO(s->reference_event_id);
}

void TimeShiftedEventDescriptor::Parse() {
   s=data.getData<const descr_time_shifted_event>();
}

void ContentDescriptor::Parse() {
   //this descriptor is only a header and a loop
   nibbleLoop.setData(data+sizeof(descr_content), getLength()-sizeof(descr_content));
}

int ContentDescriptor::Nibble::getContentNibbleLevel1() const {
   return s->content_nibble_level_1;
}

int ContentDescriptor::Nibble::getContentNibbleLevel2() const {
   return s->content_nibble_level_2;
}

int ContentDescriptor::Nibble::getUserNibble1() const {
   return s->user_nibble_1;
}

int ContentDescriptor::Nibble::getUserNibble2() const {
   return s->user_nibble_2;
}

void ContentDescriptor::Nibble::Parse() {
   s=data.getData<const nibble_content>();
}

void ParentalRatingDescriptor::Parse() {
   //this descriptor is only a header and a loop
   ratingLoop.setData(data+sizeof(descr_parental_rating), getLength()-sizeof(descr_parental_rating));
}

int ParentalRatingDescriptor::Rating::getRating() const {
   return s->rating;
}

void ParentalRatingDescriptor::Rating::Parse() {
   s=data.getData<const parental_rating>();
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
}

void TeletextDescriptor::Parse() {
   //this descriptor is only a header and a loop
   teletextLoop.setData(data+sizeof(descr_teletext), getLength()-sizeof(descr_teletext));
}

void TeletextDescriptor::Teletext::Parse() {
   s=data.getData<const item_teletext>();
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
}

int TeletextDescriptor::Teletext::getTeletextType() const {
   return s->type;
}

int TeletextDescriptor::Teletext::getTeletextMagazineNumber() const {
   return s->magazine_number;
}

int TeletextDescriptor::Teletext::getTeletextPageNumber() const {
   return s->page_number;
}

int CaDescriptor::getCaType() const {
   return HILO(s->CA_type);
}

int CaDescriptor::getCaPid() const {
   return HILO(s->CA_PID);
}

void CaDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_ca>(s, offset);
   if (checkSize(getLength()-offset))
      privateData.assign(data.getData(offset), getLength()-offset);
}

int StreamIdentifierDescriptor::getComponentTag() const {
   return s->component_tag;
}

void StreamIdentifierDescriptor::Parse() {
   s=data.getData<const descr_stream_identifier>();
}

void NetworkNameDescriptor::Parse() {
   name.setData(data+sizeof(descr_network_name), getLength()-sizeof(descr_network_name));
}

void CaIdentifierDescriptor::Parse() {
   identifiers.setData(data+sizeof(descr_ca_identifier), getLength()-sizeof(descr_ca_identifier));
}

int CarouselIdentifierDescriptor::getCarouselId() const {
   return (HILO(s->carousel_id_hi) << 16) | HILO(s->carousel_id_lo);
}

int CarouselIdentifierDescriptor::getFormatId() const {
   return s->FormatId;
}

void CarouselIdentifierDescriptor::Parse() {
   s=data.getData<const descr_carousel_identifier>();
}

void ServiceListDescriptor::Parse() {
   serviceLoop.setData(data+sizeof(descr_service_list), getLength()-sizeof(descr_service_list));
}

int ServiceListDescriptor::Service::getServiceId() const {
   return HILO(s->service_id);
}

int ServiceListDescriptor::Service::getServiceType() const {
   return s->service_type;
}

void ServiceListDescriptor::Service::Parse() {
   s=data.getData<const descr_service_list_loop>();
}

int SatelliteDeliverySystemDescriptor::getFrequency() const {
   return (HILO(s->frequency_hi) << 16) | HILO(s->frequency_lo);
}

int SatelliteDeliverySystemDescriptor::getOrbitalPosition() const {
   return HILO(s->orbital_position);
}

int SatelliteDeliverySystemDescriptor::getWestEastFlag() const {
   return s->west_east_flag;
}

int SatelliteDeliverySystemDescriptor::getPolarization() const {
   return s->polarization;
}

int SatelliteDeliverySystemDescriptor::getModulation() const {
   return s->modulation;
}

int SatelliteDeliverySystemDescriptor::getSymbolRate() const {
   return (HILO(s->symbol_rate_hi) << 12) | (s->symbol_rate_lo_1 << 4) | s->symbol_rate_lo_2;
}

int SatelliteDeliverySystemDescriptor::getFecInner() const {
   return s->fec_inner;
}

void SatelliteDeliverySystemDescriptor::Parse() {
   s=data.getData<const descr_satellite_delivery_system>();
}

int CableDeliverySystemDescriptor::getFrequency() const {
   return (HILO(s->frequency_hi) << 16) | HILO(s->frequency_lo);
}

int CableDeliverySystemDescriptor::getFecOuter() const {
   return s->fec_outer;
}

int CableDeliverySystemDescriptor::getModulation() const {
   return s->modulation;
}

int CableDeliverySystemDescriptor::getSymbolRate() const {
   return (HILO(s->symbol_rate_hi) << 12) | (s->symbol_rate_lo_1 << 4) | s->symbol_rate_lo_2;
}

int CableDeliverySystemDescriptor::getFecInner() const {
   return s->fec_inner;
}

void CableDeliverySystemDescriptor::Parse() {
   s=data.getData<const descr_cable_delivery_system>();
}

int TerrestrialDeliverySystemDescriptor::getFrequency() const {
   return (HILO(s->frequency_hi) << 16) | HILO(s->frequency_lo);
}

int TerrestrialDeliverySystemDescriptor::getBandwidth() const {
   return s->bandwidth;
}

int TerrestrialDeliverySystemDescriptor::getConstellation() const {
   return s->constellation;
}

int TerrestrialDeliverySystemDescriptor::getHierarchy() const {
   return s->hierarchy;
}

int TerrestrialDeliverySystemDescriptor::getCodeRateHP() const {
   return s->code_rate_HP;
}

int TerrestrialDeliverySystemDescriptor::getCodeRateLP() const {
   return s->code_rate_LP;
}

int TerrestrialDeliverySystemDescriptor::getGuardInterval() const {
   return s->guard_interval;
}

int TerrestrialDeliverySystemDescriptor::getTransmissionMode() const {
   return s->transmission_mode;
}

bool TerrestrialDeliverySystemDescriptor::getOtherFrequency() const {
   return s->other_frequency_flag;
}

void TerrestrialDeliverySystemDescriptor::Parse() {
   s=data.getData<const descr_terrestrial_delivery>();
}

int ServiceDescriptor::getServiceType() const {
   return s->service_type;
}

void ServiceDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_service>(s, offset);
   providerName.setDataAndOffset(data+offset, s->provider_name_length, offset);
   const descr_service_mid *mid;
   data.setPointerAndOffset<const descr_service_mid>(mid, offset);
   serviceName.setData(data+offset, mid->service_name_length);
}

void NVODReferenceDescriptor::Parse() {
   serviceLoop.setData(data+sizeof(descr_nvod_reference), getLength()-sizeof(descr_nvod_reference));
}

int NVODReferenceDescriptor::Service::getTransportStream() const {
   return HILO(s->transport_stream_id);
}

int NVODReferenceDescriptor::Service::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int NVODReferenceDescriptor::Service::getServiceId() const {
   return HILO(s->service_id);
}

void NVODReferenceDescriptor::Service::Parse() {
   s=data.getData<const item_nvod_reference>();
}

int TimeShiftedServiceDescriptor::getReferenceServiceId() const {
   return HILO(s->reference_service_id);
}

void TimeShiftedServiceDescriptor::Parse() {
   s=data.getData<const descr_time_shifted_service>();
}

int ComponentDescriptor::getStreamContent() const {
   return s->stream_content;
}

int ComponentDescriptor::getComponentType() const {
   return s->component_type;
}

int ComponentDescriptor::getComponentTag() const {
   return s->component_tag;
}

void ComponentDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_component>(s, offset);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
   description.setData(data+offset, getLength()-offset);
}

void PrivateDataSpecifierDescriptor::Parse() {
   s=data.getData<const descr_private_data_specifier>();
}

int PrivateDataSpecifierDescriptor::getPrivateDataSpecifier() const {
   return (HILO(s->private_data_specifier_hi) << 16) | HILO(s->private_data_specifier_lo);
}

void SubtitlingDescriptor::Parse() {
   subtitlingLoop.setData(data+sizeof(descr_subtitling), getLength()-sizeof(descr_subtitling));
}

int SubtitlingDescriptor::Subtitling::getSubtitlingType() const {
   return s->subtitling_type;
}

int SubtitlingDescriptor::Subtitling::getCompositionPageId() const {
   return HILO(s->composition_page_id);
}

int SubtitlingDescriptor::Subtitling::getAncillaryPageId() const {
   return HILO(s->ancillary_page_id);
}

void SubtitlingDescriptor::Subtitling::Parse() {
   s=data.getData<const item_subtitling>();
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
}

int ServiceMoveDescriptor::getNewOriginalNetworkId() const {
   return HILO(s->new_original_network_id);
}

int ServiceMoveDescriptor::getNewTransportStreamId() const {
   return HILO(s->new_transport_stream_id);
}

int ServiceMoveDescriptor::getNewServiceId() const {
   return HILO(s->new_service_id);
}

void ServiceMoveDescriptor::Parse() {
   s=data.getData<const descr_service_move>();
}

int FrequencyListDescriptor::getCodingType() const {
   return s->coding_type;
}

void FrequencyListDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_frequency_list>(s, offset);
   frequencies.setData(data+offset, getLength()-offset);
}

void ServiceIdentifierDescriptor::Parse() {
   textualServiceIdentifier.setData(data+sizeof(descr_service_identifier), getLength()-sizeof(descr_service_identifier));
}

void MultilingualNameDescriptor::Parse() {
   nameLoop.setData(data+sizeof(descr_multilingual_network_name), getLength()-sizeof(descr_multilingual_network_name));
}

void MultilingualNameDescriptor::Name::Parse() {
   int offset=0;
   const entry_multilingual_name *s;
   data.setPointerAndOffset<const entry_multilingual_name>(s, offset);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
   name.setData(data+offset, s->text_length);
}

int MultilingualComponentDescriptor::getComponentTag() const {
   return s->component_tag;
}

void MultilingualComponentDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_multilingual_component>(s, offset);
   nameLoop.setData(data+sizeof(descr_multilingual_component), getLength()-sizeof(descr_multilingual_component));
}

void MultilingualServiceNameDescriptor::Parse() {
   nameLoop.setData(data+sizeof(descr_multilingual_network_name), getLength()-sizeof(descr_multilingual_network_name));
}

void MultilingualServiceNameDescriptor::Name::Parse() {
   int offset=0;
   const entry_multilingual_name *s;
   data.setPointerAndOffset<const entry_multilingual_name>(s, offset);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
   providerName.setDataAndOffset(data+offset, s->text_length, offset);
   const entry_multilingual_service_name_mid *mid;
   data.setPointerAndOffset<const entry_multilingual_service_name_mid>(mid, offset);
   name.setData(data+offset, mid->service_name_length);
}

void LocalTimeOffsetDescriptor::Parse() {
   localTimeOffsetLoop.setData(data+sizeof(descr_local_time_offset), getLength()-sizeof(descr_local_time_offset));
}

int LocalTimeOffsetDescriptor::LocalTimeOffset::getCountryId() const {
   return s->country_region_id;
}

int LocalTimeOffsetDescriptor::LocalTimeOffset::getLocalTimeOffsetPolarity() const {
   return s->local_time_offset_polarity;
}

int LocalTimeOffsetDescriptor::LocalTimeOffset::getLocalTimeOffset() const {
   return (s->local_time_offset_h << 8) | s->local_time_offset_m;
}

time_t LocalTimeOffsetDescriptor::LocalTimeOffset::getTimeOfChange() const {
   return DVBTime::getTime(s->time_of_change_mjd_hi, s->time_of_change_mjd_lo, s->time_of_change_time_h, s->time_of_change_time_m, s->time_of_change_time_s);
}

int LocalTimeOffsetDescriptor::LocalTimeOffset::getNextTimeOffset() const {
   return (s->next_time_offset_h << 8) | s->next_time_offset_m;
}

void LocalTimeOffsetDescriptor::LocalTimeOffset::Parse() {
   s=data.getData<const local_time_offset_entry>();
   countryCode[0]=s->country_code1;
   countryCode[1]=s->country_code2;
   countryCode[2]=s->country_code3;
   countryCode[3]=0;
}

void LinkageDescriptor::Parse() {
   int offset=0;
   s1 = NULL;
   data.setPointerAndOffset<const descr_linkage>(s, offset);
   if (checkSize(getLength()-offset)) {
      if (getLinkageType() == LinkageTypeMobileHandover)
         data.setPointerAndOffset<const descr_linkage_8>(s1, offset);
      privateData.assign(data.getData(offset), getLength()-offset);
      }
}

int LinkageDescriptor::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int LinkageDescriptor::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int LinkageDescriptor::getServiceId() const {
   return HILO(s->service_id);
}

LinkageType LinkageDescriptor::getLinkageType() const {
   return (LinkageType)s->linkage_type;
}

int LinkageDescriptor::getHandOverType() const {
   return s1 == NULL ? 0 : s1->hand_over_type;
}

int LinkageDescriptor::getOriginType() const {
   return s1 == NULL ? 0 : s1->origin_type;
}

int LinkageDescriptor::getId() const {
   return s1 == NULL ? 0 : HILO(s1->id);
}

void ISO639LanguageDescriptor::Parse() {
   languageLoop.setData(data+sizeof(descr_iso_639_language), getLength()-sizeof(descr_iso_639_language));

   //all this is for backwards compatibility only
   Loop::Iterator it;
   Language first;
   if (languageLoop.getNext(first, it)) {
      languageCode[0]=first.languageCode[0];
      languageCode[1]=first.languageCode[1];
      languageCode[2]=first.languageCode[2];
      languageCode[3]=0;
   } else
      languageCode[0]=0;
}

void ISO639LanguageDescriptor::Language::Parse() {
   s=data.getData<const descr_iso_639_language_loop>();
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
}

AudioType ISO639LanguageDescriptor::Language::getAudioType() {
   return (AudioType)s->audio_type;
}

void PDCDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_pdc>(s, offset);
}

int PDCDescriptor::getDay() const {
   return ((s->pil0 & 0x0F) << 1) | ((s->pil1 & 0x80) >> 7);
}

int PDCDescriptor::getMonth() const {
   return (s->pil1 >> 3) & 0x0F;
}

int PDCDescriptor::getHour() const {
   return ((s->pil1 & 0x07) << 2) | ((s->pil2 & 0xC0) >> 6);
}

int PDCDescriptor::getMinute() const {
   return s->pil2 & 0x3F;
}

void AncillaryDataDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_ancillary_data>(s, offset);
}

int AncillaryDataDescriptor::getAncillaryDataIdentifier() const {
   return s->ancillary_data_identifier;
}

int PremiereContentTransmissionDescriptor::getOriginalNetworkId() const {
   return HILO(s->original_network_id);
}

int PremiereContentTransmissionDescriptor::getTransportStreamId() const {
   return HILO(s->transport_stream_id);
}

int PremiereContentTransmissionDescriptor::getServiceId() const {
   return HILO(s->service_id);
}

void PremiereContentTransmissionDescriptor::Parse() {
   s=data.getData<const descr_premiere_content_transmission>();
   startDayLoop.setData(data+sizeof(descr_premiere_content_transmission), getLength()-sizeof(descr_premiere_content_transmission));
}

int PremiereContentTransmissionDescriptor::StartDayEntry::getMJD() const {
   return HILO(s->mjd);
}

int PremiereContentTransmissionDescriptor::StartDayEntry::getLoopLength() const {
   return s->start_time_loop;
}

int PremiereContentTransmissionDescriptor::StartDayEntry::getLength() {
   return sizeof(item_premiere_content_transmission_day)+getLoopLength();
}

void PremiereContentTransmissionDescriptor::StartDayEntry::Parse() {
   s=data.getData<const item_premiere_content_transmission_day>();
   startTimeLoop.setData(data+sizeof(item_premiere_content_transmission_day), getLoopLength());
}

time_t PremiereContentTransmissionDescriptor::StartDayEntry::StartTimeEntry::getStartTime(int mjd) const {
   return DVBTime::getTime(mjd >> 8, mjd & 0xff, s->start_time_h, s->start_time_m, s->start_time_s);
}

void PremiereContentTransmissionDescriptor::StartDayEntry::StartTimeEntry::Parse() {
   s=data.getData<const item_premiere_content_transmission_time>();
}

void ApplicationSignallingDescriptor::Parse() {
   entryLoop.setData(data+sizeof(descr_application_signalling), getLength()-sizeof(descr_application_signalling));
}

int ApplicationSignallingDescriptor::ApplicationEntryDescriptor::getApplicationType() const {
   return HILO(s->application_type);
}

int ApplicationSignallingDescriptor::ApplicationEntryDescriptor::getAITVersionNumber() const {
   return s->AIT_version_number;
}

void ApplicationSignallingDescriptor::ApplicationEntryDescriptor::Parse() {
   s=data.getData<const application_signalling_entry>();
}

bool MHP_ApplicationDescriptor::isServiceBound() const {
   return s->service_bound_flag;
}

int MHP_ApplicationDescriptor::getVisibility() const {
   return s->visibility;
}

int MHP_ApplicationDescriptor::getApplicationPriority() const {
   return s->application_priority;
}

void MHP_ApplicationDescriptor::Parse() {
   int offset=0;
   const descr_application *dapp;
   data.setPointerAndOffset<const descr_application>(dapp, offset);
   profileLoop.setDataAndOffset(data+offset, dapp->application_profiles_length, offset);
   data.setPointerAndOffset<const descr_application_end>(s, offset);
   transportProtocolLabels.setData(data+offset, getLength()-offset);
}

int MHP_ApplicationDescriptor::Profile::getApplicationProfile() const {
      return HILO(s->application_profile);
}

int MHP_ApplicationDescriptor::Profile::getVersionMajor() const {
      return s->version_major;
}

int MHP_ApplicationDescriptor::Profile::getVersionMinor() const {
      return s->version_minor;
}

int MHP_ApplicationDescriptor::Profile::getVersionMicro() const {
      return s->version_micro;
}

void MHP_ApplicationDescriptor::Profile::Parse() {
   s=data.getData<application_profile_entry>();
}

void MHP_ApplicationNameDescriptor::Parse() {
   nameLoop.setData(data+sizeof(descr_application_name), getLength()-sizeof(descr_application_name));
}

void MHP_ApplicationNameDescriptor::NameEntry::Parse() {
   const descr_application_name_entry *s;
   s=data.getData<const descr_application_name_entry>();
   name.setData(data+sizeof(descr_application_name_entry), s->application_name_length);
   languageCode[0]=s->lang_code1;
   languageCode[1]=s->lang_code2;
   languageCode[2]=s->lang_code3;
   languageCode[3]=0;
}

int MHP_TransportProtocolDescriptor::getProtocolId() const {
   return HILO(s->protocol_id);
}

int MHP_TransportProtocolDescriptor::getProtocolLabel() const {
   return s->transport_protocol_label;
}

bool MHP_TransportProtocolDescriptor::isRemote() const {
   return remote;
}

int MHP_TransportProtocolDescriptor::getComponentTag() const {
   return componentTag;
}

void MHP_TransportProtocolDescriptor::Parse() {
   int offset=0;
   data.setPointerAndOffset<const descr_transport_protocol>(s, offset);
   if (getProtocolId() == ObjectCarousel) {
      const transport_via_oc *oc;
      data.setPointerAndOffset<const transport_via_oc>(oc, offset);
      remote=oc->remote;
      if (remote) {
         const transport_via_oc_remote_end *rem;
         data.setPointerAndOffset<const transport_via_oc_remote_end>(rem, offset);
         componentTag=rem->component_tag;
      } else {
         const transport_via_oc_end *rem;
         data.setPointerAndOffset<const transport_via_oc_end>(rem, offset);
         componentTag=rem->component_tag;
      }
   } else { //unimplemented
      remote=false;
      componentTag=-1;
   }
}

void MHP_DVBJApplicationDescriptor::Parse() {
   applicationLoop.setData(data+sizeof(descr_dvbj_application), getLength()-sizeof(descr_dvbj_application));
}

void MHP_DVBJApplicationDescriptor::ApplicationEntry::Parse() {
   const descr_dvbj_application_entry *entry=data.getData<const descr_dvbj_application_entry>();
   parameter.setData(data+sizeof(descr_dvbj_application_entry), entry->parameter_length);
}

void MHP_DVBJApplicationLocationDescriptor::Parse() {
   int offset=0;
   const descr_dvbj_application_location *first;
   data.setPointerAndOffset<const descr_dvbj_application_location>(first, offset);
   baseDirectory.setDataAndOffset(data+offset, first->base_directory_length, offset);
   const descr_dvbj_application_location_mid *mid;
   data.setPointerAndOffset<const descr_dvbj_application_location_mid>(mid, offset);
   classPath.setDataAndOffset(data+offset, mid->classpath_extension_length, offset);
   initialClass.setData(data+offset, getLength()-offset);
}

int MHP_ApplicationIconsDescriptor::getIconFlags() const {
   return HILO(s->icon_flags);
}

void MHP_ApplicationIconsDescriptor::Parse() {
   int offset=0;
   const descr_application_icons_descriptor *first;
   data.setPointerAndOffset<const descr_application_icons_descriptor>(first, offset);
   iconLocator.setDataAndOffset(data+offset, first->icon_locator_length, offset);
   data.setPointerAndOffset<const descr_application_icons_descriptor_end>(s, offset);
}

} //end of namespace
