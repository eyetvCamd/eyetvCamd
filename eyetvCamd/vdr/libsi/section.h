/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: section.h 1.4 2006/04/14 10:53:44 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_SECTION_H
#define LIBSI_SECTION_H

#include <time.h>

#include "si.h"
#include "headers.h"

namespace SI {

class PAT : public NumberedSection {
public:
   PAT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   PAT() {}
   class Association : public LoopElement {
   public:
      int getServiceId() const;
      int getPid() const;
      bool isNITPid() const { return getServiceId()==0; }
      virtual int getLength() { return sizeof(pat_prog); }
   protected:
      virtual void Parse();
   private:
      const pat_prog *s;
   };
   int getTransportStreamId() const;
   StructureLoop<Association> associationLoop;
protected:
   virtual void Parse();
private:
   const pat *s;
};

class CAT : public NumberedSection {
public:
   CAT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   CAT() {}
   DescriptorLoop loop;
protected:
   virtual void Parse();
};

class PMT : public NumberedSection {
public:
   PMT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   PMT() {}
   class Stream : public LoopElement {
   public:
      int getPid() const;
      int getStreamType() const;
      DescriptorLoop streamDescriptors;
      virtual int getLength() { return sizeof(pmt_info)+streamDescriptors.getLength(); }
   protected:
      virtual void Parse();
   private:
      const pmt_info *s;
   };
   DescriptorLoop commonDescriptors;
   StructureLoop<Stream> streamLoop;
   int getServiceId() const;
   int getPCRPid() const;
protected:
   virtual void Parse();
private:
   const pmt *s;
};

class TSDT : public NumberedSection {
public:
   TSDT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   TSDT() {}
   DescriptorLoop transportStreamDescriptors;
protected:
   virtual void Parse();
private:
   const tsdt *s;
};

class NIT : public NumberedSection {
public:
   NIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   NIT() {}
   class TransportStream : public LoopElement {
   public:
      int getTransportStreamId() const;
      int getOriginalNetworkId() const;
      virtual int getLength() { return sizeof(ni_ts)+transportStreamDescriptors.getLength(); }
      DescriptorLoop transportStreamDescriptors;
   protected:
      virtual void Parse();
   private:
      const ni_ts *s;
   };
   DescriptorLoop commonDescriptors;
   StructureLoop<TransportStream> transportStreamLoop;
   int getNetworkId() const;
protected:
   virtual void Parse();
private:
   const nit *s;
};

//BAT has the same structure as NIT but different allowed descriptors
class BAT : public NIT {
public:
   BAT(const unsigned char *data, bool doCopy=true) : NIT(data, doCopy) {}
   BAT() {}
   int getBouquetId() const { return getNetworkId(); }
};

class SDT : public NumberedSection {
public:
   SDT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   SDT() {}
   class Service : public LoopElement {
   public:
      int getServiceId() const;
      int getEITscheduleFlag() const;
      int getEITpresentFollowingFlag() const;
      RunningStatus getRunningStatus() const;
      int getFreeCaMode() const;
      virtual int getLength() { return sizeof(sdt_descr)+serviceDescriptors.getLength(); }
      DescriptorLoop serviceDescriptors;
   protected:
      virtual void Parse();
   private:
      const sdt_descr *s;
   };
   int getTransportStreamId() const;
   int getOriginalNetworkId() const;
   StructureLoop<Service> serviceLoop;
protected:
   virtual void Parse();
private:
   const sdt *s;
};

class EIT : public NumberedSection {
public:
   EIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   EIT() {}
   class Event : public LoopElement {
   public:
      int getEventId() const;
      time_t getStartTime() const; //UTC
      time_t getDuration() const;

      int getMJD() const;
      int getStartTimeHour() const; //UTC
      int getStartTimeMinute() const; //UTC
      int getStartTimeSecond() const; //UTC
      int getDurationHour() const;
      int getDurationMinute() const;
      int getDurationSecond() const;
      RunningStatus getRunningStatus() const;
      int getFreeCaMode() const;

      DescriptorLoop eventDescriptors;
      virtual int getLength() { return sizeof(eit_event)+eventDescriptors.getLength(); }
   protected:
      virtual void Parse();
   private:
      const eit_event *s;
   };
   int getServiceId() const;
   int getTransportStreamId() const;
   int getOriginalNetworkId() const;
   int getSegmentLastSectionNumber() const;
   int getLastTableId() const;
   StructureLoop<Event> eventLoop;

   //true if table conveys present/following information, false if it conveys schedule information
   bool isPresentFollowing() const;
   //true if table describes TS on which it is broadcast, false if it describes other TS
   bool isActualTS() const;
protected:
   virtual void Parse();
private:
   const eit *s;
};

class TDT : public Section {
public:
   TDT(const unsigned char *data, bool doCopy=true) : Section(data, doCopy) {}
   TDT() {}
   time_t getTime() const; //UTC
protected:
   virtual void Parse();
private:
   const tdt *s;
};

class TOT : public CRCSection {
public:
   TOT(const unsigned char *data, bool doCopy=true) : CRCSection(data, doCopy) {}
   TOT() {}
   time_t getTime() const;
   DescriptorLoop descriptorLoop;
protected:
   virtual void Parse();
private:
   const tot *s;
};

class RST : public Section {
public:
   RST(const unsigned char *data, bool doCopy=true) : Section(data, doCopy) {}
   RST() {}
   class RunningInfo : public LoopElement {
   public:
      int getTransportStreamId() const;
      int getOriginalNetworkId() const;
      int getServiceId() const;
      int getEventId() const;
      RunningStatus getRunningStatus() const;
      virtual int getLength() { return sizeof(rst_info); }
   protected:
      virtual void Parse();
   private:
      const rst_info *s;
   };
   StructureLoop<RunningInfo> infoLoop;
protected:
   virtual void Parse();
};

class AIT : public NumberedSection {
public:
   AIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   AIT() {}
   class Application : public LoopElement {
   public:
      virtual int getLength() { return sizeof(ait_app)+applicationDescriptors.getLength(); }
      long getOrganisationId() const;
      int getApplicationId() const;
      int getControlCode() const;
      MHP_DescriptorLoop applicationDescriptors;
   protected:
      virtual void Parse();
      const ait_app *s;
   };
   MHP_DescriptorLoop commonDescriptors;
   StructureLoop<Application> applicationLoop;
   int getApplicationType() const;
   int getAITVersion() const;
protected:
   const ait *first;
   virtual void Parse();
};

/* Premiere Content Information Table */

class PremiereCIT : public NumberedSection {
public:
   PremiereCIT(const unsigned char *data, bool doCopy=true) : NumberedSection(data, doCopy) {}
   PremiereCIT() {}
   int getContentId() const;
   time_t getDuration() const;
   PCIT_DescriptorLoop eventDescriptors;
protected:
   virtual void Parse();
private:
   const pcit *s;
};

} //end of namespace

#endif //LIBSI_TABLE_H
