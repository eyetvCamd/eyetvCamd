/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: si.h 1.15 2006/05/27 13:07:20 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_SI_H
#define LIBSI_SI_H

#include <stdint.h>

#include "util.h"
#include "headers.h"

namespace SI {

enum TableId { TableIdPAT = 0x00, //program association section
               TableIdCAT = 0x01, //conditional access section
               TableIdPMT = 0x02, //program map section
               TableIdTSDT = 0x03,//transport stream description section
               TableIdNIT = 0x40, //network information, actual network section
               TableIdNIT_other  = 0x41, //network information section, other network
               TableIdSDT = 0x42, //service description section
               TableIdSDT_other  = 0x46,
               TableIdBAT = 0x46, //bouquet association section
               TableIdEIT_presentFollowing = 0x4E, //event information section
               TableIdEIT_presentFollowing_other = 0x4F,
               //range from 0x50 to 0x5F
               TableIdEIT_schedule_first = 0x50,
               TableIdEIT_schedule_last = 0x5F,
               //range from 0x60 to 0x6F
               TableIdEIT_schedule_Other_first = 0x60,
               TableIdEIT_schedule_Other_last = 0x6F,
               TableIdTDT = 0x70, //time date section
               TableIdRST = 0x71, //running status section
               TableIdST  = 0x72, //stuffing section
               TableIdTOT = 0x73, //time offset section
               TableIdDIT = 0x7E, //discontinuity information section
               TableIdSIT = 0x7F, //service information section
               TableIdAIT = 0x74, //application information section
               TableIdPremiereCIT = 0xA0 //premiere content information section
             };


enum DescriptorTag {
  // defined by ISO/IEC 13818-1
               VideoStreamDescriptorTag = 0x02,
               AudioStreamDescriptorTag = 0x03,
               HierarchyDescriptorTag = 0x04,
               RegistrationDescriptorTag = 0x05,
               DataStreamAlignmentDescriptorTag = 0x06,
               TargetBackgroundGridDescriptorTag = 0x07,
               VideoWindowDescriptorTag = 0x08,
               CaDescriptorTag = 0x09,
               ISO639LanguageDescriptorTag = 0x0A,
               SystemClockDescriptorTag = 0x0B,
               MultiplexBufferUtilizationDescriptorTag = 0x0C,
               CopyrightDescriptorTag = 0x0D,
               MaximumBitrateDescriptorTag = 0x0E,
               PrivateDataIndicatorDescriptorTag = 0x0F,
               SmoothingBufferDescriptorTag = 0x10,
               STDDescriptorTag = 0x11,
               IBPDescriptorTag = 0x12,
  // defined by ISO-13818-6 (DSM-CC)
               CarouselIdentifierDescriptorTag = 0x13,
               // 0x14 - 0x3F  Reserved
  // defined by ETSI (EN 300 468)
               NetworkNameDescriptorTag = 0x40,
               ServiceListDescriptorTag = 0x41,
               StuffingDescriptorTag = 0x42,
               SatelliteDeliverySystemDescriptorTag = 0x43,
               CableDeliverySystemDescriptorTag = 0x44,
               VBIDataDescriptorTag = 0x45,
               VBITeletextDescriptorTag = 0x46,
               BouquetNameDescriptorTag = 0x47,
               ServiceDescriptorTag = 0x48,
               CountryAvailabilityDescriptorTag = 0x49,
               LinkageDescriptorTag = 0x4A,
               NVODReferenceDescriptorTag = 0x4B,
               TimeShiftedServiceDescriptorTag = 0x4C,
               ShortEventDescriptorTag = 0x4D,
               ExtendedEventDescriptorTag = 0x4E,
               TimeShiftedEventDescriptorTag = 0x4F,
               ComponentDescriptorTag = 0x50,
               MocaicDescriptorTag = 0x51,
               StreamIdentifierDescriptorTag = 0x52,
               CaIdentifierDescriptorTag = 0x53,
               ContentDescriptorTag = 0x54,
               ParentalRatingDescriptorTag = 0x55,
               TeletextDescriptorTag = 0x56,
               TelephoneDescriptorTag = 0x57,
               LocalTimeOffsetDescriptorTag = 0x58,
               SubtitlingDescriptorTag = 0x59,
               TerrestrialDeliverySystemDescriptorTag = 0x5A,
               MultilingualNetworkNameDescriptorTag = 0x5B,
               MultilingualBouquetNameDescriptorTag = 0x5C,
               MultilingualServiceNameDescriptorTag = 0x5D,
               MultilingualComponentDescriptorTag = 0x5E,
               PrivateDataSpecifierDescriptorTag = 0x5F,
               ServiceMoveDescriptorTag = 0x60,
               ShortSmoothingBufferDescriptorTag = 0x61,
               FrequencyListDescriptorTag = 0x62,
               PartialTransportStreamDescriptorTag = 0x63,
               DataBroadcastDescriptorTag = 0x64,
               CaSystemDescriptorTag = 0x65,
               DataBroadcastIdDescriptorTag = 0x66,
               TransportStreamDescriptorTag = 0x67,
               DSNGDescriptorTag = 0x68,
               PDCDescriptorTag = 0x69,
               AC3DescriptorTag = 0x6A,
               AncillaryDataDescriptorTag = 0x6B,
               CellListDescriptorTag = 0x6C,
               CellFrequencyLinkDescriptorTag = 0x6D,
               AnnouncementSupportDescriptorTag = 0x6E,
               ApplicationSignallingDescriptorTag = 0x6F,
               AdaptationFieldDataDescriptorTag = 0x70,
               ServiceIdentifierDescriptorTag = 0x71,
               ServiceAvailabilityDescriptorTag = 0x72,
 // Defined by ETSI TS 102 812 (MHP)
               // They once again start with 0x00 (see page 234, MHP specification)
               MHP_ApplicationDescriptorTag = 0x00,
               MHP_ApplicationNameDescriptorTag = 0x01,
               MHP_TransportProtocolDescriptorTag = 0x02,
               MHP_DVBJApplicationDescriptorTag = 0x03,
               MHP_DVBJApplicationLocationDescriptorTag = 0x04,
               // 0x05 - 0x0A is unimplemented this library
               MHP_ExternalApplicationAuthorisationDescriptorTag = 0x05,
               MHP_IPv4RoutingDescriptorTag = 0x06,
               MHP_IPv6RoutingDescriptorTag = 0x07,
               MHP_DVBHTMLApplicationDescriptorTag = 0x08,
               MHP_DVBHTMLApplicationLocationDescriptorTag = 0x09,
               MHP_DVBHTMLApplicationBoundaryDescriptorTag = 0x0A,
               MHP_ApplicationIconsDescriptorTag = 0x0B,
               MHP_PrefetchDescriptorTag = 0x0C,
               MHP_DelegatedApplicationDescriptorTag = 0x0E,
               MHP_ApplicationStorageDescriptorTag = 0x10,
  // Premiere private Descriptor Tags
               PremiereContentTransmissionDescriptorTag = 0xF2,

