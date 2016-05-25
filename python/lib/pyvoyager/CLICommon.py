'''
Common defines
'''
import sys

from pyvoyager.enum.RpcCommon_enum import Enum_RpcResult
from pyvoyager.enum.RpcCommon_enum import Enum_RpcEndpoints
from pyvoyager.enum.RpcCommon_enum import Enum_RpcServices

#===================== Common Defines ================================

ERROR_DICT = {'INVALID_CL_ARGS': "Invalid command line argument",
    'INVALID_NUM_PARAMETERS' : "Invalid number of input arguments",
    'INVALID_MAC': "Invalid MAC Address",
    'INVALID_NBRS_INFO': "Invalid neighbors information",
    'INVALID_USERNAME': "Invalid username",
    'CMD_NOT_IMPLEMENTED': "Sorry, command not implemented yet",
    'EMPTY_DB': "Tplg DB empty",
    'INVALID_LN': "Invalid listener name",
    'USER_INTER': "Command not supported outside the console.\nThe command needs user interaction.",
    'ROUTE_NOT_FOUND': "Route not found",
    'MOTE_NOT_FOUND': "Mote not found",
    'AP_NOT_FOUND': "Ap not found",
    'NOT_IN_SUMODE': "You must be in super user mode to use this command. Try \'su <password>\' first.",
    'JOINKEY_MISSING': "Joinkey missing.",
    'NO_SERVICE_ASSIGNED': "No services assigned",
    }

MAX_MOTEID = 9999

PROCESSLIST = ['configdb', 'authmanager', 'manager']

NOTIF_CATEGORY_DICT = {
    'DATA':   ["dataPacketReceived", "ipPacketReceived"],
    'EVENTS': ["alarmOpened", "alarmClosed", "cmdFinished", "invalidMIC", "joinFailed", "moteStateChanged", "pathStateChanged", "pathAlert", "serviceChanged", "packetSent", "pingResponse", "optPhase", "managerStopping", "managerStarted"],
    'HR':     ["deviceHealthReport", "discoveryHealthReport", "neighborHealthReport", "rawMoteNotification"],
    'CONFIG': ["configChanged", "configDeleted", "configLoaded", "configRestored"],
}


USERNAME_REGEXP = '^[a-z_][a-z0-9_-]{0,31}$'

#===================== Common functions ==============================

def printRPCError(RpcStrCode, obv=""):
    '''Print error RPC messages from error RpcCommon enumeration
   
    \param RpcStrCode RPC string error code
    '''

    try: 
        # Print string comment
        rpcCode = Enum_RpcResult.to_int(RpcStrCode)
        errmsg = Enum_RpcResult.to_string(rpcCode, True)
    except KeyError:
        raise KeyError("RPC code not in the Enum_RpcResult dictionary")

    if obv:
        errmsg = errmsg + ": {0} ".format(obv)
        
    sys.stdout.write("Error: "+ errmsg + "\n")
    sys.stdout.flush()

def printError(keyWord="Error", obv=""):
    '''Print error message from error dictionary
   
    \param keyWord Key word in the error dictionary
    \param obv     Attach observations in to the message
    '''
    
    try: 
        errmsg = "Error: " + ERROR_DICT[keyWord];
    except KeyError:
        errmsg = "Error"

    if obv:
        if obv == 'RPC Error 35: ':
            errmsg = errmsg + ": unauthorized command. The current user does not have the privileges to perform this operation. "
        else:
            errmsg = errmsg + ": {0} ".format(obv)

    if keyWord == 'INVALID_CL_ARGS':
        errmsg = errmsg + "\nTry help <command> [<subcommand>] for more information."
    sys.stdout.write(errmsg + "\n")
    sys.stdout.flush()

def printInfo(msg):
    '''Print info message
    '''
    sys.stdout.write(msg + "\n")
    sys.stdout.flush()

def getRPCEndPointPath(endpointDefName):
    ''' Get endpoint path from Enum_RpcEndpoints enumeration

    Keyword arguments:
     endpointDefName - Endpoint definition name

    Returns: Endpoint path
    '''
    ep_code = Enum_RpcEndpoints.to_int(endpointDefName)
    ep_str = Enum_RpcEndpoints.to_string(ep_code, True)
    ep_str = ep_str.replace("//", "//../")
    return ep_str

def getRPCEndpoint(endpointName, path_offset = '..', 
                   host = '', port_base = 9000):
    '''Get endpoint from Enum_RpcEndpoints enumeration

    If the host argument is not empty, return the tcp endpoint 
    composed of the host and port.
    If the host is empty, return the path adjusted with the path_offset.

    Keyword arguments:
     endpointName - Endpoint definition name

    Returns: Endpoint path
    '''
    endpt_code = Enum_RpcEndpoints.to_int(endpointName)
    if host:
        # the port is the enum value relative to the port base
        port = port_base + endpt_code
        endpt_str = 'tcp://{0}:{1}'.format(host, port)
    else:
        endpt_str = Enum_RpcEndpoints.to_string(endpt_code, True)
        if path_offset:
            endpt_str = endpt_str.replace("//", "//{0}/".format(path_offset))
    return endpt_str
    
def getRPCServiceName(serviceDefName):
    ''' Get service from Enum_RpcServices enumeration

    Keyword arguments:
     serviceDefName - Services definition name

    Returns: Service name
    '''
    
    srv_code = Enum_RpcServices.to_int(serviceDefName)
    return Enum_RpcServices.to_string(srv_code, True)

def isValidMoteId( moteId ):
    ''' Check if moteId is valid. value should be between 1 and MAX_MOTEID
    '''
    
    return (1 <= moteId <= MAX_MOTEID)

def fillObjectFromDict( ob, dct ):
    ''' Fill the object from dictionary
    Keyword arguments:
     ob - Object to be filled
     dct - input dictionary
    '''
    for key, value in dct.items():
        # dct can contain keys that are part of the command input, but not part of ob.
        if ( key in dir(ob) ) and (value != None):
            setattr(ob, key, value)

