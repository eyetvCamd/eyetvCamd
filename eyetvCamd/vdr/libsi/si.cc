/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: si.c 1.16 2006/04/14 10:53:44 kls Exp $
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include "si.h"
#include "descriptor.h"

namespace SI {

Object::Object() {
}

Object::Object(CharArray &d) : data(d) {
}

void Object::setData(const unsigned char*d, int size, bool doCopy) {
   data.assign(d, size, doCopy);
}

void Object::setData(CharArray &d) {
   data=d;
}

bool Object::checkSize(int offset) {
   return data.checkSize(offset);
}

Section::Section(const unsigned char *data, bool doCopy) {
   setData(data, getLength(data), doCopy);
}

TableId Section::getTableId() const {
   return getTableId(data.getData());
}

int Section::getLength() {
   return getLength(data.getData());
}

TableId Section::getTableId(const unsigned char *d) {
   return (TableId)((const SectionHeader *)d)->table_id;
}

int Section::getLength(const unsigned char *d) {
   return HILO(((const SectionHeader *)d)->section_length)+sizeof(SectionHeader);
}

bool CRCSection::isCRCValid() {
   return CRC32::isValid((const char *)data.getData(), getLength()/*, data.FourBytes(getLength()-4)*/);
}

bool CRCSection::CheckCRCAndParse() {
   if (!isCRCValid())
      return false;
   CheckParse();
   return isValid();
}

int NumberedSection::getTableIdExtension() const {
   return getTableIdExtension(data.getData());
}

int NumberedSection::getTableIdExtension(const unsigned char *d) {
   return HILO(((const ExtendedSectionHeader *)d)->table_id_extension);
}

bool NumberedSection::getCurrentNextIndicator() const {
   return data.getData<ExtendedSectionHeader>()->current_next_indicator;
}

int NumberedSection::getVersionNumber() const {
   return data.getData<ExtendedSectionHeader>()->version_number;
}

int NumberedSection::getSectionNumber() const {
   return data.getData<ExtendedSectionHeader>()->section_number;
}

int NumberedSection::getLastSectionNumber() const {
   return data.getData<ExtendedSectionHeader>()->last_section_number;
}

int Descriptor::getLength() {
   return getLength(data.getData());
}

DescriptorTag Descriptor::getDescriptorTag() const {
   return getDescriptorTag(data.getData());
}

int Descriptor::getLength(const unsigned char *d) {
   return ((const DescriptorHeader*)d)->descriptor_length+sizeof(DescriptorHeader);
}

DescriptorTag Descriptor::getDescriptorTag(const unsigned char *d) {
   return (DescriptorTag)((const DescriptorHeader*)d)->descriptor_tag;
}

Descriptor *DescriptorLoop::getNext(Iterator &it) {
   if (isValid() && it.i<getLength()) {
      return createDescriptor(it.i, true);
   }
   return 0;
}

Descriptor *DescriptorLoop::getNext(Iterator &it, DescriptorTag tag, bool returnUnimplemetedDescriptor) {
   Descriptor *d=0;
   int len;
   if (isValid() && it.i<(len=getLength())) {
      const unsigned char *p=data.getData(it.i);
      const unsigned char *end=p+len-it.i;
      while (p < end) {
         if (Descriptor::getDescriptorTag(p) == tag) {
            d=createDescriptor(it.i, returnUnimplemetedDescriptor);
            if (d)
               break;
         }
         it.i+=Descriptor::getLength(p);
         p+=Descriptor::getLength(p);
      }
   }
   return d;
}

Descriptor *DescriptorLoop::getNext(Iterator &it, DescriptorTag *tags, int arrayLength, bool returnUnimplementedDescriptor) {
   Descriptor *d=0;
   int len;
   if (isValid() && it.i<(len=getLength())) {
      const unsigned char *p=data.getData(it.i);
      const unsigned char *end=p+len-it.i;
      while (p < end) {
         for (int u=0; u<arrayLength;u++)
            if (Descriptor::getDescriptorTag(p) == tags[u]) {
               d=createDescriptor(it.i, returnUnimplementedDescriptor);
               break;
            }
         if (d)
            break; //length is added to it.i by createDescriptor, break here
         it.i+=Descriptor::getLength(p);
         p+=Descriptor::getLength(p);
      }
   }
   return d;
}

Descriptor *DescriptorLoop::createDescriptor(int &i, bool returnUnimplemetedDescriptor) {
   if (!checkSize(Descriptor::getLength(data.getData(i))))
      return 0;
   Descriptor *d=Descriptor::getDescriptor(data+i, domain, returnUnimplemetedDescriptor);
   if (!d)
      return 0;
   i+=d->getLength();
   d->CheckParse();
   return d;
}

int DescriptorLoop::getNumberOfDescriptors() {
   const unsigned char *p=data.getData();
   const unsigned char *end=p+getLength();
   int count=0;
   while (p < end) {
      count++;
      p+=Descriptor::getLength(p);
   }
   return count;
}

DescriptorGroup::DescriptorGroup(bool del) {
   array=0;
   length=0;
   deleteOnDesctruction=del;
}

DescriptorGroup::~DescriptorGroup() {
   if (deleteOnDesctruction)
      Delete();
   delete[] array;
}

void DescriptorGroup::Delete() {
   for (int i=0;i<length;i++)
      if (array[i]!=0) {
         delete array[i];
         array[i]=0;
      }
}

void DescriptorGroup::Add(GroupDescriptor *d) {
   if (!array) {
      length=d->getLastDescriptorNumber()+1;
      array=new GroupDescriptor*[length]; //numbering is zero-based
      for (int i=0;i<length;i++)
         array[i]=0;
   } else if (length != d->getLastDescriptorNumber()+1)
      return; //avoid crash in case of misuse
   array[d->getDescriptorNumber()]=d;
}

bool DescriptorGroup::isComplete() {
   for (int i=0;i<length;i++)
      if (array[i]==0)
         return false;
   return true;
}

char *String::getText() {
   int len=getLength();
   if (len < 0 || len > 4095)
      return strdup("text error"); // caller will delete it!
   char *data=new char(len+1);
   decodeText(data, len+1);
   return data;
}

char *String::getText(char *buffer, int size) {
   int len=getLength();
   if (len < 0 || len >= size) {
      strncpy(buffer, "text error", size);
      buffer[size-1] = 0;
      return buffer;
   }
   decodeText(buffer, size);
   return buffer;
}

//taken from VDR, Copyright Klaus Schmidinger <kls@cadsoft.de>
char *String::getText(char *buffer, char *shortVersion, int sizeBuffer, int sizeShortVersion) {
   int len=getLength();
   if (len < 0 || len >= sizeBuffer) {
      strncpy(buffer, "text error", sizeBuffer);
      buffer[sizeBuffer-1] = 0;
      *shortVersion = 0;
      return buffer;
   }
   decodeText(buffer, shortVersion, sizeBuffer, sizeShortVersion);
   return buffer;
}

//taken from libdtv, Copyright Rolf Hakenes <hakenes@hippomi.de>
void String::decodeText(char *buffer, int size) {
   const unsigned char *from=data.getData(0);
   char *to=buffer;

   /* Disable detection of coding tables - libdtv doesn't do it either
   if ( (0x01 <= *from) && (*from <= 0x1f) ) {
      codeTable=*from
   }
   */

   if (*from == 0x10)
      from += 3; // skips code table info

   int len=getLength();
   for (int i = 0; i < len; i++) {
      if (*from == 0)
         break;
      if (    ((' ' <= *from) && (*from <= '~'))
           || (*from == '\n')
           || (0xA0 <= *from)
           || (*from == 0x86 || *from == 0x87)
         )
         *to++ = *from;
      else if (*from == 0x8A)
         *to++ = '\n';
      from++;
      if (to - buffer >= size - 1)
         break;
   }
   *to = '\0';
}

void String::decodeText(char *buffer, char *shortVersion, int sizeBuffer, int sizeShortVersion) {
   const unsigned char *from=data.getData(0);
   char *to=buffer;
   char *toShort=shortVersion;
   int IsShortName=0;

   if (*from == 0x10)
      from += 3; // skips code table info

   int len=getLength();
   for (int i = 0; i < len; i++) {
      if (    ((' ' <= *from) && (*from <= '~'))
           || (*from == '\n')
           || (0xA0 <= *from)
         )
      {
         *to++ = *from;
         if (IsShortName)
            *toShort++ = *from;
      }
      else if (*from == 0x8A)
         *to++ = '\n';
      else if (*from == 0x86)
         IsShortName++;
      else if (*from == 0x87)
         IsShortName--;
      else if (*from == 0)
         break;
      from++;
      if (to - buffer >= sizeBuffer - 1 || toShort - shortVersion >= sizeShortVersion - 1)
         break;
   }
   *to = '\0';
   *toShort = '\0';
}

Descriptor *Descriptor::getDescriptor(CharArray da, DescriptorTagDomain domain, bool returnUnimplemetedDescriptor) {
   Descriptor *d=0;
   switch (domain) {
   case SI:
      switch ((DescriptorTag)da.getData<DescriptorHeader>()->descriptor_tag) {
         case CaDescriptorTag:
            d=new CaDescriptor();
            break;
         case CarouselIdentifierDescriptorTag:
            d=new CarouselIdentifierDescriptor();
            break;
         case NetworkNameDescriptorTag:
            d=new NetworkNameDescriptor();
            break;
         case ServiceListDescriptorTag:
            d=new ServiceListDescriptor();
            break;
         case SatelliteDeliverySystemDescriptorTag:
            d=new SatelliteDeliverySystemDescriptor();
            break;
         case CableDeliverySystemDescriptorTag:
            d=new CableDeliverySystemDescriptor();
            break;
         case TerrestrialDeliverySystemDescriptorTag:
            d=new TerrestrialDeliverySystemDescriptor();
            break;
         case BouquetNameDescriptorTag:
            d=new BouquetNameDescriptor();
            break;
         case ServiceDescriptorTag:
            d=new ServiceDescriptor();
            break;
         case NVODReferenceDescriptorTag:
            d=new NVODReferenceDescriptor();
            break;
         case TimeShiftedServiceDescriptorTag:
            d=new TimeShiftedServiceDescriptor();
            break;
         case ComponentDescriptorTag:
            d=new ComponentDescriptor();
            break;
         case StreamIdentifierDescriptorTag:
            d=new StreamIdentifierDescriptor();
            break;
         case SubtitlingDescriptorTag:
            d=new SubtitlingDescriptor();
            break;
         case MultilingualNetworkNameDescriptorTag:
            d=new MultilingualNetworkNameDescriptor();
            break;
         case MultilingualBouquetNameDescriptorTag:
            d=new MultilingualBouquetNameDescriptor();
            break;
         case MultilingualServiceNameDescriptorTag:
            d=new MultilingualServiceNameDescriptor();
            break;
         case MultilingualComponentDescriptorTag:
            d=new MultilingualComponentDescriptor();
            break;
         case PrivateDataSpecifierDescriptorTag:
            d=new PrivateDataSpecifierDescriptor();
            break;
         case ServiceMoveDescriptorTag:
            d=new ServiceMoveDescriptor();
            break;
         case FrequencyListDescriptorTag:
            d=new FrequencyListDescriptor();
            break;
         case ServiceIdentifierDescriptorTag:
            d=new ServiceIdentifierDescriptor();
            break;
         case CaIdentifierDescriptorTag:
            d=new CaIdentifierDescriptor();
            break;
         case ShortEventDescriptorTag:
            d=new ShortEventDescriptor();
            break;
         case ExtendedEventDescriptorTag:
            d=new ExtendedEventDescriptor();
            break;
         case TimeShiftedEventDescriptorTag:
            d=new TimeShiftedEventDescriptor();
            break;
         case ContentDescriptorTag:
            d=new ContentDescriptor();
            break;
         case ParentalRatingDescriptorTag:
            d=new ParentalRatingDescriptor();
            break;
         case TeletextDescriptorTag:
         case VBITeletextDescriptorTag:
            d=new TeletextDescriptor();
            break;
         case ApplicationSignallingDescriptorTag:
            d=new ApplicationSignallingDescriptor();
            break;
         case LocalTimeOffsetDescriptorTag:
            d=new LocalTimeOffsetDescriptor();
            break;
         case LinkageDescriptorTag:
            d=new LinkageDescriptor();
            break;
         case ISO639LanguageDescriptorTag:
            d=new ISO639LanguageDescriptor();
            break;
         case PDCDescriptorTag:
            d=new PDCDescriptor();
            break;
         case AncillaryDataDescriptorTag:
            d=new AncillaryDataDescriptor();
            break;

         //note that it is no problem to implement one
         //of the unimplemented descriptors.

         //defined in ISO-13818-1
         case VideoStreamDescriptorTag:
         case AudioStreamDescriptorTag:
         case HierarchyDescriptorTag:
         case RegistrationDescriptorTag:
         case DataStreamAlignmentDescriptorTag:
         case TargetBackgroundGridDescriptorTag:
         case VideoWindowDescriptorTag:
         case SystemClockDescriptorTag:
         case MultiplexBufferUtilizationDescriptorTag:
         case CopyrightDescriptorTag:
         case MaximumBitrateDescriptorTag:
         case PrivateDataIndicatorDescriptorTag:
         case SmoothingBufferDescriptorTag:
         case STDDescriptorTag:
         case IBPDescriptorTag:

         //defined in ETSI EN 300 468
         case StuffingDescriptorTag:
         case VBIDataDescriptorTag:
         case CountryAvailabilityDescriptorTag:
         case MocaicDescriptorTag:
         case TelephoneDescriptorTag:
         case CellListDescriptorTag:
         case CellFrequencyLinkDescriptorTag:
         case ServiceAvailabilityDescriptorTag:
         case ShortSmoothingBufferDescriptorTag:
         case PartialTransportStreamDescriptorTag:
         case DataBroadcastDescriptorTag:
         case DataBroadcastIdDescriptorTag:
         case CaSystemDescriptorTag:
         case AC3DescriptorTag:
         case DSNGDescriptorTag:
         case AnnouncementSupportDescriptorTag:
         case AdaptationFieldDataDescriptorTag:
         case TransportStreamDescriptorTag:
         default:
            if (!returnUnimplemetedDescriptor)
               return 0;
            d=new UnimplementedDescriptor();
            break;
      }
      break;
   case MHP:
      switch ((DescriptorTag)da.getData<DescriptorHeader>()->descriptor_tag) {
      // They once again start with 0x00 (see page 234, MHP specification)
         case MHP_ApplicationDescriptorTag:
            d=new MHP_ApplicationDescriptor();
            break;
         case MHP_ApplicationNameDescriptorTag:
            d=new MHP_ApplicationNameDescriptor();
            break;
         case MHP_TransportProtocolDescriptorTag:
            d=new MHP_TransportProtocolDescriptor();
            break;
         case MHP_DVBJApplicationDescriptorTag:
            d=new MHP_DVBJApplicationDescriptor();
            break;
         case MHP_DVBJApplicationLocationDescriptorTag:
            d=new MHP_DVBJApplicationLocationDescriptor();
            break;
      // 0x05 - 0x0A is unimplemented this library
         case MHP_ExternalApplicationAuthorisationDescriptorTag:
         case MHP_IPv4RoutingDescriptorTag:
         case MHP_IPv6RoutingDescriptorTag:
         case MHP_DVBHTMLApplicationDescriptorTag:
         case MHP_DVBHTMLApplicationLocationDescriptorTag:
         case MHP_DVBHTMLApplicationBoundaryDescriptorTag:
         case MHP_ApplicationIconsDescriptorTag:
         case MHP_PrefetchDescriptorTag:
         case MHP_DelegatedApplicationDescriptorTag:
         case MHP_ApplicationStorageDescriptorTag:
         default:
            if (!returnUnimplemetedDescriptor)
               return 0;
            d=new UnimplementedDescriptor();
            break;
      }
      break;
   case PCIT:
      switch ((DescriptorTag)da.getData<DescriptorHeader>()->descriptor_tag) {
         case ContentDescriptorTag:
            d=new ContentDescriptor();
            break;
         case ShortEventDescriptorTag:
            d=new ShortEventDescriptor();
            break;
         case ExtendedEventDescriptorTag:
            d=new ExtendedEventDescriptor();
            break;
         case PremiereContentTransmissionDescriptorTag:
            d=new PremiereContentTransmissionDescriptor();
            break;
         default:
            if (!returnUnimplemetedDescriptor)
               return 0;
            d=new UnimplementedDescriptor();
            break;
      }
      break;
   }
   d->setData(da);
   return d;
}

} //end of namespace