               //a descriptor currently unimplemented in this library
               //the actual value 0xFF is "forbidden" according to the spec.
               UnimplementedDescriptorTag = 0xFF
};

enum DescriptorTagDomain { SI, MHP, PCIT };

enum RunningStatus { RunningStatusUndefined = 0,
                     RunningStatusNotRunning = 1,
                     RunningStatusStartsInAFewSeconds = 2,
                     RunningStatusPausing = 3,
                     RunningStatusRunning = 4
                   };

enum LinkageType { LinkageTypeInformationService = 0x01,
                   LinkageTypeEPGService = 0x02,
                   LinkageTypeCaReplacementService = 0x03,
                   LinkageTypeTSContainingCompleteNetworkBouquetSi = 0x04,
                   LinkageTypeServiceReplacementService = 0x05,
                   LinkageTypeDataBroadcastService = 0x06,
                   LinkageTypeRCSMap = 0x07,
                   LinkageTypeMobileHandover = 0x08,
                   LinkageTypeSystemSoftwareUpdateService = 0x09,
                   LinkageTypeTSContainingSsuBatOrNit = 0x0A
                 };

enum AudioType { AudioTypeUndefined = 0x00,
                 AudioTypeCleanEffects = 0x01,
                 AudioTypeHearingImpaired = 0x02,
                 AudioTypeVisualImpairedCommentary = 0x03
               };

/* Some principles:
   - Objects that return references to other objects contained in their data must make sure
     that the returned objects have been parsed.
     (the Loop subclasses take care of that.)
     Note that this does not apply to Loops and Strings (their are never returned by reference, BTW).
*/

class Object : public Parsable {
public:
   Object();
   Object(CharArray &d);
   //can only be called once since data is immutable
   void setData(const unsigned char*data, int size, bool doCopy=true);
   CharArray getData() { return data; }
   //returns the valid flag which indicates if data is all right or errors have been encountered
   bool isValid() { return data.isValid(); }
   virtual int getLength() = 0;
protected:
   CharArray data;
   //is protected - not used for sections
   template <class T> friend class StructureLoop;
   void setData(CharArray &d);
   //returns whether the given offset fits within the limits of the actual data
   //The valid flag will be set accordingly
   bool checkSize(int offset);
};

class Section : public Object {
public:
   //convenience: sets data and parses if doParse
   Section(const unsigned char *data, bool doCopy=true);
   Section() {}
   TableId getTableId() const;
   virtual int getLength();

