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

#include "w5500.hxx"
#include "delay.hxx"
#include "panic.hxx"
#include "usart.hxx"
#include "util.hxx"


/**
 * Constructor.
 *
 * @param spi_ SPI peripheral used for the communication.
 */
w5500_t::w5500_t(volatile spi_regs_t* spi_):
    spi(spi_){
}


/**
 * Reset the Ethernet controller (hard reset).
 * Also fore chip select to high.
 */
void w5500_t::reset(){
    rst(true);
    delay(100000);
    sel(false);
    rst(false);
    delay(100000);
}


/**
 * Set the MAC address.
 *
 * @param mac MAC address.
 */
void w5500_t::set_mac(uint8_t mac[6]){
    write(w5500_reg_t::shar0, 0, mac, 6);
}


/**
 * Set the gateway address.
 *
 * @param gw Gateway address.
 */
void w5500_t::set_gateway(uint8_t gw[4]){
    write(w5500_reg_t::gar0, 0, gw, 4);
}


/**
 * Set the network mask.
 *
 * @param mask Mask.
 */
void w5500_t::set_mask(uint8_t mask[4]){
    write(w5500_reg_t::subr0, 0, mask, 4);
}


/**
 * Set the IP address.
 *
 * @param ip IP address.
 */
void w5500_t::set_ip(uint8_t ip[4]){
    write(w5500_reg_t::sipr0, 0, ip, 4);
}


/**
 * Transmit the beginning of a read or write transmission with the W5500.
 *
 * @param reg First register to be read or written.
 * @param sn Socket number. Not used for common registers.
 * @param wr true to write, false to read.
 * @param len Number of bytes to be read or written.
 */
void w5500_t::frame_head(w5500_reg_t reg, uint8_t sn, bool wr, size_t len){
    uint16_t offset = (uint16_t)((uint32_t)reg & 0xffff);
    uint8_t block = (uint8_t)(
        ((((uint32_t)reg >> 24) * sn) << 2) +
        (((uint32_t)reg >> 16) & 0xff));
    spi_byte((uint8_t)(offset >> 8));
    spi_byte((uint8_t)(offset & 0xff));
    uint8_t size_field;
    switch (len) {
        case 1:
            size_field = 1;
            break;
        case 2:
            size_field = 2;
            break;
        case 4:
            size_field = 3;
            break;
        default:
            size_field = 0;
    }
    const int wr_bit = wr ? (1 << 2) : 0;
    spi_byte((uint8_t)((block << 3) | wr_bit | size_field));
}


/**
 * Write data.
 *
 * @param reg First register to be written.
 * @param sn Socket number. Not used for common registers.
 * @param buf Buffer with the data to be written.
 * @param len Number of bytes to be written.
 */
void w5500_t::write(w5500_reg_t reg, uint8_t sn, const uint8_t* buf,
    size_t len){

    sel(true);
    frame_head(reg, sn, true, len);
    for (size_t i = 0; i < len; ++i)
        spi_byte(buf[i]);
    sel(false);
}


/**
 * Write a byte to a register.
 *
 * @param reg Register to be written.
 * @param sn Socket number. Not used for common registers.
 * @param data Data to be written.
 */
void w5500_t::write_u8(w5500_reg_t reg, uint8_t sn, uint8_t data){
    write(reg, sn, &data, 1);
}


/**
 * Write a 16-bits word to a register (big-endian).
 *
 * @param reg Register to be written.
 * @param sn Socket number. Not used for common registers.
 * @param data Data to be written.
 */
void w5500_t::write_u16(w5500_reg_t reg, uint8_t sn, uint16_t data){
    uint8_t buf[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xff)};
    write(reg, sn, buf, 2);
}


/**
 * Read data.
 *
 * @param reg First register to be read.
 * @param sn Socket number. Not used for common registers.
 * @param buf Destination buffer.
 * @param len Number of bytes to be read.
 */
void w5500_t::read(w5500_reg_t reg, uint8_t sn, uint8_t* buf, size_t len){
    sel(true);
    frame_head(reg, sn, false, len);
    for (size_t i = 0; i < len; ++i)
        buf[i] = spi_byte(0x00);
    sel(false);
}


/**
 * Read a byte from a register.
 *
 * @param reg Register to be read.
 * @param sn Socket number. Not used for common registers.
 * @return Value of the register.
 */
uint8_t w5500_t::read_u8(w5500_reg_t reg, uint8_t sn){
    uint8_t result;
    read(reg, sn, &result, 1);
    return result;
}


/**
 * Read a 16-bits word from a register.
 *
 * @param reg Register to be read.
 * @param sn Socket number. Not used for common registers.
 * @return Value of the register.
 */
