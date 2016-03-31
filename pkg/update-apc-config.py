#!/usr/bin/env python

########################################################################
# 1. To support multiple APC configuration, unique name is required
#    to identify different configurations.
#    If the name is apc-6036E3, 
#    the following configuration file will be used
#    /opt/dust-apc/conf/apc-6036E3.conf
#    /opt/dust-apc/etc/supervisor.conf.d/apc-6036E3.conf
# 2. Currently support host, port, api-device, reset-device, new 
#    parameter can be added if necessary.
# 3. Command is status, add, update, delete, default to status.
# 4. status have an optional parameter --name to display details
#    otherwise display brief for all APC configuration
# 5. Under add command, port is fixed for 9100. 
# 6. User can use update command to modify port. 
########################################################################

import os
import sys

import argparse
import glob
import re
from configobj import ConfigObj
import serial
import time

from subprocess import check_call

APC_HOME = os.path.join(os.path.sep, "opt", "dust-apc")

APC_CONF_DIR = os.path.join(APC_HOME, "conf")
APC_CONF_TEMPLATE = 'apc.conf.template'

SUPERVISOR_CONF_DIR = os.path.join(APC_HOME, "etc", "supervisor.conf.d")
SUPERVISOR_CONF_TEMPLATE = 'apc-prog.conf.template'

STUNNEL_CONF_DIR = os.path.join(APC_HOME, "etc", "stunnel")
STUNNEL_SYS_DIR = os.path.join(os.path.sep, "etc", "stunnel")
STUNNEL_CONF_TEMPLATE = 'apc_stunnel.conf.template'

UDEV_RULES_DIR = os.path.join(APC_HOME, "etc", "udev.rules.d")

USB_BY_ID_DIR = os.path.join(os.path.sep, "dev", "serial", "by-id")

template_vars = { 'apc_client_id': '',
                  'manager_host': 'localhost',
                  'api_device': '',
                  'reset_device': '',
                }

################### support functions ######################

# TCP port is 2 bytes, which can have value 0 - 65535
def id_port(x):
    x = int(x)
    if x < 0 or x > 65535:
        raise argparse.ArgumentTypeError("Invalid Port value, must be 0 - 65535")
    return x


def create_file_from_template(template_file, target_file):
    with open(template_file) as inf:
        sconf_data = inf.read()
    sconf_out = sconf_data.format(**template_vars)
    with open(target_file, 'w') as outf:
        outf.write(sconf_out)
    return True


def truncate_str(nameStr, maxLen):
    if len(nameStr) > maxLen:
        return nameStr[:maxLen] + '+'
    else:
        return nameStr


def get_mac_addr(serial_port):
    ser = serial.Serial()
    ser.baudrate = 9600
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.timeout = 2
    ser.xonoff = True
    ser.rtscts = False
    ser.dsrdtr = False
    ser.writeTimeout = 2
    ser.port = serial_port

    macStr = None

    try:
        ser.open()
    except Exception, e:
        print "error open serial port: " + str(e)
   
    if ser.isOpen():
        try:
            ser.flushInput()
            ser.flushOutput()
            ser.write("minfo\n")
            ser.flush()
            # since CLI port is 9600, it is slow, needs some sleep
            time.sleep(0.3)
            bytesToRead = ser.inWaiting()
            if bytesToRead:
                minfo = ser.read(bytesToRead)
                # find the mac from the minfo string, we need the last 6 digits
                pattern = r'(\w\w:){5}(?P<mac1>\w\w):(?P<mac2>\w\w):(?P<mac3>\w\w)'
                match = re.search(pattern, minfo)
                macStr = "{0}{1}{2}".format(match.group('mac1'), 
                                            match.group('mac2'),
                                            match.group('mac3'),)
            else:
                print "read failed"
            ser.close()
        except Exception, e:
            print "serial read/write failed" + str(e)
    else:
        print "can not open serial port " + ser.portstr

    return macStr