   static int getLength(const unsigned char *d);
   static TableId getTableId(const unsigned char *d);
};

class CRCSection : public Section {
public:
   //convenience: sets data and parses if doParse
   CRCSection(const unsigned char *data, bool doCopy=true) : Section(data, doCopy) {}
   CRCSection() {}
   bool isCRCValid();
   //convenience: isValid+CheckParse
   bool CheckCRCAndParse();
};

/* A section which has the ExtendedSectionHeader
   (section_syntax_indicator==1) */
class NumberedSection : public CRCSection {
public:
   NumberedSection(const unsigned char *data, bool doCopy=true) : CRCSection(data, doCopy) {}
   NumberedSection() {}
   int getTableIdExtension() const;
   bool getCurrentNextIndicator() const;
   int getVersionNumber() const;
   int getSectionNumber() const;
   int getLastSectionNumber() const;
   bool moreThanOneSection()  const { return getLastSectionNumber()>0; }

   static int getTableIdExtension(const unsigned char *d);
};

class VariableLengthPart : public Object {
public:
   //never forget to call this
   void setData(CharArray d, int l) { Object::setData(d); checkSize(l); length=l; }
   //convenience method
   void setDataAndOffset(CharArray d, int l, int &offset) { Object::setData(d); checkSize(l); length=l; offset+=l; }
   virtual int getLength() { return length; }
private:
   int length;
};

class LoopElement : public Object {
};

class Descriptor : public LoopElement {
public:
   virtual int getLength();
   DescriptorTag getDescriptorTag() const;

