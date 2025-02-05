/*---------------------------------------------------------------------------*\

  FILE........: horus_l2.c
  AUTHOR......: David Rowe
  DATE CREATED: Dec 2015

  Horus telemetry layer 2 processing.  Takes an array of 8 bit payload
  data, generates parity bits for a (23,12) Golay code, interleaves
  data and parity bits, pre-pends a Unique Word for modem sync.
  Caller is responsible for providing storage for output packet.

  [ ] code based interleaver
  [ ] test correction of 1,2 & 3 error patterms    

  1/ Unit test on a PC:

     $ gcc horus_l2.c -o horus_l2 -Wall -DHORUS_L2_UNITTEST
     $ ./horus_l2

     test 0: 22 bytes of payload data BER: 0.00 errors: 0
     test 0: 22 bytes of payload data BER: 0.01 errors: 0
     test 0: 22 bytes of payload data BER: 0.05 errors: 0
     test 0: 22 bytes of payload data BER: 0.10 errors: 7
     
     This indicates it's correcting all channel errors for 22 bytes of
     payload data, at bit error rate (BER) of 0, 0.01, 0.05.  It falls
     over at a BER of 0.10 which is expected.

  2/ To build with just the tx function, ie for linking with the payload
  firmware:

    $ gcc horus_l2.c -c -Wall
    
  By default the RX side is #ifdef-ed out, leaving the minimal amount
  of code for tx.

  3/ Generate some tx_bits as input for testing with fsk_horus:
 
    $ gcc horus_l2.c -o horus_l2 -Wall -DGEN_TX_BITS -DSCRAMBLER
    $ ./horus_l2
    $ more ../octave/horus_tx_bits_binary.txt
   
  4/ Unit testing interleaver:

    $ gcc horus_l2.c -o horus_l2 -Wall -DINTERLEAVER -DTEST_INTERLEAVER -DSCRAMBLER

  5/ Compile for use as decoder called by  fsk_horus.m and fsk_horus_stream.m:

    $ gcc horus_l2.c -o horus_l2 -Wall -DDEC_RX_BITS -DHORUS_L2_RX

\*---------------------------------------------------------------------------*/

#include "f_core/horus/horus.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUN_TIME_TABLES

#define INTERLEAVER
#define SCRAMBLER

static char uw[] = {'$', '$'};

/* Function Prototypes ------------------------------------------------*/

int32_t get_syndrome(int32_t pattern);

void golay23_init(void);

int golay23_decode(int received_codeword);

unsigned short gen_crc16(unsigned char *data_p, unsigned char length);

void interleave(unsigned char *inout, int nbytes, int dir);

void scramble(unsigned char *inout, int nbytes);

/* Functions ----------------------------------------------------------*/

/*
   We are using a Golay (23,12) code which has a codeword 23 bits
   long.  The tx packet format is:

      | Unique Word | payload data bits | parity bits |

   This function works out how much storage the caller of
   horus_l2_encode_tx_packet() will need to store the tx packet
 */

int horus_l2_get_num_tx_data_bytes(int num_payload_data_bytes) {
    int num_payload_data_bits, num_golay_codewords;
    int num_tx_data_bits, num_tx_data_bytes;

    num_payload_data_bits = num_payload_data_bytes * 8;
    num_golay_codewords = num_payload_data_bits / 12;
    if (num_payload_data_bits % 12) /* round up to 12 bits, may mean some unused bits */
        num_golay_codewords++;

    num_tx_data_bits = sizeof(uw) * 8 + num_payload_data_bits + num_golay_codewords * 11;
    num_tx_data_bytes = num_tx_data_bits / 8;
    if (num_tx_data_bits % 8) /* round up to nearest byte, may mean some unused bits */
        num_tx_data_bytes++;


    LOG_INF("num_payload_data_bytes: %d", num_payload_data_bytes);
    LOG_INF("num_golay_codewords...: %d", num_golay_codewords);
    LOG_INF("num_tx_data_bits......: %d", num_tx_data_bits);
    LOG_INF("num_tx_data_bytes.....: %d", num_tx_data_bytes);

    return num_tx_data_bytes;
}