def apply_config():
    # load supervisorctl configuration and start APCs
    try:
        check_call(['sudo', 'supervisorctl', 'reread'])
        check_call(['sudo', 'supervisorctl', 'update'])
    except Exception as e:
        print e

##################### command handlers #################

def status(args):
    if args.name:
        # display details for the particular configuration
        print args.name
        apcConfFile = os.path.join(APC_CONF_DIR, args.name + '.conf')
        if (os.path.isfile(apcConfFile)):
            apcConf = ConfigObj(apcConfFile)
            for item in apcConf:
                print "  {0}: {1}".format(item, apcConf.get(item))
        else:
            print "  Configuration for {0} does not exist!".format(args.name)
    else:
        # display all available configuration
        print "Name         Host              Port    api-device       reset-device"
        print "----------   ---------------   -----   --------------   --------------"
        superFilePath = os.path.join(SUPERVISOR_CONF_DIR, "*.conf")
        for superFileName in sorted(glob.glob(superFilePath)):
            # find the APC program name
            superConf = open(superFileName).read()
            match = re.search(r'(\[program:)(?P<name>\S*)\]', superConf)

            # read the APC conf file
            apcConfFile = os.path.join(APC_CONF_DIR, match.group('name') + '.conf')
            apcConf = ConfigObj(apcConfFile)

            nameStr = truncate_str(match.group('name'), 10)
            hostStr = truncate_str(apcConf.get('host'), 15)
            apiDevStr = truncate_str(apcConf.get('api-device'), 14)
            resetDevStr = truncate_str(apcConf.get('reset-device'), 14)

            STATUS_TMPL = "{:11s}  {:16s}  {:6s}  {:15s}  {:15s}"
            msg = STATUS_TMPL.format(nameStr,
                                     hostStr,
                                     apcConf.get('port'),
                                     apiDevStr,
                                     resetDevStr,)
            print msg

    
def add_apc(args):
	# add command APC conf will always point to localhost and 
	# stunnel conf will point to remote manager host
	# update command will change APC conf, does not touch stunnel
    template_vars['apc_client_id'] = args.name
    template_vars['manager_host'] = args.host
    template_vars['api_device'] = args.api_device
    template_vars['reset_device'] = args.reset_device

    apcFileName = '{0}.conf'.format(args.name)
    superConfTemp = os.path.join(SUPERVISOR_CONF_DIR, SUPERVISOR_CONF_TEMPLATE)
    superConfFile = os.path.join(SUPERVISOR_CONF_DIR, apcFileName)
    ret = create_file_from_template(superConfTemp, superConfFile)
    if ret:
        print " {0} created".format("supervisor conf file")
    else:
        print "  {0} already exists".format("supervisor conf file")
    
    apcConfTemp = os.path.join(APC_CONF_DIR, APC_CONF_TEMPLATE)
    apcConfFile = os.path.join(APC_CONF_DIR, apcFileName)
    create_file_from_template(apcConfTemp, apcConfFile)
    if ret:
        print " APC conf file for {0} created".format(args.name)
        # apply supervisor configuration
        apply_config()
    else:
        print " APC conf file for {0} already exists".format(args.name)

   

def update_apc(args):
    apcFileName = '{0}.conf'.format(args.name)
    apcConfFile = os.path.join(APC_CONF_DIR, apcFileName)
    apcConf = ConfigObj(apcConfFile)
    updated = False
    
    if not os.path.exists(apcConfFile):
        raise IOError("Configuration for APC {0} does not exist.".format(args.name))
        return None

    # update host if it is provided
    if args.host != None:
        apcConf['host'] = args.host
        updated = True

    # update port if it is provided
    if args.port != None:
        apcConf['port'] = args.port
        updated = True

    # update api-device if it is provided
    if args.api_device != None:
        apcConf['api-device'] = args.api_device
        updated = True

    # update reset-device if it is provided
    if args.reset_device != None:
        apcConf['reset-device'] = args.reset_device
        updated = True

    if updated:
        apcConf.write()
        # ConfigObj write in ' = ' format, so we need this extra 
        # step to remove the spaces
        data = open(apcConfFile).read()
        out = open(apcConfFile, "w")
        out.write(re.sub(" = ", "=", data))
        out.close()
        print 'Updated configuration for APC {0}'.format(args.name)
        # apply supervisor configuration
        apply_config()
    else:
        print 'Configuration for APC {0} not changed'.format(args.name)

