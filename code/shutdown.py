import os, sys
import time

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

time.sleep(20) # Wait for "rfresponse" send ACK

cmd = "/usr/local/bin/rfwait2"
os.system(cmd)

time.sleep(3) # Wait for ~~~

print("Bye!\n")

cmd = "/sbin/shutdown now"
os.system(cmd)

