#include "iotjs_module_tls_bio.h"

/* Return the number of pending bytes in read and write buffers */
size_t iotjs_bio_ctrl_pending(BIO* bio) {
  if (bio == NULL) {
    return 0;
  }

  if (bio->type == BIO_MEMORY) {
    return (size_t)bio->memLen;
  }

  /* type BIO_BIO then check paired buffer */
  if (bio->type == BIO_BIO && bio->pair != NULL) {
    BIO* pair = bio->pair;

    if (pair->wrIdx > 0 && pair->wrIdx <= pair->rdIdx) {
      /* in wrap around state where begining of buffer is being
       * overwritten */
      return (size_t)(pair->wrSz - pair->rdIdx + pair->wrIdx);
    } else {
      /* simple case where has not wrapped around */
      return (size_t)(pair->wrIdx - pair->rdIdx);
    }
  }
  return 0;
}

int iotjs_bio_set_write_buf_size(BIO* bio, long size) {
  if (bio == NULL || bio->type != BIO_BIO || size < 0) {
    return SSL_FAILURE;
  }

  /* if already in pair then do not change size */
  if (bio->pair != NULL) {
    return SSL_FAILURE;
  }

  bio->wrSz = (int)size;
  if (bio->wrSz < 0) {
    return SSL_FAILURE;
  }

  if (bio->mem != NULL) {
    free(bio->mem);
  }

  bio->mem = (BYTE*)malloc((size_t)bio->wrSz);
  if (bio->mem == NULL) {
    return SSL_FAILURE;
  }
  bio->wrIdx = 0;
  bio->rdIdx = 0;

  return SSL_SUCCESS;
}


/* Joins two BIO_BIO types. The write of b1 goes to the read of b2 and vise
 * versa. Creating something similar to a two way pipe.
 * Reading and writing between the two BIOs is not thread safe, they are
 * expected to be used by the same thread. */
int iotjs_bio_make_bio_pair(BIO* b1, BIO* b2) {
  if (b1 == NULL || b2 == NULL) {
    return SSL_FAILURE;
  }

  /* both are expected to be of type BIO and not already paired */
  if (b1->type != BIO_BIO || b2->type != BIO_BIO || b1->pair != NULL ||
      b2->pair != NULL) {
    return SSL_FAILURE;
  }

  /* set default write size if not already set */
  if (b1->mem == NULL &&
      iotjs_bio_set_write_buf_size(b1, SSL_BIO_SIZE) != SSL_SUCCESS) {
    return SSL_FAILURE;
  }

  if (b2->mem == NULL &&
      iotjs_bio_set_write_buf_size(b2, SSL_BIO_SIZE) != SSL_SUCCESS) {
    return SSL_FAILURE;
  }

  b1->pair = b2;
  b2->pair = b1;

  return SSL_SUCCESS;
}


/* Does not advance read index pointer */
int iotjs_bio_nread0(BIO* bio, char** buf) {
  if (bio == NULL || buf == NULL) {
    return 0;
  }

  /* if paired read from pair */
  if (bio->pair != NULL) {
    BIO* pair = bio->pair;

    /* case where have wrapped around write buffer */
    *buf = (char*)pair->mem + pair->rdIdx;
    if (pair->wrIdx > 0 && pair->rdIdx >= pair->wrIdx) {
      return pair->wrSz - pair->rdIdx;
    } else {
      return pair->wrIdx - pair->rdIdx;
    }
  }
  return 0;
}


/* similar to SSL_BIO_nread0 but advances the read index */
int iotjs_bio_nread(BIO* bio, char** buf, size_t num) {
  int sz = SSL_BIO_UNSET;

  if (bio == NULL || buf == NULL) {
    return SSL_FAILURE;
  }

  if (bio->pair != NULL) {
    /* special case if asking to read 0 bytes */
    if (num == 0) {
      *buf = (char*)bio->pair->mem + bio->pair->rdIdx;
      return 0;
    }

    /* get amount able to read and set buffer pointer */
    sz = iotjs_bio_nread0(bio, buf);
    if (sz == 0) {
      return SSL_BIO_ERROR;
    }

    if ((int)num < sz) {
      sz = (int)num;
    }
    bio->pair->rdIdx += sz;

    /* check if have read to the end of the buffer and need to reset */
    if (bio->pair->rdIdx == bio->pair->wrSz) {
      bio->pair->rdIdx = 0;
      if (bio->pair->wrIdx == bio->pair->wrSz) {
        bio->pair->wrIdx = 0;
      }
    }

    /* check if read up to write index, if so then reset indexs */
    if (bio->pair->rdIdx == bio->pair->wrIdx) {
      bio->pair->rdIdx = 0;
      bio->pair->wrIdx = 0;
    }
  }

  return sz;
}


