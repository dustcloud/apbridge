#!/usr/bin/env python
'''
CLI Console
'''

import cmd
import getpass
import os
import platform
import signal
import struct
import subprocess
from subprocess import check_call
import sys
import threading
import zmq
import psutil

if platform.system() in ['Windows']:
    APC_HOME = os.environ.get('VOYAGER_PRJ')
    APC_LIB_PATH = os.path.join(os.environ['VOYAGER_PRJ'], 'python', 'lib')
else :
    APC_HOME = os.environ.get('APC_HOME', '/opt/dust-apc')
    APC_LIB_PATH = os.path.join(APC_HOME, 'lib')
    
sys.path += [os.path.join(APC_LIB_PATH),
             os.path.join(APC_LIB_PATH, 'pyvoyager', 'enum'),
             os.path.join(APC_LIB_PATH, 'pyvoyager', 'proto'),
            ]

from pyvoyager.apc_console_version import VERSION
from pyvoyager.listener.logconsole import LogConsole
from pyvoyager.baserpcclient import RpcError
from pyvoyager.baserpcclient import TimeoutError
from pyvoyager import CLICommon
from pyvoyager import commonUtils
from pyvoyager.CLICommon import printError
from pyvoyager.CLICommon import printInfo
from pyvoyager.CLICommon import printRPCError
from pyvoyager.client.apcClient import ApcClient
from pyvoyager.proto.logging import logevent_pb2
from pyvoyager.enum.RpcCommon_enum import Enum_RpcResult

#======================== Defines ====================================
HIDE_RESULT = False

CONSOLE_VERSION = "Version " + '.'.join([str(v) for v in VERSION])

BANNER = '''Welcome to the APC CLI Console on {0}
{1}
'''.format(platform.system(), CONSOLE_VERSION)

# RPC services assigment
LOGGER_SERVICE = CLICommon.getRPCServiceName('LOG_RPC_SERVICE')

APC_NAME = 'apc'
APC_CLIENT_NAME  = 'apcClient'

# disable CTRL_C in console
signal.signal(signal.SIGINT, signal.SIG_IGN)

