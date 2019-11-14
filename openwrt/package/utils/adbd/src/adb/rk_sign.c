#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <fcntl.h>  
#include <errno.h>  
#include <stdlib.h>  

#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/mman.h>

#include <openssl/aes.h>  
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <rk_sign.h>

const char* publicKey = "-----BEGIN PUBLIC KEY-----\n"\
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDXAyGXbKEDLKaZg8sFKeZq9KIL\n"\
"uZchsXXdPYfEQKFTyuxAgR32obkJbkpskAMRyKXm6348DPyvnpNV1r+euXCJudFr\n"\
"nGtbcXuA02fpyYU3GTDWwmHJWJVJKkmQbyHcssV+wCpK+MXw/ht1OI/TsPKypOwa\n"\
"TKdptOXItURVGlvuTQIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

/**
 * Base64 API
 * \param dst      destination buffer
 * \param dlen     size of the destination buffer
 * \param olen     number of bytes written
 * \param src      source buffer
 * \param slen     amount of data to be encoded
 * return          0 - success -1 - fail
 */
int opnssl_base64_encode(char *dst, int dlen, int *olen,const char *src, int slen)
{
    BIO *bmem, *b64;  
    BUF_MEM *bptr; 
    if ( !src || !dst )  
    {  
        return -1;  
    }  
    b64 = BIO_new( BIO_f_base64() );  
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); 
    bmem = BIO_new( BIO_s_mem() );  
    b64 = BIO_push( b64, bmem );  
    BIO_write( b64, src, slen ); //encode  
    if ( BIO_flush( b64 ) );  
    BIO_get_mem_ptr( b64, &bptr );  
    if( bptr->length > (unsigned int)dlen)  
    {  
        printf("encode_len too small");  
        return -1;   
    }     
    *olen = bptr->length;  
    memcpy( dst, bptr->data, bptr->length );  
//  write(1,dst,bptr->length);  
    BIO_free_all( b64 );  
    return 0;  

}

int opnssl_base64_decode(char *dst, int dlen, int *olen,const char *src, int slen)
{  
    BIO *b64, *bio;  
    int size = 0;  
  
    if (!src || !dst )  
        return -1;  

    if(slen>dlen*4/3)
        return -1;  
  
    b64 = BIO_new(BIO_f_base64());  
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  
  
    bio = BIO_new_mem_buf(src, slen);  
    bio = BIO_push(b64, bio);  
  
    size = BIO_read(bio, dst, slen);  
    //printf("opnssl_base64_decode %d\n",size);
    dst[size] = '\0';  
    
    BIO_free_all(bio);  
    *olen = size;
    return 0;  
}

RSA* createPublicRSA(const char* key)
{
    RSA* rsa = NULL;
    BIO* keybio;
    keybio = BIO_new_mem_buf((void*)key, -1);
    if(keybio == NULL)
        return NULL;
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
    BIO_set_close(keybio, BIO_CLOSE);
    BIO_free(keybio);
    return rsa;
}

RSA* createPrivateRSA(const char* key)
{
    RSA* rsa = NULL;
    BIO* keybio;
    keybio = BIO_new_mem_buf((void*)key, -1);
    if(keybio == NULL)
        return NULL;
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    BIO_set_close(keybio, BIO_CLOSE);
    BIO_free(keybio);
    return rsa;
}

bool RSAVerifySignature(RSA* rsa,
                        const unsigned char* MsgHash,
                        size_t MsgHashLen,
                        const unsigned char* Msg,
                        size_t MsgLen,
                        bool* Authentic)
{
    *Authentic = false;
    int AuthStatus = 0;
    bool result = false;
    printf("RSAVerifySignature.\n");

    EVP_PKEY* pubKey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pubKey, rsa);
    EVP_MD_CTX* m_RSAVerifyCtx = EVP_MD_CTX_create();

    if (EVP_DigestVerifyInit(m_RSAVerifyCtx, NULL, EVP_md5(), NULL, pubKey) <= 0)
        goto end;
    if (EVP_DigestVerifyUpdate(m_RSAVerifyCtx, Msg, MsgLen) <= 0)
        goto end;
    AuthStatus = EVP_DigestVerifyFinal(m_RSAVerifyCtx, MsgHash, MsgHashLen);
    if (AuthStatus == 1)
    {
        *Authentic = true;
        result = true;
    }

end:
    EVP_MD_CTX_destroy(m_RSAVerifyCtx);
    EVP_PKEY_free(pubKey);
    return result;
}

bool verifySignature(const char* publicKey, unsigned char* data, size_t dlen, unsigned char* sig, size_t slen)
{
    RSA* publicRSA = createPublicRSA(publicKey);
    if(publicRSA == NULL)
        return false;
    bool authentic = false;
    bool result = RSAVerifySignature(publicRSA, sig, slen, data, dlen, &authentic);
    return result & authentic;
}

size_t getFileSize(const char* file)
{
    struct stat attr;
    if(stat(file, &attr) == -1)
        return 0;
    return attr.st_size;
}

bool verify_sign(char* serial_n,const char* sigbuf_in)
{
    bool authentic = false;
    int siglen = 0, siglenbase64 = 0,datalen = 0;
    unsigned char *sigbuf = NULL;
    unsigned char *sigbufbase64 = NULL;

	datalen = strlen(serial_n);
    siglenbase64 = strlen(sigbuf_in);
    if(siglenbase64 > 128*2)
    {
        printf("signature length too long.\n");
        goto end;
    }
    sigbufbase64 = (unsigned char *)sigbuf_in;
    sigbuf = (unsigned char*)OPENSSL_malloc(siglenbase64);
    opnssl_base64_decode((char *)sigbuf,siglenbase64,&siglen,(char *)sigbufbase64,siglenbase64);
    if(siglen != 128)
    {
        printf("signature length %d is mismatch .\n",siglen);
        goto end;
    }
    // 3. verify
    printf("verify: %s \n   sig: %s\n", serial_n, sigbuf);
    authentic = verifySignature(publicKey, (unsigned char*)serial_n, datalen, sigbuf, siglen);

end:
    if(sigbuf)
        OPENSSL_free(sigbuf);
    return authentic;
}

