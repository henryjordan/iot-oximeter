#!/usr/local/bin/python
__author__ = 'kalcho'

import socket
#import struct

target_host = '127.0.0.1'
target_port = 51717

# create a floating socket object
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect the client
client.connect((target_host, target_port))
print("Connected to {:s}:{:d}".format(target_host, target_port))

for n in range(10):
  client.send(str.encode("".join([str(n)]))) # send some data
  response = client.recv(4096) # receive some data
  print(response.decode('utf-8'))
