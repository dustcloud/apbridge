'''Python log console'''

import datetime
import logging
import sys
import struct
import zmq

log = logging.getLogger('voyager-py.log_console')
log.setLevel(logging.INFO)
log.addHandler(logging.NullHandler())

from listener import Listener
from pyvoyager.proto.logging import logevent_pb2

class LogConsole(Listener):
    '''Implements a logger listener

    - receives log notifications
    '''

    DEFAULT_MSG_FORMAT = "{0} {1} {2}: {3} {4}"  

    def __init__(self, ctx, logger_name, logger_addr, rpcClient, 
                 loglevel_filter = logevent_pb2.L_FATAL):
        Listener.__init__(self, ctx, logger_name, logger_addr)
        self.trace_callback = self.printTraceNotif
        self.format_string = self.DEFAULT_MSG_FORMAT
        self.rpcClient = rpcClient
        self.loggers = []
        self.loglevel_filter = loglevel_filter
        
    def logNotifFormat(self, msg):
        'Format a log notification'
        dt = datetime.datetime.fromtimestamp(msg.timestamp/1000000.0)
        time_str = '{0:%H:%M:%S}.{1:03}'.format(dt.time(), dt.time().microsecond/1000)
        level_str = logevent_pb2.LogLevel.Name(msg.logLevel)
        return self.format_string.format(str(dt.date()), time_str,
                                         msg.logger, level_str, msg.msg)

    def getSubscribedNotifs(self):
        '''Returns: list of subscribed notifications for this listener
        '''
        return self.filters
    
    def getNotifTypes(self):
        '''Returns: list of notification names handled by this listener
        '''
        if not self.loggers:
            self.loadNotifTypes()
        return self.loggers
        
    def lookupNotifId(self, notifName):
        '''lookupNotifId is used to get the id from a notification name

        Returns: id of notification with notifName
        '''
        return notifName

    def loadNotifTypes(self):
       '''Get all loggers
       '''
       #print "loading loggers"
       try :
           resp = self.rpcClient.send_secure_msg(logevent_pb2.GET_LOGGERS, 
                                        '',
                                        logevent_pb2.GetLoggersResponse, 
                                        service='logger')
           self.loggers = list(resp.loggers)
       except Exception as exc:
           print exc
       
    
    def printTraceNotif(self, msg, notifyType=''):
        'Default log notification callback'
        
        traceMsg = self.logNotifFormat(msg)
        # Save massage into the file, if required (function check it)
        self.saveTraceMessage(traceMsg)
        print traceMsg

    def printNotif(self, msgId, log_msg, notifyType=''):
        'Print notification'
        msgS = self.logNotifFormat(log_msg["msg"])
        print "{0} - {1}".format(msgId+1, msgS)
             
    def registerCallback(self, cb):
        'Register a callback function for when a log notification is received'
        self.log_callback = cb
        
    def deserialize_notifid(self, input_msg):
        '''Deserialize the notif id
        For the Log Listener, the notif id is a string'''        
        return input_msg[0]

    def processNotif(self, notifId, notification):
        '''Process and deserialize an incoming log notification
        
        Keyword arguments:
        notifId      - Notification Id
        notification - Notification data
        '''

        log.debug('Received log event from %s, len=%d', 
                  notifId, 
                  len(notification))
        # deserialize 
        log_event = logevent_pb2.LogEvent.FromString(notification)
        # Check the filter, allow error messages if the log level is severe
        # even if it is not subscribed
        log_severity = (log_event.logLevel <= self.loglevel_filter)
        if not ((notifId in self.filters) or log_severity):
            return

        log_notif = {'msg': log_event}
        if notifId not in self.INBOX:
            self.INBOX[notifId] = []
        self.INBOX[notifId].append(log_notif)
        
        # Check if trace is active and the notification type is in the 
        # trace filter list. Always show severe messages
        if (notifId in self.trace_filter) or log_severity:
            self.trace_callback(log_event)

# Standalone application
            
LOG_FILENAME = 'rpc_client.log'
LOG_FORMAT = "%(asctime)s [%(name)s:%(levelname)s] %(message)s"

def setup_logging():
    # Add the log message handler to the logger
    file_handler = logging.handlers.RotatingFileHandler(LOG_FILENAME,
                                                        maxBytes=2000000,
                                                        backupCount=5,
                                                        )
    file_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    log.addHandler(file_handler)
    # Add some basic console output
    console_handler = logging.StreamHandler()
    log.addHandler(console_handler)

    for logger_name in ['voyager-py.console']:
        logger = logging.getLogger(logger_name)
        logger.addHandler(file_handler)
        logger.addHandler(console_handler)


def print_msg(log_event):
    print str(log_event)

def main():
    setup_logging()
    ctx = zmq.Context()
    log_addr = sys.argv[1]
    console = LogConsole(ctx, 'logger', log_addr)
    console.registerCallback(print_msg)
    console.run()

    
if __name__ == '__main__':
    main()