uint16_t w5500_t::read_u16(w5500_reg_t reg, uint8_t sn){
    uint8_t buf[2];
    read(reg, sn, buf, 2);
    return ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
}


/**
 * Read a 16-bits word from a register, multiple times until the value is
 * stable. This is used for dealing with buffer management registers, as
 * recommended in the datasheet.
 *
 * @param reg Register to be read.
 * @param sn Socket number. Not used for common registers.
 * @return Last read value of the register.
 */
uint16_t w5500_t::read_u16_stable(w5500_reg_t reg, uint8_t sn){
    uint16_t a = read_u16(reg, sn);
    for (;;){
        uint16_t b = read_u16(reg, sn);
        if (b == a){
            return b;
        } else {
            a = b;
        }
    }
}


/**
 * Put the ethernet controller in reset state or not.
 *
 * @param state true to enable reset (PA1 will go low), false to leave reset
 *     (PA1 will go high).
 */
void w5500_t::rst(bool state) const {
    if (state) {
        gpioa.odr &= ~(1 << 1);
    } else {
        gpioa.odr |= (1 << 1);
    }
}


/**
 * Controls the SPI chip select pin of the Ethernet controller.
 *
 * @param state true to select (PA4 will go low), false to deselect (PA4 will go
 *     high).
 */
void w5500_t::sel(bool state) const {
    if (state) {
        gpioa.odr &= ~(1 << 4);
    } else {
        gpioa.odr |= (1 << 4);
    }
}


/**
 * Wait until SPI transmit buffer is empty.
 */
void w5500_t::spi_wait_txe() const {
    while ((this->spi->sr & (1 << 1)) == 0){}
}


/**
 * Wait until SPI receive buffer has data.
 */
void w5500_t::spi_wait_rxne() const {
    while ((this->spi->sr & (1 << 0)) == 0){}
}


/**
 * Perform a SPI byte transaction.
 *
 * @param tx_data Byte to be sent.
 * @return Received byte.
 */
uint8_t w5500_t::spi_byte(uint8_t tx_data){
    spi_wait_txe();
    this->spi->dr = tx_data;
    spi_wait_rxne();
    return this->spi->dr;
}


/**
 * Constructor
 *
 * @param dev_ W5500 low-level controller.
 * @param no_ Socket number in the W5500. From 0 to 7 included.
 */
socket_t::socket_t(w5500_t* dev_, uint8_t no_): dev(dev_), no(no_) {
    if (no >= w5500_t::max_sockets)
        panic();
}


/**
 * @return Socket number in the W5500.
 */
uint8_t socket_t::get_no() const {
    return no;
}


/**
 * Await for an incoming connection.
 *
 * @return true if a client connected successfully. false if connection failed.
 */
bool socket_t::listen(uint16_t port){
    assert(get_status() == socket_status_t::closed);
    // Set to TCP mode and configure source port
    dev->write_u8(w5500_reg_t::sn_mr, no, 1);
    dev->write_u16(w5500_reg_t::sn_port0, no, port);
    command(socket_command_t::open);
    assert(get_status() == socket_status_t::init);
    command(socket_command_t::listen);
    for (;;){
        socket_status_t st = get_status();
        switch (st){
            case socket_status_t::listen: break;
            case socket_status_t::synrecv: break;
            // synack is undocumented, but has been encountered many times.
            case socket_status_t::synack: break;
            // Close wait if the client asked socket close very fast.
            // We consider the connection has been established, and maybe there
            // are data to be read.
            case socket_status_t::close_wait:
            case socket_status_t::established: return true;
            case socket_status_t::closed: return false;
            default:
                debug_print("Unexpected socket status while listening: ");
                debug_print_i32((int32_t)st);
                debug_println("");
                panic();
        }
        // Reload watchdog
        iwdg.kr = 0xaaaa;
    }
}


/**
 * Establishes a connection.
 *
 * @param ip Destination IP address. 4 bytes (IPv4 only).
 * @param port Destination port.
 * @return true If connection is successful.
 */