class CommandLineHandler(cmd.Cmd):
    '''Voyager Console command line handler
    '''

    def __init__(self, rpcClientsDic, listenersDic):
       cmd.Cmd.__init__(self)
       self.prompt = '$> '
       self.listenersDic = listenersDic
       self.apcClient  = rpcClientsDic[APC_CLIENT_NAME]
       
    def quitApp(self):
       # Unsubscribe all subscribed listeners
       self.unsubscribeAllListeners()
       sys.stdout.write("\n")
       return

    def cmdloop(self):
        try:
           cmd.Cmd.cmdloop(self)
        except KeyboardInterrupt as e:
           sys.stdout.write('\n')
           self.cmdloop()
        except Exception as e:
           printError(obv=str(e))
           self.cmdloop()

    #======================  CLI commands ============================

    def do_trace(self, *args):
       '''Usage: trace <notifType|all> <on|off>
       Turn on/off traces a specific notify type.
       '''

       cmd = args[0].split(" ")

       if len(cmd) != 2:
          printError("INVALID_CL_ARGS")
          return

       notifType = cmd[0]

       if cmd[1] == 'off':
           fEnable = False
       elif cmd[1] == 'on':
           fEnable = True
       else:
           printError("INVALID_CL_ARGS")
           return

       if notifType.upper() == "ALL":
          try:
             for key, listener in self.listenersDic.items():
                if fEnable:
                   listener.enableTraceAll()
                   printInfo("Trace enabled for all")
                else:
                   listener.disableTrace()
                   printInfo("Trace disabled for all")
          except Exception as e:
             printError(obv=str(e))
          return

       try:
          listener = self.findListener(notifType)
          if not listener:
             printError(obv="Invalid notify type '{0}'".format(notifType))
             return
          if fEnable:
             self.subscribeListener(listener,notifType)
             self.listenersDic[listener].enableTrace(notifType)
             printInfo("Trace enabled for "+notifType)
          else:
             self.listenersDic[listener].disableTrace(notifType)
             printInfo("Trace disabled for "+notifType)
       except KeyError:
          printError(obv="Notification type '{0}' not subscribed".format(notifType))
       except Exception as e:
          printError(obv=str(e))
            

    def do_logger(self, *args):
       '''Usage: logger [logger_name]
       List all loggers, or list details of a given logger
       Optional: logger_name -- List logger level
       '''

       arg_str = args[0].strip()
       if arg_str:
          logger_req = logevent_pb2.GetLoggerRequest()
          logger_req.logger = arg_str
          try:
             resp = self.apcClient.send(logevent_pb2.GET_LOG_LEVEL,
                                        logger_req.SerializeToString(),
                                        LOGGER_SERVICE)
          except Exception as e:
             return

          (rc, ) = struct.unpack("!L", resp[1])
          if not rc:
             logger_resp = logevent_pb2.GetLogLevelResponse.FromString(resp[2])
             sys.stdout.write('Logger: {0}, rc: {1}, level={2}\n'.format(
                                                                  arg_str,
                                                                  rc,
                                                                  logger_resp.__str__(),
                                                                  ))
          else:
             sys.stdout.write('Logger: {0}, rc: {1}\n'.format(arg_str,
                                                             Enum_RpcResult.to_string(rc)))
       else:
          try:
             (rc, loggers) = self.getAllLoggers()
          except Exception as e:
              return

          if not rc:
             sys.stdout.write('Loggers: {0}\n'.format(', '.join(loggers)))

    def do_quit(self, *args):
       return True

    def do_exit(self, *args):
       return True

    def do_logout(self, *args):
       try:
          self.quitApp()
       except Exception as e:
          printError(obv=str(e))
       return True

    def do_clear(self, *args):
       if platform.system() == 'Windows':
          subprocess.call("cls", shell=True)
       else:
          subprocess.call("clear", shell=True)

    def do_version(self, *args):
       printInfo(CONSOLE_VERSION)

    def emptyline(self):
       ##Avoid resend previous cmd line with a simple enter
       print ''


    def do_subscribe(self, *args):
       '''Usage: subscribe [notifType|ALL]
       Subscribe to specific notification type.
       If a logger is specified, the logger's log level is changed
       to DEBUG. If no argument provided, the command lists all
       notifications grouped by listener.
       '''

       if len(args[0].split(" ")) > 1:
          printError("INVALID_CL_ARGS")
          return

       arg_str = args[0].strip()

       try:
          if arg_str.upper() == "ALL":
             for key, listener in self.listenersDic.items():
                for msgType in listener.getNotifTypes():
                    self.subscribeToSpecificNotifType(msgType)
          elif arg_str:
             # Subscribe to a specific notification
             self.subscribeToSpecificNotifType(arg_str)
          else:
             # list notifications for all listeners
             for listenerName, listener in self.listenersDic.items():
                notifTypesList = listener.getNotifTypes()
                notifTypesStr = ", ".join(notifTypesList)
                printInfo('{0}:\n {1}\n'.format(listenerName, notifTypesStr))
       except Exception as e:
           printError(obv=str(e))
           return

    def do_unsubscribe(self, *args):
       '''Usage: unsubscribe [<notifType>] [ALL]
       Unsubscribe from a specific message type. ALL will unsubscribe from all message type.
       '''
       if len(args[0].split(" ")) > 1 or args[0].strip() == '':
           printError("INVALID_CL_ARGS")
           return

       notifType = args[0].strip()
       try:
           if notifType.upper() == 'ALL':
              self.unsubscribeAllListeners()
           else:
              listener = self.findListener(notifType)
              if not listener:
                  printError(obv="Unknown notification name '{0}'".format(notifType))
                  return

              self.listenersDic[listener].unregisterNotifType(notifType)

              if self.listenersDic[listener].isFilterListEmpty():
                 self.stopListener(listener)
       except Exception as e:
          printError(obv=str(e))

       printInfo('Done')

    def do_stop(self, *args):
       ''' Usage: stop

       Stop trace and unsubscribe all
       '''
       self.unsubscribeAllListeners()

    def do_set(self, *args):
       ''' Usage: set <ap|loglevel> [args]
           ap        -- set AP parameters
                        [args] is a parameter followed by value
                               parameter are
                                   - macAddr
                                   - txPower
                                   - joinKey
                                   - apClkSource
                               value is parameter dependent
           loglevel  -- set loglevel for a logger
                        [args] is a logger followed by severity
                               logger is the output from logger command
                               severity is
                                   - FATAL (1)
                                   - ERR   (2)
                                   - WARN  (3)
                                   - INFO  (4)
                                   - DEBUG (5)
                                   - TRACE (6)
       '''
       cmdArgs = args[0].split(" ")

       if len(cmdArgs) != 3:
          printError("INVALID_CL_ARGS")
          return

       cmd = cmdArgs[0].lower()
       numCmdArgs = len(cmdArgs) - 1

       if numCmdArgs:
          cmdArgs = cmdArgs[1:]

       try:
          if cmd == "loglevel" and numCmdArgs == 2:
              self.setLogLevel(cmdArgs[0].lower(), cmdArgs[1].lower())
          elif cmd == "ap" and numCmdArgs == 2:
             cmdArgs = [x.lower() for x in cmdArgs]
             self.apcClient.rpcAP_API_setParameter(cmdArgs)
          else:
             printError("INVALID_CL_ARGS")
       except Exception as e:
          printError(obv=str(e))
          return


    #Parse get command options
    def do_get(self, *args):
       ''' Usage: get <ap|apc> [args]
           ap        -- get AP parameters
                        [args] are - macAddr
                                   - networkId
                                   - txPower
                                   - apInfo
                                   - time
                                   - apClkSource
                                   - apStatus
           apc       -- get APC parameters
                        [args] are - managerHost
                                   - managerPort
                                   - clientId
                                   - gpsState
       '''
       cmdArgs = args[0].split(" ")

       if len(cmdArgs) != 2:
          printError("INVALID_CL_ARGS")
          return

       cmd = cmdArgs[0].lower()
       numCmdArgs = len(cmdArgs) - 1

       if numCmdArgs:
          cmdArgs = cmdArgs[1:]

       try:
          if cmd == "ap" and numCmdArgs == 1:
                self.apcClient.rpcAP_API_getParameter(cmdArgs[0].lower())
          elif cmd == "apc" and numCmdArgs == 1:
                self.apcClient.rpcAPC_getParameter(cmdArgs[0].lower())
          else:
             printError("INVALID_CL_ARGS")
       except Exception as e:
          printError(obv=str(e))
          return 

    # Process statistics commands
    def do_stats(self, *args):
       '''Usage: stats <ap|manager|clear>
          ap               -- Get statistics of APC/AP connection
          manager          -- Get statistics of APC/Manager connection
          clear            -- Clear APC/AP and APC/Manager statistics
       '''
    
       cmdArgs = args[0].split()
       if len(cmdArgs) == 0:
          try:
             self.apcClient.getStats('all')
          except Exception as e:
             printError(obv=str(e))
             print "Please make sure APC is running and is connected to Manager and AP"
          return
       
       cmd = args[0].lower()

       try:
          # partial match of command is allowed
          if 'ap'.find(cmd) == 0:
             self.apcClient.getStats('ap')
          elif 'manager'.find(cmd) == 0:
             self.apcClient.getStats('manager')
          elif 'clear'.find(cmd) == 0:
             self.apcClient.clearStats()
          else:
             printError("INVALID_CL_ARGS")
       except Exception as e:
          printError(obv=str(e))
          return 

    # Reset command options
    def do_reset(self, *args):
       ''' Usage: reset ap
           ap        -- reset AP
       '''
       iArgs = args[0].split()
       if len(iArgs) == 0 or not (iArgs[0].lower() in ('ap')):
          printError("INVALID_CL_ARGS")
          return
       try:
          self.apcClient.rpcResetAP()
       except Exception as e:
          printError(obv=str(e))
          return

    def do_info(self, *args):
       ''' Usage: info

       Show the information of Manager, AP, APC and GPS
       '''
       try:
          self.apcClient.info()
       except Exception as e:
          printError(obv=str(e))
          print "Please make sure APC is running and is connected to Manager and AP"
       return
     
    def do_help(self, iArgs):
       cmd.Cmd.do_help(self, iArgs)
       return
   
    def default(self, *iArgs):
      print "*** Unknown command '{0}'. Type 'help' for a list of available commands.".format(iArgs[0])
      
    #=====================  Subscribe methods  =======================

    def unsubscribeAllListeners(self):
       '''Unsubscribe all subscribed listeners'''
       for listenerName, listener in self.listenersDic.items():
          for notifType in listener.getNotifTypes():
             if notifType not in ['pingResponse', 'moteTrace']:
                try:
                   listener.unregisterNotifType(notifType)
                except KeyError as exc:
                   pass
          if listener.isFilterListEmpty():
             self.stopListener(listenerName)

    def subscribeToSpecificNotifType(self, notifType):
       '''Subscribe to a specific notification type

       Keyword arguments:
       notifType - Notification type
       '''

       try:
           listener = self.findListener(notifType)
           if not listener:
               printError(obv="Notification type {0} not found".format(notifType))
               return
           if listener == 'logListener':
               #Change log level to DEBUG 
               self.setLogLevel(notifType, 'DEBUG', HIDE_RESULT)
           self.listenersDic[listener].registerNotifType(notifType)
       except KeyError as exc:
          printError(obv=str(exc))
          return
          
       if not listener:
          printError(obv="Notification type '{0}' not found".format(notifType))
          return
 
       if not self.listenersDic[listener].isRunning():
          listener_thread = threading.Thread(target=self.listenersDic[listener].run)
          listener_thread.start()
       
       printInfo("Subscribed to {0}".format(notifType))

    def subscribeListener(self, listenerName, notifType):
      listener = self.listenersDic[listenerName]
      listener.registerNotifType(notifType)
      if not listener.isRunning():
         listener_thread = threading.Thread(target = listener.run)
         listener_thread.start()
       
    def getAllLoggers(self):
       'Get all loggers'
       try:
          resp = self.apcClient.send(logevent_pb2.GET_LOGGERS, 
                                     "", 
                                     LOGGER_SERVICE)
       except Exception as e:
          printError(obv=str(e))
          return

       (rc, ) = struct.unpack("!L", resp[1])

       if not rc:
           loggers_resp = logevent_pb2.GetLoggersResponse.FromString(resp[2])
           return rc, loggers_resp.loggers
       else:
           return rc, []

    def stopListener(self,listener):
       '''Stop listener.

       Keyword arguments:
       listener - Listener Name
       '''

       if self.listenersDic[listener].isRunning():
           self.listenersDic[listener].stop()

    def findListener(self, notifType):
       '''Find listener which provides the notification type

       Keyword arguments:
       notifType - Notification type
       '''
       for listener in self.listenersDic:
           if notifType in self.listenersDic[listener].getNotifTypes():
               return listener
       return ''

    #=====================  Set methods  =============================

    def setLogLevel(self, loggerName, levelName,fShowResult=True):
       '''Unsubscribe all subscribed listeners.

       Keyword arguments:
       loggerName - Logger name
       levelName  - Level name
       '''

       logger_req = logevent_pb2.SetLogLevelRequest()

       try:
          logger_req.logger = loggerName
          levelName = "L_" + levelName
          logger_req.logLevel = logevent_pb2._LOGLEVEL.values_by_name[levelName.upper()].number
       except KeyError:
          printError(obv="Invalid name '{0}'".format(levelName))
          return
       try:
          self.apcClient.send_msg(logevent_pb2.SET_LOG_LEVEL, 
                                  logger_req,
                                  "",
                                  LOGGER_SERVICE)
       except TimeoutError as exc:
          printError(obv=str(exc))
          return 
       except RpcError as exc:
          if  exc.result_code == Enum_RpcResult.to_int('RPC_INVALID_PARAMETERS'):
              printRPCError('RPC_INVALID_PARAMETERS', loggerName)
          else:
              printError(obv=str(exc))
          return

       if fShowResult:
          printInfo("Done")


