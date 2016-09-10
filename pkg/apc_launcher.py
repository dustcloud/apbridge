#!/usr/bin/python

###################################################
#
# This python script wait for NTP sync, then replace
# itself with the real APC executable.
#
# This is needed on Raspberry Pi device, since 
# Raspberry Pi doesn't have a dedicated RTC, which
# result in incorrect clock until NTP synced. 
#
###################################################

import os
import sys
import time
import signal
import datetime
from subprocess import Popen, PIPE

NTP_TIMEOUT       = 180  # timeout in seconds, default to 3 minutes,
                         # if NTP is not working, timeout and continue
POLL_INTERVAL     = 2    # timeout in seconds, check NTP every interval

# get current time
def time_now():
    TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'
    return datetime.datetime.now().strftime(TIME_FORMAT)[:-3]

# write a timestampped log message
def log_write(log):
    sys.stdout.write("== " + time_now() + " " + log + "\n")
    sys.stdout.flush()

# handle the granceful shutdown, i.e. process the 
# SIGTERM recevied from OS
def signal_term_handler(signal, frame):
    # if we are still running python script, exit
    # if we are running AP Bridge, we won't come here
    sys.exit()

# get the output from ntpq -c rv to check NTP sync state
def read_ntp_sync():
    if not hasattr(read_ntp_sync, "last_err"):
        read_ntp_sync.last_err = 0
    try:
        p = Popen(["ntpq", "-c", "rv"], stdin=PIPE, stdout=PIPE, stderr=PIPE)
        output = p.stdout.read()
        err = p.stderr.read()
        if len(err) > 0:
            # repeated error message is printed only once
            if err != read_ntp_sync.last_err:
                log_write(err)
                read_ntp_sync.last_err = err
        return output
    except OSError as e:
        print "ntpq -c rv failed: ", e

# replace current process with apbridge
def start_apc(argv):
    if 'APC_HOME' in os.environ:
        APC_HOME = os.environ['APC_HOME']
    else:
        APC_HOME = '/opt/dust-apc'    
    APC_BIN = os.path.join(APC_HOME, 'bin', 'apc')
    log_write("Starting AP Bridge")
    os.execv(APC_BIN, ['apc'] + argv)
    
def main(argv=None):

    # set handler for graceful shutdown
    # if NTP sync is not ready, terminate apc.py
    # if NTP synced and apbridge running, apbridge will capture 
    # SIGTERM and perform graceful shutdown.

    signal.signal(signal.SIGTERM, signal_term_handler)

    # wait for ntp_sync
    log_write("Checking NTP sync")
    total_wait = 0
    while total_wait < NTP_TIMEOUT:
        ntp_status = read_ntp_sync()
        if ntp_status == None:
            # if NTP is not installed, we don't wait
            break;
        elif ntp_status.find("sync_ntp") >= 0:
            log_write("NTP synced")
            break;
        time.sleep(POLL_INTERVAL)
        total_wait += POLL_INTERVAL
        
    # start apc
    start_apc(argv)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
