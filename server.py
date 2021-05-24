# Receive UDP packets transmitted by a
# broadcasting service
MYPORT = 50000
import sys
import time
from socket import *
from datetime import datetime
from datetime import date
import sys

s = socket(AF_INET, SOCK_DGRAM)
s.bind(('', MYPORT))
# print(time.time())
# print(sys.argv[3])
while 1:
    data, wherefrom = s.recvfrom(1500, 0)
    ip = str(wherefrom).split("\'")[1]
    f = open(sys.argv[1] + "_adTime.csv", "a")
    f.write(sys.argv[2] + ",\"" + ip + "\"," + str(time.time() - float(sys.argv[3])) + "\r\n")
    f.close()
    # sys.stderr.write(repr(wherefrom).split("()")[0] + '\n')
    # sys.stderr.write(str(wherefrom).split("()")[0] + '\n')