bool socket_t::connect(uint8_t* ip, uint16_t port){
    assert(get_status() == socket_status_t::closed);
    // Set to TCP mode, configure destination port and IP address.
    dev->write_u8(w5500_reg_t::sn_mr, no, 1);
    command(socket_command_t::open);
    assert(get_status() == socket_status_t::init);
    // Clear interrupts
    dev->write_u8(w5500_reg_t::sn_ir, no, 0xff);
    dev->write_u16(w5500_reg_t::sn_port0, no, port);
    dev->write_u16(w5500_reg_t::sn_dport0, no, port);
    dev->write(w5500_reg_t::sn_dipr0, no, ip, 4);
    command(socket_command_t::connect);
    // We loop until the state is closed or established.
    for (;;){
        socket_status_t st = get_status();
        switch (st){
            // 0x19 sometimes met. This is undocumented in the datasheet and I
            // did not find anything about this value... This is probably an
            // intermediate state.
            case socket_status_t::undoc_19:
            case socket_status_t::init:
            case socket_status_t::syn:
            case socket_status_t::synsent: break;
            case socket_status_t::established: return true;
            // The socket may be closed by the host very fast.
            // The bit CON of the Sn_IR register tells if the connection has
            // been successful.
            case socket_status_t::closed: {
                uint8_t ir_val = dev->read_u8(w5500_reg_t::sn_ir, no);
                return (ir_val & 1);
            }
            default:
                debug_print("Unexpected socket status while connecting: ");
                debug_print_i32((int32_t)st);
                debug_println("");
                panic();
        }
    }
}


/**
 * Writes data to the socket.
 *
 * @param src Data buffer.
 * @param len Number of bytes to be written.
 */
void socket_t::write(const uint8_t* src, size_t len){
    uint16_t free = dev->read_u16_stable(w5500_reg_t::sn_tx_fsr0, no);
    assert(free > len);
    uint16_t tx_pointer = dev->read_u16_stable(w5500_reg_t::sn_tx_rd0, no);
    uint16_t chunk_size = (uint16_t)len;
    uint32_t tx_buf_addr = (uint32_t)w5500_reg_t::tx_buf + (uint32_t)tx_pointer;
    dev->write((w5500_reg_t)tx_buf_addr, no, src, chunk_size);
    // Update TX pointer
    tx_pointer += chunk_size;
    dev->write_u16(w5500_reg_t::sn_tx_wr0, no, tx_pointer);
    command(socket_command_t::send);
}


/**
 * @return Number of bytes available to be read.
 */
size_t socket_t::avail() {
    return dev->read_u16_stable(w5500_reg_t::sn_rx_rsr0, no);
}


/**
 * Reads data from the socket.
 *
 * @param dst Buffer where the data is written.
 * @param len Number of bytes to be read.
 * @return Number of bytes read. May be lower than len if the socket has been
 *     closed.
 */
size_t socket_t::read_exact(uint8_t* dst, size_t len){
    size_t received = 0;
    size_t remaining = len;
    while (len - received) {
        size_t n = min(avail(), remaining);
        if (n) {
            uint16_t rdp = dev->read_u16(w5500_reg_t::sn_rx_rd0, no);
            uint32_t rx_buf_addr = (uint32_t)w5500_reg_t::rx_buf +
                (uint32_t)rdp;
            dev->read((w5500_reg_t)rx_buf_addr, no, dst, n);
            dev->write_u16(w5500_reg_t::sn_rx_rd0, no, rdp + n);
            received += n;
            command(socket_command_t::recv);
        } else {
            socket_status_t st = get_status();
            switch (st) {
                case socket_status_t::established: break;
                default:
                    debug_print("Unexpected socket status while reading: ");
                    debug_print_i32((int32_t)st);
                    debug_println("");
                    panic();
            }
        }
    }
    return received;
}


/**
 * Reads all data available from the socket and returns.
 *
 * @param dst Buffer where the data is written.
 * @param len Maximum number of bytes to be read.
 * @return Number of bytes read.
 */
size_t socket_t::read_avail(uint8_t* dst, size_t len){
    size_t n = min(len, avail());
    return read_exact(dst, n);
}


/**
 * @return Socket status.
 */
socket_status_t socket_t::get_status() {
    return static_cast<socket_status_t>(dev->read_u8(w5500_reg_t::sn_sr, no));
}


/**
 * Close the socket.
 */
void socket_t::close() {
    command(socket_command_t::close);
    assert(get_status() == socket_status_t::closed);
}


/**
 * Disconnect the TCP connection and close socket.
 */
void socket_t::disconnect() {
    command(socket_command_t::discon);
    for (;;){
        socket_status_t st = get_status();
        switch (st) {
            case socket_status_t::closing:
            case socket_status_t::time_wait:
            case socket_status_t::fin_wait: break;
            case socket_status_t::closed: return;
            default:
                debug_print("Unexpected socket status while disconnecting: ");
                debug_print_i32((int32_t)st);
                debug_println("");
                panic();
        }
    }
}


/**
 * Write a command in the socket command register.
 *
 * @param command Command to be executed.
 */
void socket_t::command(socket_command_t command){
    dev->write_u8(w5500_reg_t::sn_cr, no, (uint8_t)command);
}


/**
 * Print a null terminated string.
 *
 * @param s String.
 */
void socket_t::print(const char* s){
    write((const uint8_t*)s, strlen(s));
}

