import os, sys
import time
import subprocess

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

process = subprocess.Popen(['/usr/local/bin/rfrespond'])
try:
    print('[INFO] rfespond: ', process.pid)
    process.wait(timeout=15)
except subprocess.TimeoutExpired:
    print('Timed out - killing rfrespond')
    process.kill()

# time.sleep(15) # Wait for "rfresponse" send ACK
process = subprocess.Popen(['/usr/local/bin/cleanFIFO'])
try:
    print('[INFO] cleanFIFO: ', process.pid)
    process.wait()
except subprocess.TimeoutExpired:
    print('Timed out - killing cleanFIFO')
    process.kill()
    
process = subprocess.Popen(['/usr/local/bin/rfwait'])
try:
    print('[INFO] rfwait: ', process.pid)
    process.wait()
except subprocess.TimeoutExpired:
    print('Timed out - killing rfwait')
    process.kill()

print("Bye!\n")

cmd = "/sbin/shutdown now"
os.system(cmd)

