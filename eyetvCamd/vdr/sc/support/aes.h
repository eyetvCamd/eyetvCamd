#ifndef HEADER_AES_H
#define HEADER_AES_H

/*
#ifdef OPENSSL_NO_AES
#error AES is disabled.
#endif
*/

#define AES_ENCRYPT	1
#define AES_DECRYPT	0

/* Because array size can't be a const in C, the following two are macros.
   Both sizes are in bytes. */
#define AES_MAXNR 14
#define AES_BLOCK_SIZE 16

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(OPENSSL_SYS_WINCE)
# define SWAP(x) (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)
# define GETU32(p) SWAP(*((u32 *)(p)))
# define PUTU32(ct, st) { *((u32 *)(ct)) = SWAP((st)); }
#else
# define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
# define PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }
#endif

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define MAXKC   (256/32)
#define MAXKB   (256/8)
#define MAXNR   14

/* This controls loop-unrolling in aes_core.c */
#undef FULL_UNROLL

/* This should be a hidden type, but EVP requires that the size be known */
struct aes_key_st {
    unsigned long rd_key[4 *(AES_MAXNR + 1)];
    int rounds;
};
typedef struct aes_key_st AES_KEY;

const char *AES_options(void);

int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key);
int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key);

void AES_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);

void AES_ecb_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key, const int enc);
void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, const int enc);
void AES_cfb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc);
void AES_ofb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num);
void AES_ctr128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char ivec[AES_BLOCK_SIZE],
	unsigned char ecount_buf[AES_BLOCK_SIZE],
	unsigned int *num);


#ifdef  __cplusplus
}
#endif

#endif /* !HEADER_AES_H */
