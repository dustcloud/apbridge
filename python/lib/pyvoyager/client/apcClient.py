'''apcClient

The apcClient implements the RPC connection with the APC Server
'''

import datetime
import struct

from collections import OrderedDict

from pyvoyager import CLICommon
from pyvoyager.proto.APInterface.rpc import apc_pb2
from pyvoyager.baserpcclient import BaseRpcClient

from pyvoyager.CLICommon import printInfo
from pyvoyager.commonUtils import rpcRespToDict
from pyvoyager.commonUtils import statDelaysToString

from pyvoyager.enum import dn_to_str
from pyvoyager.enum.GPSError_enum import Enum_gps_status_t
from pyvoyager.enum.IAPCClient_enum import Enum_apcclient_state_t

APC_RPC_SERVICE = CLICommon.getRPCServiceName('APC_RPC_SERVICE')

Commands = {
   'setParameter' : 1,
   'getParameter' : 2,
   'join'         : 6,
   'disconnect'   : 7,
   'reset'        : 8,
   }

APParameters = {
   #  parameterId and control flag
   'macaddr'           : (0x01, ('get')),
   'jkey'              : (0x02, ('set')),
   'netid'             : (0x03, ('get')),
   'txpwr'             : (0x04, ('get', 'set')),
   'apinfo'            : (0x0C, ('get')),
   'time'              : (0x0F, ('get')),
   'appInfo'           : (0x1E, ('get')),
   'clksrc'            : (0x26, ('get', 'set')),
   'apstatus'          : (0x28, ('get')),
   }


APCParameters = {
   'managerhost' : 0x01,
   'managerport' : 0x02,
   'clientid'    : 0x03,
   'gpsstate'    : 0x04,
}

StatsDict = {
   'AP/APC Statistics' : {
      # strcture: counter name, order, full name, counter value
      # because dictionary is unordered, use 'order' to determine 
      # display order, otherwise the output order is random
      'queueApCnt'     : [1, 'Packets in Queue', '0'],
      'apPktsSent'     : [2, 'Packets Sent', '0'],
      'apPktsRecv'     : [3, 'Packets Rcvd', '0'],
      'apRespRecv'     : [4, 'Responses Rcvd', '0'],
      'apNackSent'     : [5, 'Nacks Sent', '0'],
      'apNackRecv'     : [6, 'Nacks Rcvd', '0'],
      'apMaxNackCount' : [7, 'Max Nack Count', '0'],
      'apRetriesSent'  : [8, 'Retries Sent', '0'],
      'apRetriesRecv'  : [9, 'Retries Rcvd', '0'],
      'apMinResp'      : [10,'Resp Min (us)', '0'],
      'apMaxResp'      : [11,'Resp Max (us)', '0'],
      'apAvgResp'      : [12,'Resp Avg (us)', '0'],
      'apMinTimeInQueue' : [13,'Time In Queue Min (us)', '0'],
      'apMaxTimeInQueue' : [14,'Time In Queue Max (us)', '0'],
      'apAvgTimeInQueue' : [15,'Time In Queue Avg (us)', '0'],
      'numOutBuffers'    : [16,'Number of output buffers', '0'],
      'apCurrPktRate'    : [17, 'RX Current Rate(pps)',   '0.0'],
      'ap30secPktRate'   : [18, '30 sec average (pps)', '0.0'],
      'ap5minPktRate'    : [19, '5  min average (pps)', '0.0'],
      'apAvgPktRate'     : [20, 'total  average (pps)', '0.0'],
      },
   'Manager/APC Statistics' : {
      'queueMgrCnt'    : [1, 'Packets Queued', '0'],
      'mgrPktSent'     : [2, 'Packets Sent', '0'],
      'mgrPktRecv'     : [3, 'Packets Rcvd', '0'],
      },
}


InfoDict = {
    'managerhost'     :[1, 'Manager Host', 'Unknown'],
    'managerport'     :[2, 'Manager Port', 'Unknown'],
    'managerstate'    :[3, 'Manager State', 'Unknown'],
    'apmacaddress'    :[4, 'AP Mac Address', 'Unknown'],
    'apstate'         :[5, 'AP State', 'Unknown'],
    'appver'          :[6, 'AP Version', 'Unknown'],
    'clksource'       :[7, 'AP Clock Source', 'Unknown'],
    'clientid'        :[8, 'APC Client Id', 'Unknown'],
    'apcversion'      :[9, 'APC Version', 'Unknown'],
    'gpsstate'        :[10,'GPS State', 'Unknown'],
    'gpssatsused'     :[12,'Satellites Used', 'Unknown'],
    'gpssatsvisible'  :[13,'Satellites Visible', 'Unknown'],
}

def macToStr(mac, separator = '-'):
    '''Convert a MAC address in bytes to a string'''

    macStr = separator.join(["%02X" % ord(b) for b in mac])
    return macStr

def pb2dict(pb_msg):
    '''Convert a protobuf message into a dictionary by field name'''
    return {f.name: v for f, v in pb_msg.ListFields()}