/*
  Takes an array of payload data bytes, prepends a unique word and appends
  parity bits.

  The encoder will run on the payload on a small 8-bit uC.  As we are
  memory constrained so we do a lot of burrowing for bits out of
  packed arrays, and don't use a LUT for Golay encoding.  Hopefully it
  will run fast enough.  This was quite difficult to get going,
  suspect there is a better way to write this.  Oh well, have to start
  somewhere.
 */

int horus_l2_encode_tx_packet(unsigned char *output_tx_data, unsigned char *input_payload_data,
                              int num_payload_data_bytes) {
    int num_tx_data_bytes, num_payload_data_bits;
    unsigned char *pout = output_tx_data;
    int ninbit, ningolay, nparitybits;
    int32_t ingolay, paritybyte, inbit, golayparity;
    int ninbyte, shift, golayparitybit, i;

    num_tx_data_bytes = horus_l2_get_num_tx_data_bytes(num_payload_data_bytes);
    memcpy(pout, uw, sizeof(uw));
    pout += sizeof(uw);
    memcpy(pout, input_payload_data, num_payload_data_bytes);
    pout += num_payload_data_bytes;

    /* Read input bits one at a time.  Fill input Golay codeword.  Find output Golay codeword.
       Write this to parity bits.  Write parity bytes when we have 8 parity bits.  Bits are
       written MSB first. */

    num_payload_data_bits = num_payload_data_bytes * 8;
    ninbit = 0;
    ingolay = 0;
    ningolay = 0;
    paritybyte = 0;
    nparitybits = 0;

    while (ninbit < num_payload_data_bits) {

        /* extract input data bit */

        ninbyte = ninbit / 8;
        shift = 7 - (ninbit % 8);
        inbit = (input_payload_data[ninbyte] >> shift) & 0x1;
        LOG_DBG("inbit %d ninbyte: %d inbyte: 0x%02x inbit: %d\n", ninbit, ninbyte, input_payload_data[ninbyte], inbit);
        ninbit++;

        /* build up input golay codeword */

        ingolay = ingolay | inbit;
        ningolay++;

        /* when we get 12 bits do a Golay encode */

        if (ningolay % 12) {
            ingolay <<= 1;
        } else {
            LOG_INF(stderr, "  ningolay: %d ingolay: 0x%04x\n", ningolay, ingolay);
            golayparity = get_syndrome(ingolay << 11);
            ingolay = 0;

            LOG_INF(stderr, "  golayparity: 0x%04x\n", golayparity);

            /* write parity bits to output data */

            for (i = 0; i < 11; i++) {
                golayparitybit = (golayparity >> (10 - i)) & 0x1;
                paritybyte = paritybyte | golayparitybit;
                LOG_INF(stderr, "    i: %d golayparitybit: %d paritybyte: 0x%02x\n", i, golayparitybit, paritybyte);
                nparitybits++;
                if (nparitybits % 8) {
                    paritybyte <<= 1;
                } else {
                    /* OK we have a full byte ready */
                    *pout = paritybyte;
                    LOG_INF("      Write paritybyte: 0x%02x\n", paritybyte);
                    pout++;
                    paritybyte = 0;
                }
            }
        }
    } /* while(.... */

    /* Complete final Golay encode, we may have partially finished ingolay, paritybyte */

    LOG_INF(stderr, "finishing up .....\n");

    if (ningolay % 12) {
        ingolay >>= 1;
        golayparity = get_syndrome(ingolay << 12);
        LOG_INF("  ningolay: %d ingolay: 0x%04x\n", ningolay, ingolay);
        LOG_INF("  golayparity: 0x%04x\n", golayparity);

        /* write parity bits to output data */

        for (i = 0; i < 11; i++) {
            golayparitybit = (golayparity >> (10 - i)) & 0x1;
            paritybyte = paritybyte | golayparitybit;
            LOG_DBG(stderr, "    i: %d golayparitybit: %d paritybyte: 0x%02x\n", i, golayparitybit, paritybyte);
            nparitybits++;
            if (nparitybits % 8) {
                paritybyte <<= 1;
            } else {
                /* OK we have a full byte ready */
                *pout++ = (unsigned char) paritybyte;
                LOG_INF("      Write paritybyte: 0x%02x\n", paritybyte);
                paritybyte = 0;
            }
        }
    }

    /* and final, partially complete, parity byte */

    if (nparitybits % 8) {
        paritybyte <<= 7 - (nparitybits % 8); // use MS bits first
        *pout++ = (unsigned char) paritybyte;
        LOG_INF("      Write last paritybyte: 0x%02x nparitybits: %d \n", paritybyte, nparitybits);
    }

    LOG_INF("\npout - output_tx_data: %ld num_tx_data_bytes: %d\n", pout - output_tx_data, num_tx_data_bytes);
    assert(pout == (output_tx_data + num_tx_data_bytes));

    /* optional interleaver - we dont interleave UW */

#ifdef CONFIG_HORUS_INTERLEAVER
    interleave(&output_tx_data[sizeof(uw)], num_tx_data_bytes - 2, 0);
#endif

    /* optional scrambler to prevent long strings of the same symbol
       which upsets the modem - we dont scramble UW */

#ifdef CONFIG_HORUS_SCRAMBLER
    scramble(&output_tx_data[sizeof(uw)], num_tx_data_bytes - 2);
#endif

    return num_tx_data_bytes;
}

