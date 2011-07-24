/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: descriptor.h 1.15 2006/05/28 14:25:30 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_DESCRIPTOR_H
#define LIBSI_DESCRIPTOR_H

#include "si.h"
#include "headers.h"

namespace SI {

class ShortEventDescriptor : public Descriptor {
public:
   char languageCode[4];
   String name; //name of the event
   String text; //short description
protected:
   virtual void Parse();
};

class ExtendedEventDescriptor : public GroupDescriptor {
public:
   class Item : public LoopElement {
   public:
      virtual int getLength() { return sizeof(item_extended_event)+sizeof(item_extended_event_mid)+item.getLength()+itemDescription.getLength(); }
      String item;
      String itemDescription;
   protected:
      virtual void Parse();
   };
   char languageCode[4];
   int getDescriptorNumber();
   int getLastDescriptorNumber();
   StructureLoop<Item> itemLoop;
   String text;
protected:
   virtual void Parse();
private:
   const descr_extended_event *s;
};

class ExtendedEventDescriptors : public DescriptorGroup {
public:
   int getMaximumTextLength(const char *separation1="\t", const char *separation2="\n");
   //Returns a concatenated version of first the non-itemized and then the itemized text
   //same semantics as with SI::String
   char *getText(const char *separation1="\t", const char *separation2="\n");
   //buffer must at least be getTextLength(), getMaximumTextLength() is a good choice
   char *getText(char *buffer, int size, const char *separation1="\t", const char *separation2="\n");

   //these only return the non-itemized text fields in concatenated form
   int getMaximumTextPlainLength();
   char *getTextPlain();
   char *getTextPlain(char *buffer, int size);

