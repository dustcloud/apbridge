'''commonUtils

The commonUtils implements commnad line options and login etc
'''

import getpass
import os
import platform

from argparse import ArgumentParser

from pyvoyager.enum.RpcCommon_enum import Enum_RpcEndpoints
from pyvoyager.enum.RpcCommon_enum import Enum_RpcServices
from pyvoyager.baserpcclient import RpcError
from pyvoyager.baserpcclient import TimeoutError

IPC = 'IPC'
TCP = 'TCP'
PROTOCOLS = [TCP, TCP.lower(), IPC, IPC.lower()]

DEFAULT_PROTO    = IPC
DEFAULT_PORT     = '9000'
DEFAULT_HOST     = '127.0.0.1'
DEFAULT_IPCPATH  = 'var/run'
DEFAULT_CONFPATH = 'conf'
DEFAULT_CONFEXT  = 'conf'
DEFAULT_CONF     = 'conf'

def getFullFileName(apphome, subDir, filePath) :
   if filePath == os.path.abspath(filePath) :
      return filePath
   if subDir == os.path.abspath(subDir) :
      return os.path.join(subDir, filePath)
   return  os.path.join(apphome, subDir, filePath)

def parseFieldAndValue(field_arg):
    '''Parse field and value from <field>=<value>.

    Keyword arguments:
    field_arg  --  Input Arguments
    '''
    if '=' not in field_arg:
       raise ValueError('Invalid parameter format, expected <field>=<value>.')

    firstIndex = field_arg.find('=')

    if not firstIndex:
       raise ValueError('Field name is empty.')

    return (field_arg[:firstIndex], field_arg[firstIndex+1:])

class ApiEndpointBuilder :
   def __init__(self, apphome, proto = None, ipcPath = None, host = None, port_base = None) :
      self.apphome     = apphome
      self.proto       = self.getVal(proto, DEFAULT_PROTO).upper()
      self.ipcPath     = self.getVal(ipcPath, DEFAULT_IPCPATH)
      self.host        = self.getVal(host, DEFAULT_HOST)
      self.port_base   = int(self.getVal(port_base, DEFAULT_PORT))
      if platform.system() == 'Windows' :
         self.proto = TCP

   def getServerEndpoint(self, endpointName, endpointExt = None) :
      ipcName    = endpointExt
      portOffset = 0
      if endpointExt :
         nn = endpointExt.split(':')
         if len(nn) > 1 :
            ipcName    = nn[0]
            try :
               portOffset = int(nn[1])
            except :
               pass

      endpt_str  = None
      endpt_code = Enum_RpcEndpoints.to_int(endpointName)
      if self.proto == TCP :
         endpt_str = 'tcp://*:{0}'.format(self.port_base + endpt_code + portOffset)
      elif self.proto == IPC :
         endpt_str = self.getIPCpath(endpt_code, ipcName)
      return endpt_str
      
   def getClientEndpoint(self, endpointName, endpointExt = None) :
      ipcName    = endpointExt
      portOffset = 0
      if endpointExt :
         nn = endpointExt.split(':')
         if len(nn) > 1 :
            ipcName    = nn[0]
            try :
               portOffset = int(nn[1])
            except :
               pass
            
      endpt_str  = None
      endpt_code = Enum_RpcEndpoints.to_int(endpointName)
      if self.proto == TCP :
         endpt_str = 'tcp://{0}:{1}'.format(self.host, self.port_base + endpt_code + portOffset)
      elif self.proto == IPC :
         endpt_str = self.getIPCpath(endpt_code, ipcName)
      return endpt_str
   
   def getVal(self, val, defVal) :
      if val :
         return val
      return defVal

   def getIPCpath(self, endpt_code, ipcName) :
      POSTFIX = "://"
      ipcpath = Enum_RpcEndpoints.to_string(endpt_code, True)         
      idx = ipcpath.find(POSTFIX)
      if idx >= 0 :
         idx = idx + len(POSTFIX)
      else :
          idx = 0
      fileName = ipcpath[idx:]
      if ipcName :
         idx1 = fileName.rfind('.')
         if idx1 >= 0 :
            fileName = '{0}_{1}{2}'.format(fileName[:idx1], ipcName, fileName[idx1:])
         else :
            fileName = '{0}_{1}'.format(fileName, ipcName)
      return ipcpath[:idx] + getFullFileName(self.apphome, self.ipcPath, fileName) 