#ifdef CONFIG_HORUSV2_RX
void horus_l2_decode_rx_packet(unsigned char *output_payload_data, unsigned char *input_rx_data,
                               int num_payload_data_bytes) {
    int num_payload_data_bits;
    unsigned char *pout = output_payload_data;
    unsigned char *pin = input_rx_data;
    int ninbit, ingolay, ningolay, paritybyte, nparitybits;
    int ninbyte, shift, inbit, golayparitybit, i, outbit, outbyte, noutbits, outdata;
    int num_tx_data_bytes = horus_l2_get_num_tx_data_bytes(num_payload_data_bytes);

    /* optional scrambler and interleaver - we dont interleave UW */

#ifdef CONFIG_HORUS_SCRAMBLER
    scramble(&input_rx_data[sizeof(uw)], num_tx_data_bytes - 2);
#endif

#ifdef CONFIG_HORUS_INTERLEAVER
    interleave(&input_rx_data[sizeof(uw)], num_tx_data_bytes - 2, 1);
#endif

    pin = input_rx_data + sizeof(uw) + num_payload_data_bytes;

    /* Read input data bits one at a time.  When we have 12 read 11 parity bits. Golay decode.
       Write decoded (output data) bits every time we have 8 of them. */

    num_payload_data_bits = num_payload_data_bytes * 8;
    ninbit = 0;
    ingolay = 0;
    ningolay = 0;
    nparitybits = 0;
    paritybyte = *pin++;
    LOG_INF(stderr, "  Read paritybyte: 0x%02x\n", paritybyte);
    pout = output_payload_data;
    noutbits = 0;
    outbyte = 0;

    while (ninbit < num_payload_data_bits) {

        /* extract input data bit */

        ninbyte = ninbit / 8 + sizeof(uw);
        shift = 7 - (ninbit % 8);
        inbit = (input_rx_data[ninbyte] >> shift) & 0x1;
        LOG_DBG(std "inbit %d ninbyte: %d inbyte: 0x%02x inbit: %d\n", ninbit, ninbyte, input_rx_data[ninbyte], inbit);
        ninbit++;

        /* build up golay codeword */

        ingolay = ingolay | inbit;
        ningolay++;
        ingolay <<= 1;

        /* when we get 12 data bits start reading parity bits */

        if ((ningolay % 12) == 0) {
            LOG_INF(stderr, "  ningolay: %d ingolay: 0x%04x\n", ningolay, ingolay >> 1);
            for (i = 0; i < 11; i++) {
                shift = 7 - (nparitybits % 8);
                golayparitybit = (paritybyte >> shift) & 0x1;
                ingolay |= golayparitybit;
                if (i != 10) ingolay <<= 1;
                nparitybits++;
                if ((nparitybits % 8) == 0) {
                    /* OK grab a new byte */
                    paritybyte = *pin++;
                    LOG_INF("  Read paritybyte: 0x%02x\n", paritybyte);
                }
            }

            LOG_INF("  golay code word: 0x%04x\n", ingolay);
            LOG_INF("  golay decode...: 0x%04x\n", golay23_decode(ingolay));

            /* write decoded/error corrected bits to output payload data */

            outdata = golay23_decode(ingolay) >> 11;
            LOG_INF("  outdata...: 0x%04x\n", outdata);

            for (i = 0; i < 12; i++) {
                shift = 11 - i;
                outbit = (outdata >> shift) & 0x1;
                outbyte |= outbit;
                noutbits++;
                if (noutbits % 8) {
                    outbyte <<= 1;
                } else {
                    LOG_INF(stderr, "  output payload byte: 0x%02x\n", outbyte);
                    *pout++ = outbyte;
                    outbyte = 0;
                }
            }

            ingolay = 0;
        }
    } /* while(.... */

    LOG_INF("finishing up .....\n");

    /* Complete final Golay decode  */

    int golayparity = 0;
    if (ningolay % 12) {
        for (i = 0; i < 11; i++) {
            shift = 7 - (nparitybits % 8);
            golayparitybit = (paritybyte >> shift) & 0x1;
            golayparity |= golayparitybit;
            if (i != 10) golayparity <<= 1;
            nparitybits++;
            if ((nparitybits % 8) == 0) {
                /* OK grab a new byte */
                paritybyte = *pin++;
                LOG_INF("  Read paritybyte: 0x%02x", paritybyte);
            }
        }

        ingolay >>= 1;
        int codeword = (ingolay << 12) + golayparity;
        LOG_INF("  ningolay: %d ingolay: 0x%04x", ningolay, ingolay);
        LOG_INF("  golay code word: 0x%04x", codeword);
        LOG_INF("  golay decode...: 0x%04x", golay23_decode(codeword));

        outdata = golay23_decode(codeword) >> 11;
        LOG_INF("  outdata...: 0x%04x", outdata);
        LOG_INF("  num_payload_data_bits: %d noutbits: %d", num_payload_data_bits, noutbits);

        /* write final byte */

        int ntogo = num_payload_data_bits - noutbits;
        for (i = 0; i < ntogo; i++) {
            shift = ntogo - i;
            outbit = (outdata >> shift) & 0x1;
            outbyte |= outbit;
            noutbits++;
            if (noutbits % 8) {
                outbyte <<= 1;
            } else {
                LOG_INF("  output payload byte: 0x%02x\n", outbyte);
                *pout++ = outbyte;
                outbyte = 0;
            }
        }
    }

    LOG_INF("pin - output_payload_data: %ld num_payload_data_bytes: %d\n", pout - output_payload_data,
            num_payload_data_bytes);

    assert(pout == (output_payload_data + num_payload_data_bytes));
}
#endif

