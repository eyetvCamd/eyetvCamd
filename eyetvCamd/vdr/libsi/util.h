/***************************************************************************
 *       Copyright (c) 2003 by Marcel Wiesweg                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   $Id: util.h 1.7 2006/02/25 10:13:28 kls Exp $
 *                                                                         *
 ***************************************************************************/

#ifndef LIBSI_UTIL_H
#define LIBSI_UTIL_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define HILO(x) (x##_hi << 8 | x##_lo)
#define BCD_TIME_TO_SECONDS(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                             (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                             ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))

namespace SI {

//Holds an array of unsigned char which is deleted
//when the last object pointing to it is deleted.
//Optimized for use in libsi.
class CharArray {
public:
   CharArray();

   CharArray(const CharArray &source);
   CharArray& operator=(const CharArray &source);
   ~CharArray();

   //can be called exactly once
   void assign(const unsigned char*data, int size, bool doCopy=true);
   //compares to a null-terminated string
   bool operator==(const char *string) const;
   //compares to another CharArray (data not necessarily null-terminated)
   bool operator==(const CharArray &other) const;

   //returns another CharArray with its offset incremented by offset
   CharArray operator+(const int offset) const;

   //access and convenience methods
   const unsigned char* getData() const { return data_->data+off; }
   const unsigned char* getData(int offset) const { return data_->data+offset+off; }
   template <typename T> const T* getData() const { return (T*)(data_->data+off); }
   template <typename T> const T* getData(int offset) const { return (T*)(data_->data+offset+off); }
      //sets p to point to data+offset, increments offset
   template <typename T> void setPointerAndOffset(const T* &p, int &offset) const { p=(T*)getData(offset); offset+=sizeof(T); }
   unsigned char operator[](const int index) const { return data_->data ? data_->data[off+index] : 0; }
   int getLength() const { return data_->size; }
   u_int16_t TwoBytes(const int index) const { return data_->data ? data_->TwoBytes(off+index) : 0; }
   u_int32_t FourBytes(const int index) const { return data_->data ? data_->FourBytes(off+index) : 0; }

   bool isValid() const { return data_->valid; }
   bool checkSize(int offset) { return (data_->valid && (data_->valid=(offset>=0 && off+offset < data_->size))); }

   void addOffset(int offset) { off+=offset; }
private:
   class Data {
   public:
      Data();
      virtual ~Data();

      virtual void assign(const unsigned char*data, int size) = 0;
      virtual void Delete() = 0;

      u_int16_t TwoBytes(const int index) const
         { return (data[index] << 8) | data[index+1]; }
      u_int32_t FourBytes(const int index) const
         { return (data[index] << 24) | (data[index+1] << 16) | (data[index+2] << 8) | data[index+3]; }
      /*#ifdef CHARARRAY_THREADSAFE
      void Lock();
      void Unlock();
      #else
      void Lock() {}
      void Unlock() {}
      #endif
      Data(const Data& d);
      void assign(int size);
      */

      const unsigned char*data;
      int size;

      // count_ is the number of CharArray objects that point at this
      // count_ must be initialized to 1 by all constructors
      // (it starts as 1 since it is pointed to by the CharArray object that created it)
      unsigned count_;

      bool valid;

      /*
      pthread_mutex_t mutex;
      pid_t lockingPid;
      pthread_t locked;
      */
   };
   class DataOwnData : public Data {
   public:
      DataOwnData() {}
      virtual ~DataOwnData();
      virtual void assign(const unsigned char*data, int size);
      virtual void Delete();
   };
   class DataForeignData : public Data {
   public:
      DataForeignData() {}
      virtual ~DataForeignData();
      virtual void assign(const unsigned char*data, int size);
      virtual void Delete();
   };
   Data* data_;
   int off;
};



//abstract base class
class Parsable {
public:
   void CheckParse();
protected:
   Parsable();
   virtual ~Parsable() {}
   //actually parses given data.
   virtual void Parse() = 0;
private:
   bool parsed;
};

//taken and adapted from libdtv, (c) Rolf Hakenes and VDR, (c) Klaus Schmidinger
namespace DVBTime {
time_t getTime(unsigned char date_hi, unsigned char date_lo, unsigned char timehr, unsigned char timemi, unsigned char timese);
time_t getDuration(unsigned char timehr, unsigned char timemi, unsigned char timese);
inline unsigned char bcdToDec(unsigned char b) { return ((b >> 4) & 0x0F) * 10 + (b & 0x0F); }
}

//taken and adapted from libdtv, (c) Rolf Hakenes
class CRC32 {
public:
   CRC32(const char *d, int len, u_int32_t CRCvalue=0xFFFFFFFF);
   bool isValid() { return crc32(data, length, value) == 0; }
   static bool isValid(const char *d, int len, u_int32_t CRCvalue=0xFFFFFFFF) { return crc32(d, len, CRCvalue) == 0; }
protected:
   static u_int32_t crc_table[256];
   static u_int32_t crc32 (const char *d, int len, u_int32_t CRCvalue);

   const char *data;
   int length;
   u_int32_t value;
};

} //end of namespace

#endif
