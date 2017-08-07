#include <crypto/aes128.h>
#include <string.h>
#include <printf.h>

const uint8_t rawkey[16] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
};

const uint32_t expanded[44] = {
	/*  0 */ 0x2b7e1516,
	/*  1 */ 0x28aed2a6,
	/*  2 */ 0xabf71588,
	/*  3 */ 0x09cf4f3c,
	/*  4 */ 0xa0fafe17,
	/*  5 */ 0x88542cb1,
	/*  6 */ 0x23a33939,
	/*  7 */ 0x2a6c7605,
	/*  8 */ 0xf2c295f2,
	/*  9 */ 0x7a96b943,
	/* 10 */ 0x5935807a,
	/* 11 */ 0x7359f67f,
	/* 12 */ 0x3d80477d,
	/* 13 */ 0x4716fe3e,
	/* 14 */ 0x1e237e44,
	/* 15 */ 0x6d7a883b,
	/* 16 */ 0xef44a541,
	/* 17 */ 0xa8525b7f,
	/* 18 */ 0xb671253b,
	/* 19 */ 0xdb0bad00,
	/* 20 */ 0xd4d1c6f8,
	/* 21 */ 0x7c839d87,
	/* 22 */ 0xcaf2b8bc,
	/* 23 */ 0x11f915bc,
	/* 24 */ 0x6d88a37a,
	/* 25 */ 0x110b3efd,
	/* 26 */ 0xdbf98641,
	/* 27 */ 0xca0093fd,
	/* 28 */ 0x4e54f70e,
	/* 29 */ 0x5f5fc9f3,
	/* 30 */ 0x84a64fb2,
	/* 31 */ 0x4ea6dc4f,
	/* 32 */ 0xead27321,
	/* 33 */ 0xb58dbad2,
	/* 34 */ 0x312bf560,
	/* 35 */ 0x7f8d292f,
	/* 36 */ 0xac7766f3,
	/* 37 */ 0x19fadc21,
	/* 38 */ 0x28d12941,
	/* 39 */ 0x575c006e,
	/* 40 */ 0xd014f9a8,
	/* 41 */ 0xc9ee2589,
	/* 42 */ 0xe13f0cc8,
	/* 43 */ 0xb6630ca6
};

int check_key_expansion(void)
{
	struct aes128 ae;
	int i, ret = 0;

	aes128_init(&ae, rawkey);

	for(i = 0; i < 44; i++)
		if(ae.W[i] != expanded[i]) {
			tracef("FAIL %i %08X expected %08X\n",
					i, ae.W[i], expanded[i]);
			ret = -1;
			break;
		}
	if(i >= 44)
		tracef("OK key expanded\n");

	return ret;
}

const uint8_t key[16] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
};

const uint8_t plain[16] = {
	0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
	0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
};

const uint8_t crypt[16] = {
	0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,
	0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a
};

int check_decryption(void)
{
	struct aes128 ae;
	uint8_t work[16];
	int ret;

	memcpy(work, crypt, 16);

	aes128_init(&ae, key);
	aes128_decrypt(&ae, work);

	if((ret = memcmp(work, plain, 16)))
		tracef("FAIL decrypt\n");
	else
		tracef("OK decrypt\n");

	return ret;
}

int check_encryption(void)
{
	struct aes128 ae;
	uint8_t work[16];
	int ret;

	memcpy(work, plain, 16);

	aes128_init(&ae, key);
	aes128_encrypt(&ae, work);

	if((ret = memcmp(work, crypt, 16)))
		tracef("FAIL encrypt\n");
	else
		tracef("OK encrypt\n");

	return ret;
}

int main(void)
{
	int ret = 0;

	ret |= check_key_expansion();
	ret |= check_decryption();
	ret |= check_encryption();

	return ret ? -1 : 0;
}
