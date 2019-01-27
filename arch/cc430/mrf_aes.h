#ifndef __MRF_AES_INCLUDED__
#define __MRF_AES_INCLUDED__

void aes_init();
void aes_encrypt(uint16_t  *in, uint16_t *out, uint16_t *iv, int len);
void aes_encrypt2(uint16_t  *in, uint16_t *out, uint16_t *iv, int len);
void aes_decrypt(uint16_t  *in, uint16_t *out, uint16_t *iv, int len);

#endif