int iotjs_bio_nwrite(BIO* bio, char** buf, int num) {
  int sz = SSL_BIO_UNSET;

  if (bio == NULL || buf == NULL) {
    return 0;
  }

  if (bio->pair != NULL) {
    if (num == 0) {
      *buf = (char*)bio->mem + bio->wrIdx;
      return 0;
    }

    if (bio->wrIdx < bio->rdIdx) {
      /* if wrapped around only write up to read index. In this case
       * rdIdx is always greater then wrIdx so sz will not be negative. */
      sz = bio->rdIdx - bio->wrIdx;
    } else if (bio->rdIdx > 0 && bio->wrIdx == bio->rdIdx) {
      return SSL_BIO_ERROR; /* no more room to write */
    } else {
      /* write index is past read index so write to end of buffer */
      sz = bio->wrSz - bio->wrIdx;

      if (sz <= 0) {
        /* either an error has occured with write index or it is at the
         * end of the write buffer. */
        if (bio->rdIdx == 0) {
          /* no more room, nothing has been read */
          return SSL_BIO_ERROR;
        }

        bio->wrIdx = 0;

        /* check case where read index is not at 0 */
        if (bio->rdIdx > 0) {
          sz = bio->rdIdx; /* can write up to the read index */
        } else {
          sz = bio->wrSz; /* no restriction other then buffer size */
        }
      }
    }

    if (num < sz) {
      sz = num;
    }
    *buf = (char*)bio->mem + bio->wrIdx;
    bio->wrIdx += sz;

    /* if at the end of the buffer and space for wrap around then set
     * write index back to 0 */
    if (bio->wrIdx == bio->wrSz && bio->rdIdx > 0) {
      bio->wrIdx = 0;
    }
  }

  return sz;
}


/* Reset BIO to initial state */
int iotjs_bio_reset(BIO* bio) {
  if (bio == NULL) {
    /* -1 is consistent failure even for FILE type */
    return SSL_BIO_ERROR;
  }

  switch (bio->type) {
    case BIO_BIO:
      bio->rdIdx = 0;
      bio->wrIdx = 0;
      return 0;

    default: { break; }
  }

  return SSL_BIO_ERROR;
}


int iotjs_bio_read(BIO* bio, const char* buf, size_t size) {
  int sz;
  char* pt;
  sz = iotjs_bio_nread(bio, &pt, size);

  if (sz > 0) {
    memset((void*)buf, 0, (size_t)sz);
    memcpy((void*)buf, pt, (size_t)sz);
  }

  return sz;
}

int iotjs_bio_write(BIO* bio, const char* buf, size_t size) {
  /* internal function where arguments have already been sanity checked */
  int sz;
  char* data;

  sz = iotjs_bio_nwrite(bio, &data, (int)size);

  /* test space for write */
  if (sz <= 0) {
    return sz;
  }

  memset(data, 0, (size_t)sz);
  memcpy(data, buf, (size_t)sz);
  return sz;
}


/**
 * support bio type only
 *e
 * @param type
 * @return
 */
BIO* iotjs_ssl_bio_new(int type) {
  BIO* bio = (BIO*)malloc(sizeof(BIO));
  if (bio) {
    bzero(bio, sizeof(BIO));
    bio->type = type;
    bio->mem = NULL;
    bio->prev = NULL;
    bio->next = NULL;
  }
  return bio;
}

int iotjs_bio_free(BIO* bio) {
  /* unchain?, doesn't matter in goahead since from free all */
  if (bio) {
    /* remove from pair by setting the paired bios pair to NULL */
    if (bio->pair != NULL) {
      bio->pair->pair = NULL;
    }
    if (bio->mem)
      free(bio->mem);
    free(bio);
  }
  return 0;
}

int iotjs_bio_free_all(BIO* bio) {
  while (bio) {
    BIO* next = bio->next;
    iotjs_bio_free(bio);
    bio = next;
  }
  return 0;
}

int iotjs_bio_net_send(void* ctx, const unsigned char* buf, size_t len) {
  BIO* bio = (BIO*)ctx;

  int sz;
  sz = iotjs_bio_write(bio, (const char*)buf, len);
  if (sz <= 0) {
    return MBEDTLS_ERR_SSL_WANT_WRITE;
  }
  return sz;
}

int iotjs_bio_net_recv(void* ctx, unsigned char* buf, size_t len) {
  BIO* bio = (BIO*)ctx;
  int sz;
  sz = iotjs_bio_read(bio, (const char*)buf, len);

  if (sz <= 0) {
    return MBEDTLS_ERR_SSL_WANT_READ;
  }
  return sz;
}
