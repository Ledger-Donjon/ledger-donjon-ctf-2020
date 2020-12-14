/**
 * This file is part of picoHSM
 * 
 * picoHSM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Ledger SAS, written by Olivier Hériveaux
 */

#ifndef _W5500_HXX_
#define _W5500_HXX_


#include <unistd.h>
#include "stm32f205.hxx"


/**
 * Enum with all the possible registers of the W5500.
 * Encoding is special: bits [31:24] are 0x00 for common registers, 0x01 for
 * socket registers. It is used to calculate the block number from the socket
 * number. The bits [23:16] selects the block independently from the socket
 * number. 16-bits LSB are the offset in the block.
 */
enum class w5500_reg_t: uint32_t {
    // Common Register Block
    mr = 0x00000000,
    gar0 = 0x00000001,
    gar1 = 0x00000002,
    gar2 = 0x00000003,
    gar3 = 0x00000004,
    subr0 = 0x00000005,
    subr1 = 0x00000006,
    subr2 = 0x00000007,
    subr3 = 0x00000008,
    shar0 = 0x00000009,
    shar1 = 0x0000000a,
    shar2 = 0x0000000b,
    shar3 = 0x0000000c,
    shar4 = 0x0000000d,
    shar5 = 0x0000000e,
    sipr0 = 0x0000000f,
    sipr1 = 0x00000010,
    sipr2 = 0x00000011,
    sipr3 = 0x00000012,
    intlevel0 = 0x00000013,
    intlevel1 = 0x00000014,
    ir = 0x00000015,
    imr = 0x00000016,
    sir = 0x00000017,
    simr = 0x00000018,
    rtr0 = 0x00000019,
    rtr1 = 0x0000001a,
    rcr = 0x0000001b,
    ptimer = 0x0000001c,
    pmagic = 0x0000001d,
    phar0 = 0x0000001e,
    phar1 = 0x0000001f,
    phar2 = 0x00000020,
    phar3 = 0x00000021,
    phar4 = 0x00000022,
    phar5 = 0x00000023,
    psid0 = 0x00000024,
    psid1 = 0x00000025,
    pmru0 = 0x00000026,
    pmru1 = 0x00000027,
    uipr0 = 0x00000028,
    uipr1 = 0x00000029,
    uipr2 = 0x0000002a,
    uipr3 = 0x0000002b,
    uportr0 = 0x0000002c,
    uportr1 = 0x0000002d,
    phycfgr = 0x0000002e,
    versionr = 0x00000039,
    // Socket Register Block
    sn_mr = 0x01010000,
    sn_cr = 0x01010001,
    sn_ir = 0x01010002,
    sn_sr = 0x01010003,
    sn_port0 = 0x01010004,
    sn_port1 = 0x01010005,
    sn_dhar0 = 0x01010006,
    sn_dhar1 = 0x01010007,
    sn_dhar2 = 0x01010008,
    sn_dhar3 = 0x01010009,
    sn_dhar4 = 0x0101000a,
    sn_dhar5 = 0x0101000b,
    sn_dipr0 = 0x0101000c,
    sn_dipr1 = 0x0101000d,
    sn_dipr2 = 0x0101000e,
    sn_dipr3 = 0x0101000f,
    sn_dport0 = 0x01010010,
    sn_dport1 = 0x01010011,
    sn_mssr0 = 0x01010012,
    sn_mssr1 = 0x01010013,
    sn_tos = 0x01010015,
    sn_ttl = 0x01010016,
    sn_rxbuf_size = 0x0101001e,
    sn_txbuf_size = 0x0101001f,
    sn_tx_fsr0 = 0x01010020,
    sn_tx_fsr1 = 0x01010021,
    sn_tx_rd0 = 0x01010022,
    sn_tx_rd1 = 0x01010023,
    sn_tx_wr0 = 0x01010024,
    sn_tx_wr1 = 0x01010025,
    sn_rx_rsr0 = 0x01010026,
    sn_rx_rsr1 = 0x01010027,
    sn_rx_rd0 = 0x01010028,
    sn_rx_rd1 = 0x01010029,
    sn_rx_wr0 = 0x0101002a,
    sn_rx_wr1 = 0x0101002b,
    sn_imr = 0x0101002c,
    sn_frag0 = 0x0101002d,
    sn_frag1 = 0x0101002e,
    sn_kpalvtr = 0x0101002f,
    // Socket buffers
    tx_buf = 0x01020000,
    rx_buf = 0x01030000
};


class w5500_t {
    public:
        /** Number of maximum supported sockets by the W5500. */
        static const uint8_t max_sockets = 8;

        w5500_t(volatile spi_regs_t*);
        void reset();
        void set_mac(uint8_t[6]);
        void set_gateway(uint8_t[4]);
        void set_ip(uint8_t[4]);
        void set_mask(uint8_t[4]);
        void write(w5500_reg_t, uint8_t, const uint8_t*, size_t);
        void write_u8(w5500_reg_t, uint8_t, uint8_t);
        void write_u16(w5500_reg_t, uint8_t, uint16_t);
        void read(w5500_reg_t, uint8_t, uint8_t*, size_t);
        uint8_t read_u8(w5500_reg_t, uint8_t);
        uint16_t read_u16(w5500_reg_t, uint8_t);
        uint16_t read_u16_stable(w5500_reg_t, uint8_t);
        
    private:
        /** SPI peripheral used for the communication with the Ethernet
         * controller. */
        volatile spi_regs_t* spi;

        void rst(bool) const;
        void sel(bool) const;
        void spi_wait_txe() const;
        void spi_wait_rxne() const;
        uint8_t spi_byte(uint8_t);
        void frame_head(w5500_reg_t, uint8_t, bool, size_t);
};


/**
 * Possible socket status values from W5500 socket status registers.
 */
enum class socket_status_t: uint8_t {
    closed = 0x00,
    init = 0x13,
    listen = 0x14,
    established = 0x17,
    close_wait = 0x1c,
    udp = 0x22,
    macraw = 0x42,
    synsent = 0x15,
    synrecv = 0x16,
    fin_wait = 0x18,
    closing = 0x1a,
    time_wait = 0x1b,
    last_ack = 0x1d,
    undoc_19 = 0x19,
    syn = 0x10, // Undocumented in datasheet
    synack = 0x11 // Undocumented in datasheet
};


/**
 * Possible command codes for W5500 socket command registers.
 */
enum class socket_command_t: uint8_t {
    open = 0x01,
    listen = 0x02,
    connect = 0x04,
    discon = 0x08,
    close = 0x10,
    send = 0x20,
    send_mac = 0x21,
    send_keep = 0x22,
    recv = 0x40
};


class socket_t {
    public:
        socket_t(w5500_t*, uint8_t);

        socket_status_t get_status();
        uint8_t get_no() const;
        bool listen(uint16_t);
        bool connect(uint8_t*, uint16_t);
        void write(const uint8_t*, size_t);
        size_t avail();
        size_t read_exact(uint8_t*, size_t);
        size_t read_avail(uint8_t*, size_t);
        void print(const char*);
        void close();
        void disconnect();

    private:
        /** W5500 low-level controller. */
        w5500_t* dev;
        /** Socket number in the W5500 */
        uint8_t no;

        void command(socket_command_t);
};


#endif