#ifdef CONFIG_HORUS_INTERLEAVER

uint16_t primes[] = {2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,  71,
                     73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                     179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
                     283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 379, 383, 389, 757, 761, 769, 773};

void interleave(unsigned char *inout, int nbytes, int dir) {
    uint16_t nbits = (uint16_t) nbytes * 8;
    uint32_t i, j, n, ibit, ibyte, ishift, jbyte, jshift;
    uint32_t b;
    unsigned char out[nbytes];

    memset(out, 0, nbytes);

    /* b chosen to be co-prime with nbits, I'm cheating by just finding the
       nearest prime to nbits.  It also uses storage, is run on every call,
       and has an upper limit.  Oh Well, still seems to interleave OK. */
    i = 1;
    uint16_t imax = sizeof(primes) / sizeof(uint16_t);
    while ((primes[i] < nbits) && (i < imax)) i++;
    b = primes[i - 1];

    for (n = 0; n < nbits; n++) {

        /*
          "On the Analysis and Design of Good Algebraic Interleavers", Xie et al,eq (5)
        */

        i = n;
        j = (b * i) % nbits;

        if (dir) {
            uint16_t tmp = j;
            j = i;
            i = tmp;
        }

        LOG_INF("i: %d j: %d\n", i, j);

        /* read bit i and write to bit j postion */

        ibyte = i / 8;
        ishift = i % 8;
        ibit = (inout[ibyte] >> ishift) & 0x1;

        jbyte = j / 8;
        jshift = j % 8;

        /* write jbit to ibit position */

        out[jbyte] |= ibit << jshift; // replace with i-th bit
        //out[ibyte] |= ibit << ishift; // replace with i-th bit
    }

    memcpy(inout, out, nbytes);

    LOG_INF("\nInterleaver Out:\n");
    for (i = 0; i < nbytes; i++) LOG_INF("%02d 0x%02x", i, inout[i]);
}