   //these only return the itemized text fields in concatenated form.
   //Between the description and the text the separation1 character is used,
   //separation2 used between two pairs. Example:
   //Director\tSteven Spielberg\nActor\tMichael Mendl\n
   int getMaximumTextItemizedLength(const char *separation1="\t", const char *separation2="\n");
   char *getTextItemized(const char *separation1="\t", const char *separation2="\n");
   char *getTextItemized(char *buffer, int size, const char *separation1="\t", const char *separation2="\n");
   //returns the itemized text pair by pair. Maximum length for buffers is 256.
   //Return value is false if and only if the end of the list is reached.
   //The argument valid indicates whether the buffers contain valid content.
   bool getTextItemized(Loop::Iterator &it, bool &valid, char *itemDescription, char *itemText, int sizeItemDescription, int sizeItemText);
};

class TimeShiftedEventDescriptor : public Descriptor {
public:
   int getReferenceServiceId() const;
   int getReferenceEventId() const;
protected:
   virtual void Parse();
private:
   const descr_time_shifted_event *s;
};

class ContentDescriptor : public Descriptor {
public:
   class Nibble : public LoopElement {
   public:
      virtual int getLength() { return sizeof(nibble_content); }
      int getContentNibbleLevel1() const;
      int getContentNibbleLevel2() const;
      int getUserNibble1() const;
      int getUserNibble2() const;
   protected:
      virtual void Parse();
   private:
      const nibble_content *s;
   };
   StructureLoop<Nibble> nibbleLoop;
protected:
   virtual void Parse();
};

class ParentalRatingDescriptor : public Descriptor {
public:
   class Rating : public LoopElement {
   public:
      char languageCode[4];
      int getRating() const;
      virtual int getLength() { return sizeof(parental_rating); }
   protected:
      virtual void Parse();
   private:
      const parental_rating *s;
   };
   StructureLoop<Rating> ratingLoop;
protected:
   virtual void Parse();
};

class TeletextDescriptor : public Descriptor {
public:
   class Teletext : public LoopElement {
   public:
      char languageCode[4];
      int getTeletextType() const;
      int getTeletextMagazineNumber() const;
      int getTeletextPageNumber() const;
      virtual int getLength() { return sizeof(item_teletext); }
   protected:
      virtual void Parse();
   private:
      const item_teletext *s;
   };
   StructureLoop<Teletext> teletextLoop;
protected:
   virtual void Parse();
};

class CaDescriptor : public Descriptor {
public:
   int getCaType() const;
   int getCaPid() const;
   CharArray privateData;
protected:
   virtual void Parse();
private:
   const descr_ca *s;
};

class StreamIdentifierDescriptor : public Descriptor {
public:
   int getComponentTag() const;
protected:
   virtual void Parse();
private:
   const descr_stream_identifier *s;
};

class NetworkNameDescriptor : public Descriptor {
public:
   String name;
protected:
   virtual void Parse();
};

class CaIdentifierDescriptor : public Descriptor {
public:
   TypeLoop<SixteenBit> identifiers;
protected:
   virtual void Parse();
};

class CarouselIdentifierDescriptor : public Descriptor {
public:
   int getCarouselId() const;
   int getFormatId() const;
protected:
   virtual void Parse();
private:
   const descr_carousel_identifier *s;
};

class BouquetNameDescriptor : public NetworkNameDescriptor {
};

class ServiceListDescriptor : public Descriptor {
public:
   class Service : public LoopElement {
   public:
      int getServiceId() const;
      int getServiceType() const;
   virtual int getLength() { return sizeof(descr_service_list_loop); }
   protected:
      virtual void Parse();
   private:
      const descr_service_list_loop *s;
   };
   StructureLoop<Service> serviceLoop;
protected:
   virtual void Parse();
};

class SatelliteDeliverySystemDescriptor : public Descriptor {
public:
   int getFrequency() const;
   int getOrbitalPosition() const;
   int getWestEastFlag() const;
   int getPolarization() const;
   int getModulation() const;
   int getSymbolRate() const;
   int getFecInner() const;
protected:
   virtual void Parse();
private:
   const descr_satellite_delivery_system *s;
};

class CableDeliverySystemDescriptor : public Descriptor {
public:
   int getFrequency() const;
   int getFecOuter() const;
   int getModulation() const;
   int getSymbolRate() const;
   int getFecInner() const;
protected:
   virtual void Parse();
private:
   const descr_cable_delivery_system *s;
};

class TerrestrialDeliverySystemDescriptor : public Descriptor {
public:
   int getFrequency() const;
   int getBandwidth() const;
   int getConstellation() const;
   int getHierarchy() const;
   int getCodeRateHP() const;
   int getCodeRateLP() const;
   int getGuardInterval() const;
   int getTransmissionMode() const;
   bool getOtherFrequency() const;
protected:
   virtual void Parse();
private:
   const descr_terrestrial_delivery *s;
};

class ServiceDescriptor : public Descriptor {
public:
   int getServiceType() const;
   String serviceName;
   String providerName;
protected:
   virtual void Parse();
private:
   const descr_service *s;
};

class NVODReferenceDescriptor : public Descriptor {
public:
   class Service : public LoopElement {
   public:
      int getTransportStream() const;
      int getOriginalNetworkId() const;
      int getServiceId() const;
      virtual int getLength() { return sizeof(item_nvod_reference); }
   protected:
      virtual void Parse();
   private:
      const item_nvod_reference *s;
   };
   StructureLoop<Service> serviceLoop;
protected:
   virtual void Parse();
};

class TimeShiftedServiceDescriptor : public Descriptor {
public:
   int getReferenceServiceId() const;
protected:
   virtual void Parse();
private:
   const descr_time_shifted_service *s;
};

class ComponentDescriptor : public Descriptor {
public:
   int getStreamContent() const;
   int getComponentType() const;
   int getComponentTag() const;
   char languageCode[4];
   String description;
protected:
   virtual void Parse();
private:
   const descr_component *s;
};

class PrivateDataSpecifierDescriptor : public Descriptor {
public:
   int getPrivateDataSpecifier() const;
protected:
   virtual void Parse();
private:
   const descr_private_data_specifier *s;
};

class SubtitlingDescriptor : public Descriptor {
public:
   class Subtitling : public LoopElement {
   public:
      char languageCode[4];
      int getSubtitlingType() const;
      int getCompositionPageId() const;
      int getAncillaryPageId() const;
      virtual int getLength() { return sizeof(item_subtitling); }
   protected:
      virtual void Parse();
   private:
      const item_subtitling *s;
   };
   StructureLoop<Subtitling> subtitlingLoop;
protected:
   virtual void Parse();
};

class ServiceMoveDescriptor : public Descriptor {
public:
   int getNewOriginalNetworkId() const;
   int getNewTransportStreamId() const;
   int getNewServiceId() const;
protected:
   virtual void Parse();
private:
   const descr_service_move *s;
};

class FrequencyListDescriptor : public Descriptor {
public:
   int getCodingType() const;
   TypeLoop<ThirtyTwoBit> frequencies;
protected:
   virtual void Parse();
private:
   const descr_frequency_list *s;
};

class ServiceIdentifierDescriptor : public Descriptor {
public:
   String textualServiceIdentifier;
protected:
   virtual void Parse();
};

//abstract base class
class MultilingualNameDescriptor : public Descriptor {
public:
   class Name : public LoopElement {
   public:
      char languageCode[4];
      String name;
      virtual int getLength() { return sizeof(entry_multilingual_name)+name.getLength(); }
   protected:
      virtual void Parse();
   };
   StructureLoop<Name> nameLoop;
protected:
   virtual void Parse();
};

class MultilingualNetworkNameDescriptor : public MultilingualNameDescriptor {
   //inherits nameLoop from MultilingualNameDescriptor
};

class MultilingualBouquetNameDescriptor : public MultilingualNameDescriptor {
   //inherits nameLoop from MultilingualNameDescriptor
};

class MultilingualComponentDescriptor : public MultilingualNameDescriptor {
public:
   int getComponentTag() const;
   //inherits nameLoop from MultilingualNameDescriptor
protected:
   virtual void Parse();
private:
   const descr_multilingual_component *s;
};

class MultilingualServiceNameDescriptor : public Descriptor {
public:
   class Name : public MultilingualNameDescriptor::Name {
   public:
      virtual int getLength() { return sizeof(entry_multilingual_name)+providerName.getLength()+sizeof(entry_multilingual_service_name_mid)+name.getLength(); }
      String providerName;
      //inherits name, meaning: service name;
   protected:
      virtual void Parse();
   };
   StructureLoop<Name> nameLoop;
protected:
   virtual void Parse();
};

class LocalTimeOffsetDescriptor : public Descriptor {
public:
   class LocalTimeOffset : public LoopElement {
   public:
      char countryCode[4];
      virtual int getLength() { return sizeof(local_time_offset_entry); }
      int getCountryId() const;
      int getLocalTimeOffsetPolarity() const;
      int getLocalTimeOffset() const;
      time_t getTimeOfChange() const;
      int getNextTimeOffset() const;
   protected:
      virtual void Parse();
   private:
      const local_time_offset_entry *s;
   };
   StructureLoop<LocalTimeOffset> localTimeOffsetLoop;
protected:
   virtual void Parse();
};

class LinkageDescriptor : public Descriptor {
public:
   int getTransportStreamId() const;
   int getOriginalNetworkId() const;
   int getServiceId() const;
   LinkageType getLinkageType() const;
   int getHandOverType() const;
   int getOriginType() const;
   int getId() const;
   CharArray privateData;
protected:
   virtual void Parse();
private:
   const descr_linkage *s;
   const descr_linkage_8 *s1;
};

class ISO639LanguageDescriptor : public Descriptor {
public:
   char languageCode[4]; //for backwards compatibility
   class Language : public LoopElement {
   public:
      virtual int getLength() { return sizeof(descr_iso_639_language_loop); }
      char languageCode[4];
      AudioType getAudioType();
   protected:
      virtual void Parse();
   private:
      const descr_iso_639_language_loop *s;
   };
   StructureLoop<Language> languageLoop;
protected:
   virtual void Parse();
private:
   const descr_iso_639_language *s;
};

class PDCDescriptor : public Descriptor {
public:
   int getDay() const;
   int getMonth() const;
   int getHour() const;
   int getMinute() const;
protected:
   virtual void Parse();
private:
   const descr_pdc *s;
};

class AncillaryDataDescriptor : public Descriptor {
public:
   int getAncillaryDataIdentifier() const;
protected:
   virtual void Parse();
private:
   const descr_ancillary_data *s;
};

// Private DVB Descriptor  Premiere.de
// 0xF2  Content Transmission Descriptor
// http://dvbsnoop.sourceforge.net/examples/example-private-section.html

class PremiereContentTransmissionDescriptor : public Descriptor {
public:
   class StartDayEntry : public LoopElement {
   public:
      class StartTimeEntry : public LoopElement {
      public:
         virtual int getLength() { return sizeof(item_premiere_content_transmission_time); }
         time_t getStartTime(int mjd) const; //UTC
      protected:
         virtual void Parse();
      private:
         const item_premiere_content_transmission_time *s;
      };
      StructureLoop<StartTimeEntry> startTimeLoop;
      virtual int getLength();
      int getMJD() const;
      int getLoopLength() const;
   protected:
      virtual void Parse();
   private:
      const item_premiere_content_transmission_day *s;
   };
   StructureLoop<StartDayEntry> startDayLoop;
   int getOriginalNetworkId() const;
   int getTransportStreamId() const;
   int getServiceId() const;
protected:
   virtual void Parse();
private:
   const descr_premiere_content_transmission *s;
};

//a descriptor currently unimplemented in this library
class UnimplementedDescriptor : public Descriptor {
protected:
   virtual void Parse() {}
};

class ApplicationSignallingDescriptor : public Descriptor {
public:
   class ApplicationEntryDescriptor : public LoopElement {
   public:
      virtual int getLength() { return sizeof(application_signalling_entry); }
      int getApplicationType() const;
      int getAITVersionNumber() const;
   protected:
      virtual void Parse();
   private:
      const application_signalling_entry *s;
   };
   StructureLoop<ApplicationEntryDescriptor> entryLoop;
protected:
   virtual void Parse();
};

class MHP_ApplicationDescriptor : public Descriptor {
public:
   class Profile : public LoopElement {
   public:
      virtual int getLength() { return sizeof(application_profile_entry); }
      int getApplicationProfile() const;
      int getVersionMajor() const;
      int getVersionMinor() const;
      int getVersionMicro() const;
   private:
      const application_profile_entry *s;
   protected:
      virtual void Parse();
   };
   StructureLoop<Profile> profileLoop;
   bool isServiceBound() const;
   int getVisibility() const;
   int getApplicationPriority() const;
   TypeLoop<EightBit> transportProtocolLabels;
private:
   const descr_application_end *s;
protected:
   virtual void Parse();
};

class MHP_ApplicationNameDescriptor : public Descriptor {
public:
   class NameEntry : public LoopElement {
   public:
      virtual int getLength() { return sizeof(descr_application_name_entry)+name.getLength(); }
      char languageCode[4];
      String name;
   protected:
      virtual void Parse();
   };
   StructureLoop<NameEntry> nameLoop;
protected:
   virtual void Parse();
};

class MHP_TransportProtocolDescriptor : public Descriptor {
public:
   enum Protocol { ObjectCarousel = 0x01, IPviaDVB = 0x02, HTTPoverInteractionChannel = 0x03 };
   int getProtocolId() const;
   int getProtocolLabel() const;
   bool isRemote() const;
   int getComponentTag() const;
protected:
   virtual void Parse();
private:
   const descr_transport_protocol *s;
   bool remote;
   int componentTag;
};

class MHP_DVBJApplicationDescriptor : public Descriptor {
public:
   class ApplicationEntry : public LoopElement {
   public:
      virtual int getLength() { return sizeof(descr_dvbj_application_entry)+parameter.getLength(); }
      String parameter;
   protected:
      virtual void Parse();
   };
   StructureLoop<ApplicationEntry> applicationLoop;
protected:
   virtual void Parse();
};

class MHP_DVBJApplicationLocationDescriptor : public Descriptor {
public:
   String baseDirectory;
   String classPath;
   String initialClass;
protected:
   virtual void Parse();
};

class MHP_ApplicationIconsDescriptor : public Descriptor {
public:
   String iconLocator;
   int getIconFlags() const;
protected:
   virtual void Parse();
private:
   const descr_application_icons_descriptor_end *s;
};

} //end of namespace

#endif //LIBSI_TABLE_H