def main(argv):
    # Parse command line arguments    
    parser = commonUtils.MainCmdArgParser(APC_HOME, 'Console arguments.', 'console')    
    parser.add_argument('--cmd', 
                        help="Execute a single command in batch mode")
    parser.add_argument('-f','--filename',
                        help="Execute all cmds inside the file")
    parser.add_argument('-i','--interactive', type=bool, default=True,
                        help="Interactive mode")
    parser.add_argument('-n','--name', 
                        help="APC client-id")
    args = parser.parse_args()

    if not args :        
        print parser.errmsg        
        sys.exit(1)

    endPoints = commonUtils.ApiEndpointBuilder(APC_HOME, 
                                               args.api_proto, 
                                               args.api_ipcpath, 
                                               args.api_host, 
                                               args.api_port)

    if args.name:
       APC_ENDPOINT           = endPoints.getClientEndpoint('APC_ENDPOINT_PATH', args.name)
       APC_LOG_ENDPOINT       = endPoints.getClientEndpoint('APC_LOG_ENDPOINT_PATH', args.name)
    else:
       # if there is only one APC running, connec to it
       numApcRunning = 0
       apcInfo = []
       for proc in psutil.process_iter():
           try:
               pInfo = proc.as_dict(attrs = ['pid', 'name', 'cmdline'])
               if pInfo['name'] == 'apc':
                   numApcRunning = numApcRunning + 1
                   apcInfo = pInfo
           except psutil.NoSuchProcess:
               pass
       if numApcRunning == 0:
           print "APC is not running, please use apcctl to start APC then try again."
           sys.exit(1)
       elif numApcRunning == 1:
           try:
               location = apcInfo['cmdline'].index('--client-id')
               # the APC client-id value is the next
               APCName = apcInfo['cmdline'][location+1]
               APC_ENDPOINT = endPoints.getClientEndpoint('APC_ENDPOINT_PATH', APCName)
               APC_LOG_ENDPOINT = endPoints.getClientEndpoint('APC_LOG_ENDPOINT_PATH', APCName)
           except ValueError:
               print "APC is running without client-id, please check."
               sys.exit(1)
       else:
           print "{0} APCs are running, don't know which one to connect to. Please retry with -n option.".format(numApcRunning)
           sys.exit(1)
       
    ctx = zmq.Context()

    # Print BANNER
    print BANNER

    # apc RPC client
    apc = ApcClient(ctx, APC_ENDPOINT)

    rpcClientsDic = {APC_CLIENT_NAME: apc }

    # Notification listeners
    log_console = LogConsole(ctx, "logListener", APC_LOG_ENDPOINT,
                             rpcClientsDic[APC_CLIENT_NAME])
    listenersDic = {'logListener': log_console }

    commander = CommandLineHandler(rpcClientsDic, listenersDic)

    if args.cmd:
       l = commander.precmd(args.cmd)
       r = commander.onecmd(l)
       r = commander.postcmd(r, l)
       if not args.interactive:
          #To unsubscribe listeners(stop threads).
          commander.unsubscribeAllListeners()

    elif args.filename:
        try:
            fd = open(args.filename, 'r')
            printInfo('Loading commands from {0}'.format(args.filename))
            for cmd in fd:
                cmd = cmd.strip()
                if cmd.startswith('#'):
                  continue
                printInfo('exec: {0}'.format(cmd))
                l = commander.precmd(cmd)
                r = commander.onecmd(l)
                r = commander.postcmd(r, l)   
            fd.close()
        except IOError as exc:
            printError("FILE_NOT_FOUND", str(exc))

        if not args.interactive:
            #To unsubscribe listeners(stop threads).
            commander.unsubscribeAllListeners()
    else:
       try:
          commander.cmdloop()
       except:
          print "Command failed, please check if APC is running"
    
    # Close listeners and RPC clients
    log_console.close()

if __name__ == '__main__':
   try:
      main(sys.argv[1:])
   except Exception as e:
      printError(obv=str(e))
    