#endif

#ifdef CONFIG_HORUS_SCRAMBLER

/* 16 bit DVB additive scrambler as per Wikpedia example */

void scramble(unsigned char *inout, int nbytes) {
    int nbits = nbytes * 8;
    int i, ibit, ibits, ibyte, ishift, mask;
    uint16_t scrambler = 0x4a80; /* init additive scrambler at start of every frame */
    uint16_t scrambler_out;

    /* in place modification of each bit */

    for (i = 0; i < nbits; i++) {

        scrambler_out = ((scrambler & 0x2) >> 1) ^ (scrambler & 0x1);

        /* modify i-th bit by xor-ing with scrambler output sequence */

        ibyte = i / 8;
        ishift = i % 8;
        ibit = (inout[ibyte] >> ishift) & 0x1;
        ibits = ibit ^ scrambler_out; // xor ibit with scrambler output

        mask = 1 << ishift;
        inout[ibyte] &= ~mask;           // clear i-th bit
        inout[ibyte] |= ibits << ishift; // set to scrambled value

        /* update scrambler */

        scrambler >>= 1;
        scrambler |= scrambler_out << 14;

        LOG_INF("i: %02d ibyte: %d ishift: %d ibit: %d ibits: %d scrambler_out: %d", i, ibyte, ishift, ibit, ibits,
                scrambler_out);
    }

    LOG_INF("\nScrambler Out:\n");
    for (i = 0; i < nbytes; i++) LOG_INF("%02d 0x%02x", i, inout[i]);
}

#endif


/*---------------------------------------------------------------------------*\

                                   GOLAY FUNCTIONS

\*---------------------------------------------------------------------------*/

