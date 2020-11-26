import os, sys
import time
import subprocess

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

# process = subprocess.Popen(['/usr/local/bin/rfrespond'])
# try:
#     print('[INFO] rfespond: ', process.pid)
#     process.wait(timeout=15)
# except subprocess.TimeoutExpired:
#     print('Timed out - killing rfrespond')
#     process.kill()

process = subprocess.Popen(['/usr/local/bin/cleanFIFO'])
try:
    print('[INFO] cleanFIFO: ', process.pid)
    process.wait()
except subprocess.CalledProcessError as e:
    print "cleanFIFO Unexpected error:", e.output
    process.kill()
    
process = subprocess.Popen(['/usr/local/bin/rfwait'])
try:
    print('[INFO] rfwait: ', process.pid)
    process.wait()
except subprocess.CalledProcessError as e:
    print "Rfwait Unexpected error:", e.output
    process.kill()

print("Bye!\n")

cmd = "/sbin/shutdown now"
os.system(cmd)

