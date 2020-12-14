#!/usr/bin/python3
from pwn import *
import sys
import click

def make_patched(input_elf, input_bin, output_bin, new_ip, new_mac, hide_flag):
    firm_elf = ELF(input_elf, checksec=False)
    firm_bin = open(input_bin, 'rb').read()
    base = 0x08000000
    off_ip = firm_elf.symbols['ip'] - base
    off_mac = firm_elf.symbols['mac'] - base
    off_flag = firm_elf.symbols['_ZL4flag'] - base
    assert firm_bin[off_flag:off_flag+4] == b'CTF{'
    off_flag_end = off_flag + 1
    while firm_bin[off_flag_end] != ord('}'):
        off_flag_end += 1
    flag_len = off_flag_end - off_flag
    
    new_firm_bin = bytearray(firm_bin)
    new_firm_bin[off_ip:off_ip+len(new_ip)] = new_ip
    new_firm_bin[off_mac:off_mac+len(new_mac)] = new_mac
    if hide_flag:
        new_firm_bin[off_flag+4:off_flag+4+flag_len-4] = b'?'*(flag_len-4)
    open(output_bin, 'wb').write(new_firm_bin)

@click.group()
def cli():
    pass

@cli.command(help='Patch IP and MAC')
@click.argument('ip')
@click.argument('mac')
@click.argument('input_elf')
@click.argument('input_bin')
@click.argument('output_bin')
def ipmac(ip, mac, input_elf, input_bin, output_bin):
    new_ip = bytes(int(s) for s in ip.split('.'))
    assert len(new_ip) == 4
    new_mac = bytes(int(s) for s in mac.split('.'))
    assert len(new_mac) == 6
    make_patched(input_elf, input_bin, output_bin, new_ip, new_mac, False)

@cli.command(help='Hide IP, MAC and flag')
@click.argument('input_elf')
@click.argument('input_bin')
@click.argument('output_bin')
def hide(input_elf, input_bin, output_bin):
    new_ip = bytes([0, 0, 0, 0])
    new_mac = bytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    make_patched(input_elf, input_bin, output_bin, new_ip, new_mac, True)

if __name__ == '__main__':
    cli()
