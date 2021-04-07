import os, sys
import time
import subprocess

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

try:
    process = subprocess.Popen(['getStatus'])
    process.wait(timeout=2)
except: 
    print("[ERROR] Can't not getStatus")


process = subprocess.Popen(['/usr/local/bin/rfrespond'])
try:
    print('[INFO] rfespond: ', process.pid)
    process.wait(timeout=25)
except subprocess.TimeoutExpired:
    print('Timed out - killing rfrespond')
    process.kill()

try:
    process = subprocess.Popen(['date'])
    process.wait(timeout=2)
except: 
    print("[ERROR] Can't not print date")

process = subprocess.Popen(['/usr/local/bin/cleanFIFO'])
try:
    print('[INFO] cleanFIFO: ', process.pid)
    process.wait()
except subprocess.CalledProcessError as e:
    print("cleanFIFO Unexpected error:", e.output)
    process.kill()
    
process = subprocess.Popen(['/usr/local/bin/rfwait2'])
try:
    print('[INFO] rfwait2: ', process.pid)
    process.wait()
except subprocess.CalledProcessError as e:
    print("Rfwait Unexpected error:", e.output)
    process.kill()

time.sleep(1)

os.system("echo '======================================================================\n'")
cmd = "/sbin/shutdown now"
os.system(cmd)

