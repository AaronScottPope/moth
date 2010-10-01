#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sysexits.h>
#include "common.h"
#include "arc4.h"

#define itokenlen 5

char const consonants[] = "bcdfghklmnprstvz";
char const vowels[]     = "aeiouy";

#define bubblebabble_len(n) (6*(((n)/2)+1))

/** Compute bubble babble for input buffer.
 *
 * The generated output will be of length 6*((inlen/2)+1), including the
 * trailing NULL.
 *
 * Test vectors:
 *     `' (empty string) `xexax'
 *     `1234567890'      `xesef-disof-gytuf-katof-movif-baxux'
 *     `Pineapple'       `xigak-nyryk-humil-bosek-sonax'
 */
void
bubblebabble(unsigned char *out,
             unsigned char const *in,
             const size_t inlen)
{
  size_t pos  = 0;
  int    seed = 1;
  size_t i    = 0;

  out[pos++] = 'x';
  while (1) {
    unsigned char c;

    if (i == inlen) {
      out[pos++] = vowels[seed % 6];
      out[pos++] = 'x';
      out[pos++] = vowels[seed / 6];
      break;
    }

    c = in[i++];
    out[pos++] = vowels[(((c >> 6) & 3) + seed) % 6];
    out[pos++] = consonants[(c >> 2) & 15];
    out[pos++] = vowels[((c & 3) + (seed / 6)) % 6];
    if (i == inlen) {
      break;
    }
    seed = ((seed * 5) + (c * 7) + in[i]) % 36;

    c = in[i++];
    out[pos++] = consonants[(c >> 4) & 15];
    out[pos++] = '-';
    out[pos++] = consonants[c & 15];
  }

  out[pos++] = 'x';
  out[pos] = '\0';
}

int
main(int argc, char *argv[])
{
  char    category[CAT_MAX];
  size_t  categorylen;
  char    token[TOKEN_MAX];
  size_t  tokenlen;
  uint8_t key[256];
  size_t  keylen;

  /* Read category name. */
  {
    ssize_t len;

    len = read(0, category, sizeof(category));
    if (0 >= len) return 0;
    for (categorylen = 0;
         (categorylen < len) && isalnum(category[categorylen]);
         categorylen += 1);
  }

  /* Read in that category's key. */
  {
    int fd;
    int ret;

    fd = open(package_path("mcp/tokend.keys/%.*s", (int)categorylen, category), O_RDONLY);
    if (-1 == fd) {
      fprintf(stderr, "Open key %.*s: %s\n",
              (int)categorylen, category, strerror(errno));
      return 0;
    }

    ret = read(fd, &key, sizeof(key));
    if (-1 == ret) {
      fprintf(stderr, "Read key %.*s: %s\n",
              (int)categorylen, category, strerror(errno));
      return 0;
    }
    keylen = (size_t)ret;

    close(fd);
  }

  /* Send a nonce, expect it back encrypted */
  {
    int32_t nonce;
    int32_t enonce = 0;

    urandom((char *)&nonce, sizeof(nonce));
    write(1, &nonce, sizeof(nonce));
    arc4_crypt_buffer(key, keylen, (uint8_t *)&nonce, sizeof(nonce));
    read(0, &enonce, sizeof(enonce));
    if (nonce != enonce) {
      write(1, ":<", 2);
      return 0;
    }
  }

  /* Create the token. */
  {
    unsigned char crap[itokenlen];
    unsigned char digest[bubblebabble_len(itokenlen)];

    urandom((char *)crap, sizeof(crap));

    /* Digest some random junk. */
    bubblebabble(digest, (unsigned char *)&crap, itokenlen);

    /* Append digest to category name. */
    tokenlen = (size_t)snprintf(token, sizeof(token),
                               "%.*s:%s",
                                (int)categorylen, category, digest);
  }

  /* Write that token out now. */
  {
    int fd;
    int ret;

    do {
      fd = open(state_path("tokens.db"), O_WRONLY | O_CREAT, 0666);
      if (-1 == fd) break;

      ret = lockf(fd, F_LOCK, 0);
      if (-1 == ret) break;

      ret = lseek(fd, 0, SEEK_END);
      if (-1 == ret) break;

      ret = write(fd, token, tokenlen);
      if (-1 == ret) break;

      ret = write(fd, "\n", 1);
      if (-1 == ret) break;

      ret = close(fd);
      if (-1 == ret) break;
    } while (0);

    if ((-1 == fd) || (-1 == ret)) {
      printf("!%s", strerror(errno));
      return 0;
    }
  }

  /* Encrypt the token. */
  {
    arc4_crypt_buffer(key, keylen, (uint8_t *)token, tokenlen);
  }

  /* Send it back.  If there's an error here, it's okay.  Better to have
     unclaimed tokens than unclaimable ones. */
  write(1, token, tokenlen);

  return 0;
}