def presentAPInfo(apInfo):
    # big endian
    version, serialNum, model, revision, swMajor, swMinor, swPatch, swBuild, \
    bootloader = struct.unpack('!B8sBBBBBHB', apInfo)

    out = "API protocol version: " + str(version) + "\n"
    out += "Serial Number       : " + macToStr(serialNum) + "\n"
    out += "Hardware Model      : " + str(model) + "\n"
    out += "Hardware Revision   : " + str(revision) + "\n"
    out += "Network Stack       : " + str(swMajor) + '.' + \
                                      str(swMinor) + '.' + \
                                      str(swPatch) + '.' + \
                                      str(swBuild) + "\n"
    out += "Bootloader version  : " + str(bootloader)
    return out


def presentTime(timeInfo):
    # TODO: ASN is 5 bytes, how to use struct.unpack
    upTime, utcTimeSec, utcTimeMsec, asn, asnOffset = struct.unpack('!Lql5sH', timeInfo)
    out =  "Up Time    : " + str(datetime.timedelta(0, upTime, 0)) + "\n"
    out += "UTC Time   : " + str(utcTimeSec) + "." + str(utcTimeMsec) + "\n"
    out += "Local Time : " + datetime.datetime.fromtimestamp(utcTimeSec).isoformat(' ') + "\n"
    out += "ASN        : " + str(int(asn.encode("hex"), 16)) + "\n"
    out += "ASN Offset : " + str(asnOffset)
    return out

def getAppVersion(appVer):
   major, minor, patch, build = struct.unpack('!BBHB', appVer)
   return str(major) + '.' + str(minor) + '.' + str(patch) + '.' + str(build)

def gen_payload(param_id, param_data):
   ''' A generic function generate request
   '''
   payload = struct.pack('B', param_id)
   if (param_id == APParameters['clksrc'][0] or 
       param_id == APParameters['txpwr'][0]):
      payload += struct.pack('B', int(param_data))
   elif param_id == APParameters['netid'][0]:
      payload += struct.pack('!H', int(param_data))
   elif param_id == APParameters['macaddr'][0]:
      # in format of 01-23-45-67-89-AB-CD-EF or 0123456789ABCDEF
      macStr = param_data
      if len(macStr) == 23:
         macStr = macStr.replace(macStr[2], "")
         macAddr = macStr.decode("hex")
      elif len(macStr) == 16:
         macAddr = macStr.decode("hex")
      else:
         # wrong format
         print "Invalid mac address, expect 01-23-45-67-89-AB-CD-EF or 0123456789ABCDEF"
         return ""
      payload += macAddr
   elif param_id == APParameters['jkey'][0]:
      # jkey is 16 bytes, input format is 0123456789ABCDEF...
      payload += param_data.decode('hex')
   return payload


def translate_enums(dict, InfoDict):
   if 'macAddr' in dict:
      InfoDict['apmacaddress'][2] = macToStr(dict['macAddr'])
   if 'managerState' in dict:
      InfoDict['managerstate'][2] = Enum_apcclient_state_t.to_string(dict['managerState'])
   if 'clksource' in dict:
      InfoDict['clksource'][2] = dn_to_str.dnToStrClkSrc(dict['clksource'])
   if 'apState' in dict:
      InfoDict['apstate'][2] = dn_to_str.dnToStrMoteSt(dict['apState'])
   if 'gpsState' in dict:
      InfoDict['gpsstate'][2] = Enum_gps_status_t.to_string(dict['gpsState'])