/* File:    golay23.c
 * Title:   Encoder/decoder for a binary (23,12,7) Golay code
 * Author:  Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu)
 * Date:    August 1994
 *
 * The binary (23,12,7) Golay code is an example of a perfect code, that is,
 * the number of syndromes equals the number of correctable error patterns.
 * The minimum distance is 7, so all error patterns of Hamming weight up to
 * 3 can be corrected. The total number of these error patterns is:
 *
 *       Number of errors         Number of patterns
 *       ----------------         ------------------
 *              0                         1
 *              1                        23
 *              2                       253
 *              3                      1771
 *                                     ----
 *    Total number of error patterns = 2048 = 2^{11} = number of syndromes
 *                                               --
 *                number of redundant bits -------^
 *
 * Because of its relatively low length (23), dimension (12) and number of
 * redundant bits (11), the binary (23,12,7) Golay code can be encoded and
 * decoded simply by using look-up tables. The program below uses a 16K
 * encoding table and an 8K decoding table.
 *
 * For more information, suggestions, or other ideas on implementing error
 * correcting codes, please contact me at (I'm temporarily in Japan, but
 * below is my U.S. address):
 *
 *                    Robert Morelos-Zaragoza
 *                    770 S. Post Oak Ln. #200
 *                      Houston, Texas 77056
 *
 *             email: robert@spectra.eng.hawaii.edu
 *
 *       Homework: Add an overall parity-check bit to get the (24,12,8)
 *                 extended Golay code.
 *
 * COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
 * You may implement this program for any non-commercial application. You may
 * also implement this program for commercial purposes, provided that you
 * obtain my written permission. Any modification of this program is covered
 * by this copyright.
 *
 * ==   Copyright (c) 1994  Robert Morelos-Zaragoza. All rights reserved.   ==
 */

#define X22    0x00400000 /* vector representation of X^{22} */
#define X11    0x00000800 /* vector representation of X^{11} */
#define MASK12 0xfffff800 /* auxiliary vector for testing */
#define GENPOL 0x00000c75 /* generator polinomial, g(x) */

/* Global variables:
 *
 * pattern = error pattern, or information, or received vector
 * encoding_table[] = encoding table
 * decoding_table[] = decoding table
 * data = information bits, i(x)
 * codeword = code bits = x^{11}i(x) + (x^{11}i(x) mod g(x))
 * numerr = number of errors = Hamming weight of error polynomial e(x)
 * position[] = error positions in the vector representation of e(x)
 * recd = representation of corrupted received polynomial r(x) = c(x) + e(x)
 * decerror = number of decoding errors
 * a[] = auxiliary array to generate correctable error patterns
 */

#ifdef CONFIG_HORUSV2_RX
static int inited = 0;

static int encoding_table[4096], decoding_table[2048];

static int arr2int(int a[], int r)
/*
 * Convert a binary vector of Hamming weight r, and nonzero positions in
 * array a[1]...a[r], to a long integer \sum_{i=1}^r 2^{a[i]-1}.
 */
{
    int i;
    long mul, result = 0, temp;

    for (i = 1; i <= r; i++) {
        mul = 1;
        temp = a[i] - 1;
        while (temp--) mul = mul << 1;
        result += mul;
    }
    return (result);
}
#endif

#ifdef CONFIG_HORUSV2_RX
void nextcomb(int n, int r, int a[])
/*
 * Calculate next r-combination of an n-set.
 */
{
    int i, j;

    a[r]++;
    if (a[r] <= n) return;
    j = r - 1;
    while (a[j] == n - r + j) j--;
    for (i = r; i >= j; i--) a[i] = a[j] + i - j + 1;
    return;
}
#endif

int32_t get_syndrome(int32_t pattern)
/*
 * Compute the syndrome corresponding to the given pattern, i.e., the
 * remainder after dividing the pattern (when considering it as the vector
 * representation of a polynomial) by the generator polynomial, GENPOL.
 * In the program this pattern has several meanings: (1) pattern = infomation
 * bits, when constructing the encoding table; (2) pattern = error pattern,
 * when constructing the decoding table; and (3) pattern = received vector, to
 * obtain its syndrome in decoding.
 */
{
    int32_t aux = X22;

    if (pattern >= X11)
        while (pattern & MASK12) {
            while (!(aux & pattern)) aux = aux >> 1;
            pattern ^= (aux / X11) * GENPOL;
        }
    return (pattern);
}