def getRPCServiceName(serviceDefName):
    ''' Get service from Enum_RpcServices enumeration

    Keyword arguments:
     serviceDefName - Services definition name

    Returns: Service name
    '''
    
    srv_code = Enum_RpcServices.to_int(serviceDefName)
    return Enum_RpcServices.to_string(srv_code, True)

class MainCmdArgParser(ArgumentParser) :
   # apphome is the VOYAGER_HOME for voyager and APC_HOME for apc.
   def __init__(self, apphome, desc, appname) :
      self.apphome = apphome
      self.appname = appname
      ArgumentParser.__init__(self, description=desc)
      ArgumentParser.add_argument(self, '--api-proto',    help="API protocol", choices = PROTOCOLS, 
                                  default = DEFAULT_PROTO)
      ArgumentParser.add_argument(self, '--api-ipcpath',  help="Path to directory of IPC files (only for IPC protocol)", 
                                  default = DEFAULT_IPCPATH)
      ArgumentParser.add_argument(self, '--api-host',     help="Host or IP address of the RPC server (only for TCP protocol)", 
                                  default = DEFAULT_HOST)
      ArgumentParser.add_argument(self, '--api-port',     help="Base port for RPC services (only for TCP protocol)", 
                                  default = DEFAULT_PORT)
      ArgumentParser.add_argument(self, '-c', '--config-file',   help="Configuration file")
      ArgumentParser.add_argument(self, '-u', '--username', help="User name")
      ArgumentParser.add_argument(self, '--password', help="Password for user authentication")
      self.errmsg = ''
      
   def parse_args(self) :
      args = ArgumentParser.parse_args(self)
      
      if args.config_file :
         isConfNameSet = True
      else :
         isConfNameSet = False
         args.config_file = '{0}.{1}'.format(self.appname, DEFAULT_CONFEXT)
      configName = getFullFileName(self.apphome, DEFAULT_CONFPATH, args.config_file)
      
      isUsernameOverriden = bool(args.username)
         
      args = None
      if os.path.isfile(configName)  :
         with open(configName, "r") as cfgFile:
            fileArgs = []
            for line in cfgFile :
               comPos = line.find('#')
               if comPos >= 0 :
                  line =  line[:comPos]
               line = line.strip()
               if len(line) == 0 :
                  continue
               field, value = parseFieldAndValue(line)
               if not (field and value):
                  self.errmsg = 'Error line "{0}" in configuration file: {1}'.format(line, configName)
                  return None
               if isUsernameOverriden and field.strip() == 'password':
                  continue
               fileArgs.append('--' + field.strip())
               fileArgs.append(value.strip())
            args = ArgumentParser.parse_args(self, fileArgs)
      else :
         if isConfNameSet :
            self.errmsg = 'Can not open configuration file: {0}'.format(configName)
            return None
            
      args = ArgumentParser.parse_args(self, None, args)
      
      args.api_proto = args.api_proto.upper()
      
      return args
   
def webUnittestGetIpcNames(baseName) :
   rpc = baseName.replace(".ipc", "_rpc.ipc")         
   pub = baseName.replace(".ipc", "_mngrnotif.ipc'")         
   return (rpc, pub)
   
class ConsoleCmdParser(ArgumentParser) :
   # It is used to parse arguments of individual command. It is designed to suppress default error and help text.
   def __init__(self) :
      ArgumentParser.__init__(self, add_help=False, usage="None")
   
   def error(self, message):
      raise ValueError(message)

def rpcRespToDict(resp) :
    '''
    Convert RPC response (protobuf structure) to dictionary
    '''
    res = {}
    for f, v in resp.ListFields() :
        if f.type ==  f.TYPE_MESSAGE :
            if f.label == f.LABEL_REPEATED :
                l = []
                for item in v :
                    l.append(rpcRespToDict(item))
                res[f.name] = l
            else :
                res[f.name] = rpcRespToDict(v)
        else :
            res[f.name] = v
    return res

def statDelaysToString(stat) :
    if 'delays' in stat :
        outDelay  = stat['outThresholdEvents']
        s = ''.join((', '.join('<{}ms: {}'.format(d['threshold'], d['numEvents']) for d in stat['delays']),
                     ', >{}ms: {}'.format(outDelay['threshold'], outDelay['numEvents']),
                     '; Max delay: {:.3f}ms'.format(stat['maxDelay'])))
    else :
        s = ''
    return s
    
    