   static int getLength(const unsigned char *d);
   static DescriptorTag getDescriptorTag(const unsigned char *d);
protected:
   friend class DescriptorLoop;
   //returns a subclass of descriptor according to the data given.
   //The object is allocated with new and must be delete'd.
   //setData() will have been called, CheckParse() not.
   //if returnUnimplemetedDescriptor==true:
   //   Never returns null - maybe the UnimplementedDescriptor.
   //if returnUnimplemetedDescriptor==false:
   //   Never returns the UnimplementedDescriptor - maybe null
   static Descriptor *getDescriptor(CharArray d, DescriptorTagDomain domain, bool returnUnimplemetedDescriptor);
};

class Loop : public VariableLengthPart {
public:
   class Iterator {
   public:
      Iterator() { i=0; }
      void reset() { i=0; }
   private:
      template <class T> friend class StructureLoop;
      friend class DescriptorLoop;
      template <class T> friend class TypeLoop;
      friend class ExtendedEventDescriptors;
      int i;
   };
protected:
   virtual void Parse() {}
};

//contains LoopElements of one type only
template <class T> class StructureLoop : public Loop {
public:
   //currently you must use a while-loop testing for hasNext()
   //i must be 0 to get the first descriptor (with the first call)
   bool getNext(T &obj, Iterator &it)
      {
         if (!isValid() || it.i >= getLength())
            return false;
         CharArray d=data;
         d.addOffset(it.i);
         T ret;
         ret.setData(d);
         ret.CheckParse();
         if (!checkSize(ret.getLength()))
            return false;
         it.i+=ret.getLength();
         obj=ret;
         return true;
      }
   T* getNextAsPointer(Iterator &it)
      {
         if (!isValid() || it.i >= getLength())
            return 0;
         CharArray d=data;
         d.addOffset(it.i);
         T *ret=new T();
         ret->setData(d);
         ret->CheckParse();
         if (!checkSize(ret->getLength()))
            return 0;
         it.i+=ret->getLength();
         return ret;
      }
   //bool hasNext(Iterator &it) { return getLength() > it.i; }
};

//contains descriptors of different types
class DescriptorLoop : public Loop {
public:
   DescriptorLoop() { domain=SI; }
   //i must be 0 to get the first descriptor (with the first call)
   //All returned descriptors must be delete'd.
   //returns null if no more descriptors available
   Descriptor *getNext(Iterator &it);
   //return the next descriptor with given tag, or 0 if not available.
   //if returnUnimplemetedDescriptor==true:
   //   an UnimplementedDescriptor may be returned if the next matching descriptor is unimplemented,
   //   0 will be returned if and only if no matching descriptor is found.
   //if returnUnimplemetedDescriptor==false:
   //   if 0 is returned, either no descriptor with the given tag was found,
   //   or descriptors were found, but the descriptor type is not implemented
   //In either case, a return value of 0 indicates that no further calls to this method
   //with the iterator shall be made.
   Descriptor *getNext(Iterator &it, DescriptorTag tag, bool returnUnimplemetedDescriptor=false);
   //return the next descriptor with one of the given tags, or 0 if not available.
   //if returnUnimplemetedDescriptor==true:
   //   returns 0 if and only if no descriptor with one of the given tags was found.
   //   The UnimplementedDescriptor may be returned.
   //if returnUnimplemetedDescriptor==false:
   //   if 0 is returned, either no descriptor with one of the given tags was found,
   //   or descriptors were found, but none of them are implemented.
   //   The UnimplementedDescriptor will never be returned.
   //In either case, a return value of 0 indicates that no further calls to this method
   //with the iterator shall be made.
   Descriptor *getNext(Iterator &it, DescriptorTag *tags, int arrayLength, bool returnUnimplemetedDescriptor=false);
   //returns the number of descriptors in this loop
   int getNumberOfDescriptors();
   //writes the tags of the descriptors in this loop in the array,
   // which must at least have the size getNumberOfDescriptors().
   //The number of descriptors, i.e. getNumberOfDescriptors(), is returned.
   // You can specify the array type (Descriptor tags are 8 Bit,
   // you might e.g. choose a char, short, int or DescriptorTag array)
   template <typename T> int getDescriptorTags(T *tags)
      {
         const unsigned char *p=data.getData();
         const unsigned char *end=p+getLength();
         int count=0;
         while (p < end) {
            tags[count++]=(T)Descriptor::getDescriptorTag(p);
            p+=Descriptor::getLength(p);
         }
         return count;
      }
protected:
   Descriptor *createDescriptor(int &i, bool returnUnimplemetedDescriptor);
   DescriptorTagDomain domain;
};

typedef uint8_t   EightBit;
typedef uint16_t  SixteenBit;
typedef uint32_t  ThirtyTwoBit;
typedef uint64_t  SixtyFourBit;

template <typename T> class TypeLoop : public Loop {
public:
   int getCount() { return getLength()/sizeof(T); }
   T operator[](const int index) const
      {
         switch (sizeof(T)) {
         case 1:
            return data[index];
         case 2:
            return data.TwoBytes(index);
         case 4:
            return data.FourBytes(index);
         case 8:
            return (SixtyFourBit(data.FourBytes(index)) << 32) | data.FourBytes(index+4);
         }
         return 0; // just to avoid a compiler warning
      }
   T getNext(Iterator &it) const
      {
         T ret=operator[](it.i);
         it.i+=sizeof(T);
         return ret;
      }
   bool hasNext(Iterator &it) { return isValid() && (getLength() > it.i); }
};

class MHP_DescriptorLoop : public DescriptorLoop {
public:
   MHP_DescriptorLoop() { domain=MHP; }
};

//Premiere Content Information Table
class PCIT_DescriptorLoop : public DescriptorLoop {
public:
   PCIT_DescriptorLoop() { domain=PCIT; }
};

//The content of the ExtendedEventDescriptor may be split over several
//descriptors if the text is longer than 256 bytes.
//The following classes provide base functionality to handle this case.
class GroupDescriptor : public Descriptor {
public:
   virtual int getDescriptorNumber() = 0;
   virtual int getLastDescriptorNumber() = 0;
};

class DescriptorGroup {
public:
   DescriptorGroup(bool deleteOnDesctruction=true);
   ~DescriptorGroup();
   void Add(GroupDescriptor *d);
   void Delete();
   int getLength() { return length; }
   GroupDescriptor **getDescriptors() { return array; }
   bool isComplete(); //if all descriptors have been added
protected:
   int length;
   GroupDescriptor **array;
   bool deleteOnDesctruction;
};

class String : public VariableLengthPart {
public:
   //A note to the length: getLength() returns the length of the raw data.
   //The text may be shorter. Its length can be obtained with one of the
   //getText functions and strlen.

   //returns text. Data is allocated with new and must be delete'd by the user.
   char *getText();
   //copies text into given buffer.
   //a buffer of size getLength()+1 is guaranteed to be sufficiently large.
   //In most descriptors the string length is an 8-bit field,
   //so the maximum there is 256.
   //returns the given buffer for convenience.
   //The emphasis marks 0x86 and 0x87 are still available.
   char *getText(char *buffer, int size);
   //The same semantics as for getText(char*) apply.
   //The short version of the text according to ETSI TR 101 211 (chapter 4.6)
   //will be written into the shortVersion buffer (which should, therefore, have the same
   //length as buffer). If no shortVersion is available, shortVersion will contain
   //an empty string.
   //The emphasis marks 0x86 and 0x87 are still available in buffer, but not in shortVersion.
   char *getText(char *buffer, char *shortVersion, int sizeBuffer, int sizeShortVersion);
protected:
   virtual void Parse() {}
   void decodeText(char *buffer, int size);
   void decodeText(char *buffer, char *shortVersion, int sizeBuffer, int sizeShortVersion);
};

} //end of namespace

#endif //LIBSI_SI_H