def delete_apc(args):
    if args.name.upper() == 'ALL':
        print "Delete all APC configurations"
        superFileList = os.path.join(SUPERVISOR_CONF_DIR, "*.conf")
        for superConfFile in sorted(glob.glob(superFileList)):
            # find the APC program name
            superConf = open(superConfFile).read()
            match = re.search(r'(\[program:)(?P<name>\S*)\]', superConf)
            apcConfFile = os.path.join(APC_CONF_DIR, match.group('name') + '.conf')
            try:
                os.remove(superConfFile)
                os.remove(apcConfFile)
            except Exception as e:
                print ' failed to remove {0}.'.format(match.group('name'))
                raise e
            else:
                print " removed {0}".format(match.group('name'))
    else:
        print "Delete one APC configuration"
        apcFileName = '{0}.conf'.format(args.name)
        superConfFile = os.path.join(SUPERVISOR_CONF_DIR, apcFileName)
        apcConfFile = os.path.join(APC_CONF_DIR, apcFileName)
        try:
            os.remove(superConfFile)
            os.remove(apcConfFile)
        except Exception as e:
            print ' fail to remove {0}.'.format(args.name)
            raise e
        else:
            print " removed {0}.".format(args.name)
    # apply supervisor configuration
    apply_config()


def auto(args):
    # 1. turn on plug and play feature
    # 1. scan for USB and retrieve MAC address
    # 2. create APC configuration files
    # 3. supervisorctl reread/update

    # install dev rules for plug and play
    udevRulesFile = os.path.join(UDEV_RULES_DIR, '88-dust-usbs.rules')    
    udevRulesDest = os.path.join(os.path.sep, 'etc', 'udev', 'rules.d')
    ret = check_call(['sudo', 'cp', udevRulesFile, udevRulesDest])
    if ret:
        print " failed to install udev rules, AP mote plug and play will not work!"
    else:
        print " installed udev rules, APC configuration will be created automatically when an AP mote is connected"

    devFilePath = os.path.join(USB_BY_ID_DIR, "*")
    for devFullName in sorted(glob.glob(devFilePath)):
        if "if02" in devFullName:
            devFileName = os.path.basename(devFullName)

            # retrieve the devStr from the USB device
            match = re.search(r'(usb-)(?P<devStr>\S*)(-if\S*)', devFileName)
            devStr = match.group('devStr')

            # send CLI minfo command to retrieve the MAC address
            # the devStr may not contain MAC address
            macStr = get_mac_addr(devFullName)
            if macStr == None:
                # wait 5 seconds for AP to boot up and retry 
                time.sleep(5)
                macStr = get_mac_addr(devFullName)
                if macStr == None:
                    print " failed to get mac address from USB device, please make sure USB device is ready and try again."
                    sys.exit()
            
            # replace the parameters and reuse add_apc function
            args.name = "apc-{0}".format(macStr)
            args.host = "localhost"
            args.api_device = devFullName.replace("if02", "if03")
            args.reset_device = devFullName.replace("if02", "if00")
            print "Create APC configuration for {0}".format(devStr)
            add_apc(args)
    # apply supervisor configuration
    apply_config()

