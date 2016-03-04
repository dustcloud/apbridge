'''Python RPC client interface'''

import struct
import zmq
import collections
import logging

log = logging.getLogger('voyager-py.baserpcclient')
log.addHandler(logging.NullHandler())

CLIENT_PROTOCOL = 'RPCC01'

TIMEOUT = 10000 # in milliseconds

class RpcError(Exception):
    '''RpcError exception for RPC errors

    The result_code attribute contains the numeric result code. 
    The msg attribute contains the error message. 
    '''
    def __init__(self, rc, msg=''):
        self.result_code = rc
        self.msg = msg

    def __str__(self):
        return 'RPC Error {0}: {1}'.format(self.result_code, self.msg)

class TimeoutError(Exception):
    '''TimeoutError exception indicates the RPC was terminated by a timeout

    The result_code attribute contains the numeric result code. 
    The msg attribute contains the error message. 
    '''
    def __init__(self, rc, msg='ZMQ timeout error'):
        self.result_code = rc
        self.msg = msg

    def __str__(self):
        return self.msg

class BaseRpcClient(object):
    '''Implements the RPC Client protocol

    The Client connects to a RPC Server and can perform synchronous RPCs.
    '''

    # Define ZMQ server parameters
    ServerParams = collections.namedtuple('Server', 'server_addr service identity')

    def __init__(self, ctx, server_addr, service, identity=''):
        '''Initialize the RpcClient

        ctx - ZMQ context
        server_addr - ZMQ address of the server endpoint
        service - Default service name
        identity - Client identity string. This string must be unique in order for the server to route the response to the correct client. 
        '''
        self.context = ctx
        self.sessionId = None
        
        self.serverParams = BaseRpcClient.ServerParams(server_addr, service, identity)
        self.server_poller = zmq.Poller()
        self.connect()

    def getDefaultServiceName(self) :
        return self.serverParams.service
        
    def connect(self):
        self.server = self.connect_p(self.serverParams)

    def close(self):
        self.close_p(self.serverParams, self.server)
        
    def send(self, cmd, param_str="", service=None, authN=None, server=None) :
        '''Send an RPC request to the server and return the response
        
        cmd - command identifier (as integer)
        param_str - serialized parameters
        authN     - Command authorization structure
        service   - (Optional) service name to handle the command
        server    - Server

        Returns: list of (cmd, result code, serialized response structure)
        '''
        if not server :
           server = self.server
           
        if not service:
            service = self.serverParams.service
            
        log.debug('Sending request to service=%s, cmd=%d', service, cmd)

        cmd_msg = struct.pack('B', cmd)
        rpcParts = [CLIENT_PROTOCOL, service, cmd_msg, param_str]

        if authN:
            rpcParts.append(authN)

        server.send_multipart(rpcParts, flags=zmq.NOBLOCK)

        # wait for a response
        newMsg = self.server_poller.poll(TIMEOUT)
        if newMsg:
            msg_parts = server.recv_multipart()
        else:
            log.error('ZMQ timeout error, reconnecting')
            self.close()
            self.connect()
            raise TimeoutError(1)

        # extract and validate the RPC client wrapper
        if len(msg_parts) < 3:
            err = 'Bad response length: {0}'.format(len(msg_parts))
            log.error(err)
            raise RuntimeError(err)
        if msg_parts[0] != CLIENT_PROTOCOL:
            err = 'Bad client protocol: {0}'.format(msg_parts[0])
            log.error(err)
            raise RuntimeError(err)
        if msg_parts[1] != service:
            err = "Bad service: expected '{0}', got '{1}'".format(service, msg_parts[1])
            log.error(err)
            raise RuntimeError(err)
        # reply should consist of:
        # - cmd id
        # - result code
        # - serialized protobuf reply structure
        msg_parts[2] = ord(msg_parts[2])
        log.debug('Received response for service=%s cmd=%d', msg_parts[1], msg_parts[2])
        return msg_parts[2:]

    def send_msg(self, cmd, pb_req, pb_resp_class, service=None, server=None):
        '''Send an RPC to the server and handle common serialization operations

        cmd - command identifier (as integer)
        pb_req - Protocol buffer request message
        pb_resp_class - Protocol buffer response message class
        service - (Optional) service name to handle the command
        server  - (Optional) Server to send the message to (ZMQ socket)

        Returns: Deserialized protocol buffer message or raises RpcError
        '''
        rpc_params = ''
        if pb_req:
            rpc_params = pb_req.SerializeToString()

        rpc_resp = self.send(cmd, rpc_params, service, server=server)
        # parse the response
        # assert(rpc_resp[0] == cmd)
        result_code = struct.unpack('!L', rpc_resp[1])[0]
        if result_code == 0:
            if pb_resp_class:
                resp = pb_resp_class.FromString(rpc_resp[2])
            else:
                resp = rpc_resp[2]
            return resp
        
        else:
            # raise RpcError on errors
            err = rpc_resp[2] if len(rpc_resp) > 2 else ''
            raise RpcError(result_code, err)

    def connect_p(self, serverParams) :
        server = self.context.socket(zmq.REQ)
        if serverParams.identity:
            server.identity = serverParams.identity
        server.connect(serverParams.server_addr)
        log.info('RPC client connected to {0}'.format(serverParams.server_addr))
        self.server_poller.register(server, zmq.POLLIN)
        return server 
    
    def close_p(self, serverParams, server):
        server_addr = serverParams.server_addr
        server.disconnect(server_addr)
        log.info('Disconnect from {0}'.format(server_addr))
        server.close()