#ifdef CONFIG_HORUSV2_RX

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_init()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Call this once when you start your program to init the Golay tables.

\*---------------------------------------------------------------------------*/

void golay23_init(void) {
    int i;
    long temp;
    int a[4];
    int pattern;

    /*
    * ---------------------------------------------------------------------
    *                  Generate ENCODING TABLE
    *
    * An entry to the table is an information vector, a 32-bit integer,
    * whose 12 least significant positions are the information bits. The
    * resulting value is a codeword in the (23,12,7) Golay code: A 32-bit
    * integer whose 23 least significant bits are coded bits: Of these, the
    * 12 most significant bits are information bits and the 11 least
    * significant bits are redundant bits (systematic encoding).
    * ---------------------------------------------------------------------
    */
    for (pattern = 0; pattern < 4096; pattern++) {
        temp = pattern << 11;                                /* multiply information by X^{11} */
        encoding_table[pattern] = temp + get_syndrome(temp); /* add redundancy */
    }

    /*
    * ---------------------------------------------------------------------
    *                  Generate DECODING TABLE
    *
    * An entry to the decoding table is a syndrome and the resulting value
    * is the most likely error pattern. First an error pattern is generated.
    * Then its syndrome is calculated and used as a pointer to the table
    * where the error pattern value is stored.
    * ---------------------------------------------------------------------
    *
    * (1) Error patterns of WEIGHT 1 (SINGLE ERRORS)
    */
    decoding_table[0] = 0;
    decoding_table[1] = 1;
    temp = 1;
    for (i = 2; i <= 23; i++) {
        temp *= 2;
        decoding_table[get_syndrome(temp)] = temp;
    }
    /*
    * (2) Error patterns of WEIGHT 2 (DOUBLE ERRORS)
    */
    a[1] = 1;
    a[2] = 2;
    temp = arr2int(a, 2);
    decoding_table[get_syndrome(temp)] = temp;
    for (i = 1; i < 253; i++) {
        nextcomb(23, 2, a);
        temp = arr2int(a, 2);
        decoding_table[get_syndrome(temp)] = temp;
    }
    /*
    * (3) Error patterns of WEIGHT 3 (TRIPLE ERRORS)
    */
    a[1] = 1;
    a[2] = 2;
    a[3] = 3;
    temp = arr2int(a, 3);
    decoding_table[get_syndrome(temp)] = temp;
    for (i = 1; i < 1771; i++) {
        nextcomb(23, 3, a);
        temp = arr2int(a, 3);
        decoding_table[get_syndrome(temp)] = temp;
    }
    inited = 1;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_encode()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Given 12 bits of data retiurns a 23 bit codeword for transmission
  over the channel.

\*---------------------------------------------------------------------------*/

int golay23_encode(int data) {
    assert(inited);
    assert(data <= 0xfff);

    return encoding_table[data];
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_decode()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Given a 23 bit received codeword, returns the 12 bit corrected data.

\*---------------------------------------------------------------------------*/

int golay23_decode(int received_codeword) {
    assert(inited);
    assert((received_codeword < (1 << 23)) && (received_codeword >= 0));

    LOG_INF("syndrome: 0x%x\n", get_syndrome(received_codeword));
    return received_codeword ^= decoding_table[get_syndrome(received_codeword)];
}

int golay23_count_errors(int recd_codeword, int corrected_codeword) {
    int errors = 0;
    int diff, i;

    diff = recd_codeword ^ corrected_codeword;
    for (i = 0; i < 23; i++) {
        if (diff & 0x1) errors++;
        diff >>= 1;
    }

    return errors;
}

#endif

// from http://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum

unsigned short gen_crc16(unsigned char *data_p, unsigned char length) {
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((unsigned short) (x << 12)) ^ ((unsigned short) (x << 5)) ^ ((unsigned short) x);
    }
    return crc;
}

SYS_INIT(golay23_init, POST_KERNEL, 0);