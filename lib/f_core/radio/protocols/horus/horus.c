#ifdef __cplusplus
extern "C" {
#endif
#include "f_core/radio/protocols/horus/horus.h"

#include "f_core/radio/protocols/horus/golay.h"

#include <assert.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(HORUSV2, CONFIG_HORUS_LOG_LEVEL);

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
\*---------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char uw[] = {'$', '$'};

/* Function Prototypes ------------------------------------------------*/
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
    if (num_payload_data_bits % 12) { /* round up to 12 bits, may mean some unused bits */
        num_golay_codewords++;
    }

    num_tx_data_bits = (sizeof(uw) * 8) + num_payload_data_bits + (num_golay_codewords * 11);
    num_tx_data_bytes = num_tx_data_bits / 8;
    if (num_tx_data_bits % 8) { /* round up to nearest byte, may mean some unused bits */
        num_tx_data_bytes++;
    }

    LOG_DBG("num_payload_data_bytes: %d", num_payload_data_bytes);
    LOG_DBG("num_golay_codewords...: %d", num_golay_codewords);
    LOG_DBG("num_tx_data_bits......: %d", num_tx_data_bits);
    LOG_DBG("num_tx_data_bytes.....: %d", num_tx_data_bytes);

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
    unsigned char *pout = output_tx_data;

    int num_tx_data_bytes = horus_l2_get_num_tx_data_bytes(num_payload_data_bytes);
    memcpy(pout, uw, sizeof(uw));
    pout += sizeof(uw);
    memcpy(pout, input_payload_data, num_payload_data_bytes);
    pout += num_payload_data_bytes;

    /* Read input bits one at a time.  Fill input Golay codeword.  Find output Golay codeword.
       Write this to parity bits.  Write parity bytes when we have 8 parity bits.  Bits are
       written MSB first. */
    int num_payload_data_bits = num_payload_data_bytes * 8;
    int ninbit = 0;
    int32_t ingolay = 0;
    int32_t ningolay = 0;
    int32_t paritybyte = 0;
    int nparitybits = 0;

    while (ninbit < num_payload_data_bits) {

        /* extract input data bit */
        int ninbyte = ninbit / 8;
        int shift = 7 - (ninbit % 8);
        int inbit = (input_payload_data[ninbyte] >> shift) & 0x1;
        LOG_DBG("inbit %d ninbyte: %d inbyte: 0x%02x inbit: %d", ninbit, ninbyte, input_payload_data[ninbyte], inbit);
        ninbit++;

        /* build up input golay codeword */

        ingolay = ingolay | inbit;
        ningolay++;

        /* when we get 12 bits do a Golay encode */

        if (ningolay % 12) {
            ingolay <<= 1;
        } else {
            LOG_INF("ningolay: %d ingolay: 0x%04x", ningolay, ingolay);
            int32_t golayparity = get_syndrome(ingolay << 11);
            ingolay = 0;

            LOG_INF("golayparity: 0x%04x", golayparity);

            /* write parity bits to output data */

            for (int i = 0; i < 11; i++) {
                int golayparitybit = (golayparity >> (10 - i)) & 0x1;
                paritybyte = paritybyte | golayparitybit;
                LOG_INF("i: %d golayparitybit: %d paritybyte: 0x%02x", i, golayparitybit, paritybyte);
                nparitybits++;
                if (nparitybits % 8) {
                    paritybyte <<= 1;
                } else {
                    /* OK we have a full byte ready */
                    *pout = paritybyte;
                    LOG_INF("\t\tWrite paritybyte: 0x%02x", paritybyte);
                    pout++;
                    paritybyte = 0;
                }
            }
        }
    }

    /* Complete final Golay encode, we may have partially finished ingolay, paritybyte */

    LOG_INF("finishing up .....");

    if (ningolay % 12) {
        ingolay >>= 1;
        int32_t golayparity = get_syndrome(ingolay << 12);
        LOG_INF("\tningolay: %d ingolay: 0x%04x", ningolay, ingolay);
        LOG_INF("\tgolayparity: 0x%04x", golayparity);

        /* write parity bits to output data */

        for (int i = 0; i < 11; i++) {
            int golayparitybit = (golayparity >> (10 - i)) & 0x1;
            paritybyte = paritybyte | golayparitybit;
            LOG_DBG("\ti: %d golayparitybit: %d paritybyte: 0x%02x", i, golayparitybit, paritybyte);
            nparitybits++;
            if (nparitybits % 8) {
                paritybyte <<= 1;
            } else {
                /* OK we have a full byte ready */
                *pout++ = (unsigned char) paritybyte;
                LOG_INF("\t\tWrite paritybyte: 0x%02x", paritybyte);
                paritybyte = 0;
            }
        }
    }

    /* and final, partially complete, parity byte */

    if (nparitybits % 8) {
        paritybyte <<= 7 - (nparitybits % 8); // use MS bits first
        *pout++ = (unsigned char) paritybyte;
        LOG_INF("\t\tWrite last paritybyte: 0x%02x nparitybits: %d", paritybyte, nparitybits);
    }

    // LOG_INF(pout - output_tx_data: %ld num_tx_data_bytes: %d", pout - output_tx_data, num_tx_data_bytes);
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
    unsigned char *pout = output_payload_data;
    unsigned char *pin = input_rx_data;
    // int ninbyte, shift, inbit, golayparitybit, i, outbit, outbyte, noutbits, outdata;
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

    int num_payload_data_bits = num_payload_data_bytes * 8;
    int ninbit = 0;
    int ingolay = 0;
    int ningolay = 0;
    int nparitybits = 0;
    int paritybyte = *pin++;
    LOG_INF("Read paritybyte: 0x%02x", paritybyte);
    pout = output_payload_data;
    int noutbits = 0;
    int outbyte = 0;

    while (ninbit < num_payload_data_bits) {

        /* extract input data bit */

        int ninbyte = ninbit / 8 + sizeof(uw);
        int shift = 7 - (ninbit % 8);
        int inbit = (input_rx_data[ninbyte] >> shift) & 0x1;
        LOG_DBG("inbit %d ninbyte: %d inbyte: 0x%02x inbit: %d", ninbit, ninbyte, input_rx_data[ninbyte], inbit);
        ninbit++;

        /* build up golay codeword */

        ingolay = ingolay | inbit;
        ningolay++;
        ingolay <<= 1;

        /* when we get 12 data bits start reading parity bits */

        if ((ningolay % 12) == 0) {
            LOG_INF("ningolay: %d ingolay: 0x%04x", ningolay, ingolay >> 1);
            for (int i = 0; i < 11; i++) {
                shift = 7 - (nparitybits % 8);
                int golayparitybit = (paritybyte >> shift) & 0x1;
                ingolay |= golayparitybit;
                if (i != 10) ingolay <<= 1;
                nparitybits++;
                if ((nparitybits % 8) == 0) {
                    /* OK grab a new byte */
                    paritybyte = *pin++;
                    LOG_INF("\tRead paritybyte: 0x%02x", paritybyte);
                }
            }

            LOG_INF("\tgolay code word: 0x%04x", ingolay);
            LOG_INF("\tgolay decode...: 0x%04x", golay23_decode(ingolay));

            /* write decoded/error corrected bits to output payload data */

            int outdata = golay23_decode(ingolay) >> 11;
            LOG_INF("\toutdata...: 0x%04x", outdata);

            for (int i = 0; i < 12; i++) {
                shift = 11 - i;
                int outbit = (outdata >> shift) & 0x1;
                outbyte |= outbit;
                noutbits++;
                if (noutbits % 8) {
                    outbyte <<= 1;
                } else {
                    LOG_INF("output payload byte: 0x%02x", outbyte);
                    *pout++ = outbyte;
                    outbyte = 0;
                }
            }

            ingolay = 0;
        }
    }

    LOG_INF("finishing up .....");

    /* Complete final Golay decode  */

    int golayparity = 0;
    if (ningolay % 12) {
        for (int i = 0; i < 11; i++) {
            int shift = 7 - (nparitybits % 8);
            int golayparitybit = (paritybyte >> shift) & 0x1;
            golayparity |= golayparitybit;
            if (i != 10) golayparity <<= 1;
            nparitybits++;
            if ((nparitybits % 8) == 0) {
                /* OK grab a new byte */
                paritybyte = *pin++;
                LOG_INF("\tRead paritybyte: 0x%02x", paritybyte);
            }
        }

        ingolay >>= 1;
        int codeword = (ingolay << 12) + golayparity;
        LOG_INF("\tningolay: %d ingolay: 0x%04x", ningolay, ingolay);
        LOG_INF("\tgolay code word: 0x%04x", codeword);
        LOG_INF("\tgolay decode...: 0x%04x", golay23_decode(codeword));

        int outdata = golay23_decode(codeword) >> 11;
        LOG_INF("\toutdata...: 0x%04x", outdata);
        LOG_INF("\tnum_payload_data_bits: %d noutbits: %d", num_payload_data_bits, noutbits);

        /* write final byte */

        int ntogo = num_payload_data_bits - noutbits;
        for (int i = 0; i < ntogo; i++) {
            int shift = ntogo - i;
            int outbit = (outdata >> shift) & 0x1;
            outbyte |= outbit;
            noutbits++;
            if (noutbits % 8) {
                outbyte <<= 1;
            } else {
                LOG_INF("\toutput payload byte: 0x%02x", outbyte);
                *pout++ = outbyte;
                outbyte = 0;
            }
        }
    }

    LOG_INF("pin - output_payload_data: %ld num_payload_data_bytes: %d", pout - output_payload_data,
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
    unsigned char out[nbytes] = {};

    /* b chosen to be co-prime with nbits, I'm cheating by just finding the
       nearest prime to nbits.  It also uses storage, is run on every call,
       and has an upper limit.  Oh Well, still seems to interleave OK. */
    uint32_t i = 1;
    uint16_t imax = sizeof(primes) / sizeof(uint16_t);
    while ((primes[i] < nbits) && (i < imax)) i++;
    uint32_t b = primes[i - 1];

    for (uint32_t n = 0; n < nbits; n++) {

        /*
          "On the Analysis and Design of Good Algebraic Interleavers", Xie et al,eq (5)
        */

        uint32_t i = n;
        uint32_t j = (b * i) % nbits;

        if (dir) {
            uint16_t tmp = j;
            j = i;
            i = tmp;
        }

        LOG_INF("i: %d j: %d", i, j);

        /* read bit i and write to bit j postion */

        uint32_t ibyte = i / 8;
        uint32_t ishift = i % 8;
        uint32_t ibit = (inout[ibyte] >> ishift) & 0x1;

        uint32_t jbyte = j / 8;
        uint32_t jshift = j % 8;

        /* write jbit to ibit position */

        out[jbyte] |= ibit << jshift; // replace with i-th bit
        //out[ibyte] |= ibit << ishift; // replace with i-th bit
    }

    memcpy(inout, out, nbytes);

    LOG_HEXDUMP_INF(inout, nbytes, "Interleaver Out");
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

    LOG_HEXDUMP_INF(inout, nbytes, "Scrambler Out");
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

/*
 * Added code, not from RS41ng, to wrap generic encoding for specifically V2 use
 */

int horusv2_encode(struct horus_packet_v2 *input_packet, horus_packet_v2_encoded_buffer_t *output_buffer) {
    input_packet->checksum =
        gen_crc16((unsigned char *) input_packet, sizeof(struct horus_packet_v2) - sizeof(uint16_t));
    horus_l2_encode_tx_packet((unsigned char *) &output_buffer[0], (unsigned char *) input_packet,
                              sizeof(struct horus_packet_v2));
    return 0;
}

#ifdef CONFIG_HORUSV2_RX
void horusv2_decode(const horus_packet_v2_encoded_buffer_t *input_buffer, struct horus_packet_v2 *output_packet) {
    horus_l2_decode_rx_packet((unsigned char *) output_packet, (unsigned char *) input_buffer,
                              sizeof(struct horus_packet_v2));
}
bool horusv2_checksum_verify(const struct horus_packet_v2 *input_packet) {
    uint16_t checksum = gen_crc16((unsigned char *) input_packet, sizeof(struct horus_packet_v2) - sizeof(uint16_t));
    return checksum == input_packet->checksum;
}
#endif

#ifdef __cplusplus
}
#endif
