#if !defined(__MESSAGES_H__)
#define __MESSAGES_H__

typedef enum 
{
  msg_new_channel = 0x25252525,
  msg_termitate = 0xdededede,
  msg_initialized = 0xdadadada,
  msg_add_pid = 0xadadadad,
  msg_remove_pid = 0xaaaaaaaa,
  msg_dw = 0x11111111,
  msg_ca_change = 0x22222222,
  msg_camd_online = 0x12121212,
} msg_type;

typedef struct
{
  msg_type id;
  unsigned long mTransponder;
  unsigned long mService;
  unsigned long mPmt;
} msgNewChannel;

typedef struct
{
  msg_type id;
} msgTerminate;

typedef struct
{
  msg_type id;
} msgInitialized;

typedef struct
{
  msg_type id;
  unsigned long mPid;
} msgPid;

typedef struct
{
  msg_type id;
  unsigned char dw[16];
} newDw;

static inline void fillNewChannelMessage(msgNewChannel *msg, unsigned long t, unsigned long s, unsigned long p)
{
  msg->id = msg_new_channel;
  msg->mPmt = htonl(p);
  msg->mService = htonl(s);
  msg->mTransponder = htonl(t);
}

#endif