def stunnel(args):
    if args.host:
        stunnelConfTemp = os.path.join(STUNNEL_CONF_DIR, STUNNEL_CONF_TEMPLATE)
        stunnelConfFile = os.path.join(STUNNEL_CONF_DIR, 'apc_stunnel.conf')
        template_vars['manager_host'] = args.host
        ret = create_file_from_template(stunnelConfTemp, stunnelConfFile)
        if ret:
            print " {0} created".format(stunnelConfFile)
        # apc stunnel only need to do once per host, if file already exist,
        # we assume stunnel is already running and should skip
        stunnelTargetFile = os.path.join(os.path.sep, 'etc', 'stunnel', 'apc_stunnel.conf')
        ret = check_call(['sudo', 'cp', stunnelConfFile, '/etc/stunnel'])
        if ret:
            print " failed to install stunnel conf file"
        else:
            print " installed stunnel conf file"
        ret = check_call(['sudo', 'service', 'stunnel4', 'stop'])
        if ret:
            print " failed to stop stunnel"
        else:
            print " stopped stunnel"
        ret = check_call(['sudo', 'stunnel4', stunnelTargetFile])
        if ret:
            print " failed to start stunnel"
        else:
            print " started stunnel"
    else:
        print "Stunnel configuration"
        stunnelConfFile = os.path.join(STUNNEL_SYS_DIR, 'apc_stunnel.conf')
        if (os.path.isfile(stunnelConfFile)):
            stunnelConf = ConfigObj(stunnelConfFile)
            for item in stunnelConf['tcp']:
                print "  {0}: {1}".format(item, stunnelConf['tcp'][item])
        else:
            print "  APC stunnel configuration does not exist!"

def main(argv):
    parser = argparse.ArgumentParser(version = '1.0.1')

    subparsers = parser.add_subparsers(help='commands', dest='subparser_name')

    # status command
    status_parser = subparsers.add_parser('status', help='list of APC configurations')
    status_parser.add_argument('--name',
                            help='name of an APC configuration',
                           )
    status_parser.set_defaults(func=status)

    # add command
    add_parser = subparsers.add_parser('add', help='add a new APC configuration')
    add_parser.add_argument('--name',
                            required=True,
                            help='name of the APC configuration',
                           )
    add_parser.add_argument('--host',
                            default = 'localhost',
                            help='host name or IP address of the VManager',
                           )
    add_parser.add_argument('--api-device',
                            required=True,
                            help='serial device to communicate with the AP mote Serial API',
                           )
    add_parser.add_argument('--reset-device',
                            required=True,
                            help='serial device exposing the AP mote reset line',
                           )
    add_parser.set_defaults(func=add_apc)

    # update command
    update_parser = subparsers.add_parser('update', help='update an existing APC configuration')
    update_parser.add_argument('--name',
                            required=True,
                            help='name of the APC configuration',
                           )
    update_parser.add_argument('--host',
                            help='host name or IP address of the VManager',
                           )
    update_parser.add_argument('--port',
                            type = id_port,
                            help='TCP port of the VManager',
                           )
    update_parser.add_argument('--api-device',
                            help='serial device to communicate with the AP mote Serial API',
                           )
    update_parser.add_argument('--reset-device',
                            help='serial device exposing the AP mote reset line',
                           )
    update_parser.set_defaults(func=update_apc)

    # delete command
    # delete a specific APC configuration or delete all
    # if name is *, delete all
    delete_parser = subparsers.add_parser('delete', help='delete an existing APC configuration')
    delete_parser.add_argument('--name',
                            required=True,
                            help='name of the APC configuration, or * to delete all',
                           )
    delete_parser.set_defaults(func=delete_apc)

    # auto command
    # Scan for AP and create APC configuration automatically
    auto_parser = subparsers.add_parser('auto', help='create an APC configuration using default configuration')
    auto_parser.set_defaults(func=auto)

    # stunnel command
    # Will create and start stunnel if --host is present
    # will display stunnel conf if --host is omitted
    stunnel_parser = subparsers.add_parser('stunnel', help='create and start stunnel for the APC')
    stunnel_parser.add_argument('--host',
                            help='host name or IP address of the VManager',
                           )
    stunnel_parser.set_defaults(func=stunnel)

    # default to status command if no parameter is provided
    if (len(argv) == 0):
        args = parser.parse_args(['status'])
    else:
        args = parser.parse_args()

    args.func(args)

if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except Exception as e:
        print e