class ApcClient(BaseRpcClient):
    '''APC API Client
    
    This object provides an interface to the apc API
    '''
    
    def __init__(self, ctx, svrAddr, clientName=''):
       BaseRpcClient.__init__(self, ctx, svrAddr, APC_RPC_SERVICE, clientName)
       self.client_id = 0
       self.client_name = ''
       
    def info(self):
       ''' Show manager and AP information
       '''
       dict = {}

       try:
          # get AP info
          apInfo_resp = self.send_msg(
                            apc_pb2.GET_AP_INFO, 
                            "",
                            apc_pb2.APInfoResp,
                            service=APC_RPC_SERVICE)
          dict.update(pb2dict(apInfo_resp))

          # get APC info
          apcInfo_resp = self.send_msg(
                            apc_pb2.GET_APC_INFO, 
                            "",
                            apc_pb2.APCInfoResp,
                            service=APC_RPC_SERVICE)
          dict.update(pb2dict(apcInfo_resp))

       except Exception as e:
          raise e
          return None

       for key in dict:
          # we store lower case as key to allow case insensitive user input 
          f = key.lower()
          if f in InfoDict:
             InfoDict[f][2] = dict[key]

       # special handling for macAddress, appVersion etc
       translate_enums(dict, InfoDict)

       out = []
       dict = OrderedDict(sorted(InfoDict.items(), key=lambda t: t[1][0]))
       for key in dict:
          out += ['{0: <18}'.format(dict[key][1]) + '{0: >40}'.format(dict[key][2])]
       print '\n'.join(out)

    def rpcResetAP(self):
       ''' Reset AP
       '''
       self.send_msg(apc_pb2.AP_RESET, 
                            "",
                            "",
                            service=APC_RPC_SERVICE)
       return None


    def rpcAP_API_getParameter(self, args):
       ''' AP getParameter
       '''
       req = apc_pb2.AP_APIReq()
       req.cmdId = Commands['getParameter']
       if args in APParameters:
          if 'get' in APParameters[args][1]:
             req.payload = chr(APParameters[args][0])
          else:
             print "Unsupported operation"
       else:
          print "Unrecognized parameter"
          return None;

       try:  
          apAPI_resp = self.send_msg(
                            apc_pb2.AP_API, 
                            req,
                            apc_pb2.AP_APIResp,
                            service=APC_RPC_SERVICE)
       except Exception as e:
          raise e
          return None
       else:
          dict = pb2dict(apAPI_resp)

       # dict will only contains 2 entries, 'cmdId' and 'response'
       if len(dict) != 2:
          return None;

       respCode, cmd = struct.unpack('!BB', dict['response'][0:2])
       payload = dict['response'][2:]
       if cmd == APParameters['macaddr'][0]:
          val = macToStr(payload)
       elif cmd == APParameters['apinfo'][0]:
          val = presentAPInfo (payload)
       elif cmd == APParameters['time'][0]:
          val = presentTime(payload)
       elif cmd == APParameters['netid'][0]:
          val = struct.unpack('!H', payload)[0]
       elif cmd == APParameters['txpwr'][0]:
          val = struct.unpack('!B', payload)[0]
       elif cmd == APParameters['clksrc'][0]:
          val = struct.unpack('!B', payload)[0]
          val = dn_to_str.dnToStrClkSrc(val)
       elif cmd == APParameters['apstatus'][0]:
          val = struct.unpack('!B', payload)[0]
          val = dn_to_str.dnToStrMoteSt(val)
       elif cmd == APParameters['txpwr'][0]:
          val = struct.unpack('!B', payload)[0]
       else:
          val = 'Unknown ' + str(cmd)

       print val

       return None
    

    def rpcAP_API_setParameter(self, args):
       ''' AP setParameter
       '''
       req = apc_pb2.AP_APIReq()
       req.cmdId = Commands['setParameter']
       if args[0] in APParameters:
          if 'set' in APParameters[args[0]][1]:
             req.payload = gen_payload(APParameters[args[0]][0], args[1])
             if len(req.payload) == 0:
                return None
          else:
             print "Unsupported operation"
       else:
          print "Unrecognized parameter"
          return None;

       apAPI_resp = self.send_msg(
                        apc_pb2.AP_API, 
                        req,
                        apc_pb2.AP_APIResp,
                        service=APC_RPC_SERVICE)
       return None

    def rpcAPC_getParameter(self, args):
       ''' APC getParameter
       '''
       if args not in APCParameters:
          print "Unrecognized parameter"
          return None;

       apcInfo_resp = self.send_msg(
                            apc_pb2.GET_APC_INFO, 
                            "",
                            apc_pb2.APCInfoResp,
                            service=APC_RPC_SERVICE)
       dict = pb2dict(apcInfo_resp)

       for key in dict:
          if key.lower() == args:
             if args == 'gpsstate':
                print Enum_gps_status_t.to_string(dict[key])
             else:
                print str(dict[key])


    def getStats(self, args):
       ''' get statistics for connection of APC/AP or APC/Manager
       '''
       apcStats_resp = self.send_msg(
                            apc_pb2.GET_APC_STATS, 
                            "",
                            apc_pb2.APCStatsResp,
                            service=APC_RPC_SERVICE)

       for f, v in apcStats_resp.ListFields():
          for names in StatsDict.keys():
             if f.name in StatsDict[names]:
                if type(v) is float:
                   StatsDict[names][f.name][2] = "{:.3f}".format(v)
                else:
                   StatsDict[names][f.name][2] = "{:,}".format(v)

       for names in StatsDict.keys():
          # partial match for dictionary key
          if names.lower().find(args) == 0 or args == 'all':
             out  = [names]
             out += ['----------------------------------------------']
             dict = OrderedDict(sorted(StatsDict[names].items(), key=lambda t: t[1][0]))
             for key in dict:
                out += ['{0: <24}'.format(dict[key][1]) + '{0: >22}'.format(dict[key][2])]
             print '\n'.join(out)

             # print delay stats
             delayStats = rpcRespToDict(apcStats_resp)
             if 'Manager' in names:
                print "TX delays:  ", statDelaysToString(delayStats['toMngr'])
             print            
    
    def clearStats(self):
       ''' reset statistics for connection of APC/AP or APC/Manager
       '''
       self.send_msg(apc_pb2.CLEAR_APC_STATS, 
                            "",
                            "",
                            service=APC_RPC_SERVICE)

