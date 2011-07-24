#if !defined(__DVB_HEADER_H__)
#define __DVB_HEADER_H__

typedef struct 
{
#if defined(__BIG_ENDIAN__)
  unsigned int syncByte:8,
	       transportErr:1,
	       payloadStart:1,
	       priority:1,
	       pid:13,
	       scrambling:2,
	       adaption:2,
	       continuity:4;
#endif
#if defined(__LITTLE_ENDIAN__)

  unsigned int continuity:4,
	       adaption:2,
	       scrambling:2,
	       pid:13,
	       priority:1,
	       payloadStart:1,
	       transportErr:1,
	       syncByte:8;
#endif
} TSFields;

typedef union
{
  TSFields ts;
  unsigned int word;
} TSHeader;

#endif