'''Python listener class'''

import copy
import os
import struct
import threading
import zmq

try:
    from queue import Queue
except:
    from Queue import Queue

from pyvoyager.CLICommon import printError
from pyvoyager.CLICommon import ERROR_DICT

def processNotifMsg(notifMsg, pb_class):
    '''Parse a serialized notification string into a dictionary of fields'''
    pb_msg = pb_class.FromString(notifMsg)
    notif_params = {f.name: v for f, v in pb_msg.ListFields()}
    # TODO: allow dict conversion of internal repeated fields
    return notif_params


class Listener(object):
    '''Implements a listener base class

    - receives notifications
    '''
    DEFAULT_POLL_TIMEOUT = 1000 # milliseconds
    NUM_LINES_X_PAGE = 25
    INBOX_SIZE = 100

    class QueueEnd(object) :
        pass
                          
    def __init__(self, ctx, listener_Name, publisher_addr, inpq_max_size = 1000, inpq_threshold = 100):
        self.context = ctx
        self.publisher_addr = publisher_addr
        self.listener_name = listener_Name
        self.poll_timeout = self.DEFAULT_POLL_TIMEOUT
        self.listener = None
        # the filter attributes contain subscribed notif ids 
        self.trace_filter = set()
        self.filters = set()
        self.fRun = False
        self.logFileName = ''
        self.fSave = False
        # Queue of input messages
        self.inpQueue = Queue()
        # Store all notifications published
        self.INBOX = {}
        # counter that keeps track of base MsgId while displaying index, once INBOX is full
        self.base_msgId = 0
        self.inpq_max_size  = inpq_max_size
        self.inpq_threshold = inpq_threshold
           
    def _connect(self):
        'Subscribe the listener to a specific publisher'
        if not self.listener:
            self.listener = self.context.socket(zmq.SUB)
            self.listener.subscribe = ''
            #TODO use subscribe as a filters 
            self.listener.connect(self.publisher_addr) 

    def setInpQlimits(self, inpq_max_size, inpq_threshold):
        try :
            inpq_max_size = int(inpq_max_size)
            self.inpq_max_size  = inpq_max_size
        except :
            pass
        try :
            inpq_threshold = int(inpq_threshold)
            self.inpq_threshold = inpq_threshold
        except :
            pass
    
    def isRunning(self):
        '''Returns: True if listener is running, False otherwise
        '''
        return self.fRun

    def getNotifTypes(self):
        '''Returns: list of notification names handled by this listener
        '''
        raise NotImplementedError("Child must implement getNotifTypes")

    def getSubscribedNotifs(self):
        '''Returns: list of subscribed notifications for this listener
        '''
        raise NotImplementedError("Child must implement getSubscribedNotifs")

    def lookupNotifId(self, notifName):
        '''lookupNotifId is used to map a human-readable notification name to the
        corresponding id.

        Returns: id of notification with notifName
        '''
        raise NotImplementedError("Child must implement lookupNotifId")        

    def enableTrace(self, traceFilter):
        '''Add a notifcation to the trace filter list

        Keyword arguments:
        traceFilter - Trace filter name
        '''
        traceId = self.lookupNotifId(traceFilter)
        if not (traceId in self.trace_filter):
            self.trace_filter.add(traceId)
        
    def enableTraceAll(self):
        '''Add all notifcation to the trace filter list
        '''
        traceFilterList = self.getNotifTypes()
        for traceFilter in traceFilterList:
           traceId = self.lookupNotifId(traceFilter)
           if not (traceId in self.trace_filter):
               self.trace_filter.add(traceId)

    def disableTrace(self, traceFilter=''):
        '''Remove a filter from the trace filter list. If no filter is provided, all
        filters will be removed

        Keyword arguments:
        traceFilter - Trace filter name
        '''
        try:
            if traceFilter:
                traceId = self.lookupNotifId(traceFilter)
                self.trace_filter.remove(traceId)
            else:
                self.trace_filter = set()
        except KeyError:
            pass

    def saveTraceMessage(self, traceMsg):
        '''Save trace message into a specific file

        Keyword arguments:
        traceMsg - Trace message to save
        '''
        # Check if the message should be saved
        if not self.fSave:
           return

        # Open/create file
        try:
           traceFile = open(self.logFileName, 'a+')
        except IOError as exc:
           printError(obv='FILE_NOT_FOUND ' + str(exc))
           return

        # Save message
        traceFile.write(traceMsg)

        # Close file      
        traceFile.close()

    def registerNotifType(self, notifName):
        '''Register new notification type to process. The notification id is added
        to the list of subscribed notifications.
        
        Keyword arguments:
        notifName - Notification name to register
        '''
        notifId = self.lookupNotifId(notifName)
        if not notifId in self.filters:
            self.filters.add(notifId)
        if not notifId in self.INBOX :
            self.INBOX[notifId] = []

    def unregisterNotifType(self, notifName):
        '''Unregister notification type from the list of subscribed notifications. 

        Keyword arguments:
        notifName - Notification name to unregister
        '''
        notifId = self.lookupNotifId(notifName)
        if notifId in self.filters:
            self.filters.remove(notifId)
            del self.INBOX[notifId]
        else:
            raise KeyError("Notification not registered: " + notifName)
        
    def isNotifTypeRegistered(self, notifName):
        '''Check if the notification type is in the filter list. This method may be
        called with a notifName that is not handled by this listener.
        
        Keyword arguments:
        notifName - Notification name to check
        '''
        try:
            notifId = self.lookupNotifId(notifName)
            return (notifId in self.filters)
        except KeyError:
            return False

    def isFilterListEmpty(self):
        '''Returns: True if filter list is empty, False otherwise
        '''
        if not self.filters:
            return True
        return False

    def stop(self):
        '''Stop main loop. 
        Delete old msgs
        '''
        # TODO: there's a race condition here where the main loop can insert 
        # messages before it exits, but after the INBOX has been cleared
        self.INBOX = {} 
        self.fRun = False 

    def close(self):
        self.stop()
        if self.listener:
           self.listener.close()

    def getName(self):
        'Get listener name'
        return self.listener_name

    def parseArgs(self, args, notifType):
       '''Parse arguments for printing list of stored notifications

       Keyword arguments:
       args - Input argument list
       notifType - Notification type

       String to parse: ALL/Last/Page/Range NumberLines/PageNum/N M
       '''

       
       if args[0].upper() == "ALL":
           if len(args) != 1:
             raise ValueError("Invalid args")

           return [0, len(self.INBOX[notifType]),args[0].upper()]
       elif args[0].upper() == "LAST":
         #Check number of arguments
         if len(args) != 2:
             raise ValueError("Invalid args")

         try:
             numLine = int(args[1])
             if numLine <= len(self.INBOX[notifType]):
                 return [len(self.INBOX[notifType])-numLine,
                         len(self.INBOX[notifType]),args[0].upper()]
             else:
                 return [0, len(self.INBOX[notifType]),args[0].upper()]
         except:
             raise IndexError(ERROR_DICT['INVALID_CL_ARGS'])

       elif args[0].upper() == "PAGE":
           #Check number of arguments
           if len(args) != 2:
               raise ValueError("Invalid args")

           try:
               numLine = int(args[1])
           except ValueError:
               raise

           if numLine <= 0:
               raise IndexError("Invalid index")

           firstLine = (numLine-1) * self.NUM_LINES_X_PAGE
           lastLine = numLine * self.NUM_LINES_X_PAGE

           if lastLine > len(self.INBOX[notifType]):
               lastLine = len(self.INBOX[notifType])

           return [firstLine,lastLine,args[0].upper()]

       elif args[0].upper() == "RANGE":
           #Check number of arguments
           if len(args) != 3:
               raise ValueError("Invalid args")

           try:
              firstLine = int(args[1])
              lastLine =int(args[2])
              return [firstLine-1, lastLine,args[0].upper()]
           except ValueError:
               raise
       else:
           raise ValueError("Invalid args")

    def listMessages(self, args, notifyType, fPrint=True):
       '''List messages in the INBOX

       Keyword arguments:
       args - How many notifications to show
       notifType - Notificaction type to list

       Example:  Last 10
       '''
       
       try:
           notifyType = self.lookupNotifId(notifyType)
           
           if not self.INBOX[notifyType]:
               if fPrint:
                  print "No messages"
                  return (0, [])
           
           rangeValues = self.parseArgs(args, notifyType)
       except IndexError as exc:
           printError(obv=str(exc))
           return (1, [])
       except ValueError:
           printError(obv='INVALID_CL_ARGS')
           return (2, [])
       except KeyError:
           printError(obv=str(exc))
           return (3, [])

       firstLine = rangeValues[0]
       lastLine = rangeValues[1]
       rangeIndex = firstLine
       outputMsgs = []
      
       for i in range(firstLine, lastLine):
           queueIndex = i
           if fPrint:
              if rangeValues[2] != "RANGE":
                 self.printNotif(queueIndex+self.base_msgId, self.INBOX[notifyType][queueIndex], notifyType)
              else:
                 queueIndex = (rangeIndex - self.base_msgId)
                 rangeIndex = rangeIndex + 1
                 if queueIndex < 0 : continue
                 if queueIndex >=  len(self.INBOX[notifyType]) : break
                 self.printNotif(i, self.INBOX[notifyType][queueIndex], notifyType)
           outputMsgs.append(self.INBOX[notifyType][queueIndex])
       
       return (0, outputMsgs)

    def printNotif(self, msgId, notif, notifyType):
        '''The printNotif method prints a string containing a human-readable 
        notification.
        '''
        print "msgId:{0} :: notif:{1} :: notifType::{2}".format( msgId, notif, notifyType)
        # TODO: this should return a string and let the console manage printing
        raise NotImplementedError("Child must implement printNotif")

    def processNotif(self, notifId, notification):
        '''Process any of the notifications received by the listener.
        
        The listener should parse the notification based on its id and store
        the notification object (dict) in the INBOX under the notification id.
        '''
        raise NotImplementedError('child must implement processNotif')

    def limitQueueError(self, qsize):
        if len(self.trace_filter) or len(self.filters):
            try:
                printError(obv='Listener queue is full. Unsubscribed from all (inbox is saved): {0}'.format(' '.join(self.getSubscribedNotifs())))
            except:
                printError(obv='Listener queue is full. Unsubscribed from all (inbox is saved)')
            self.trace_filter = set()
            self.filters = set()
        
    def fatalQueueError(self, qsize):
         printError(obv='\nFATAL ERROR. Listener queue overflowed.')
         os._exit(1)
         
    def fatalZMQError(self, ex):
         printError(obv='FATAL ZMQ ERROR: {0}'.format(ex))
         
    def deserialize_notifid(self, input_msg):
        '''Deserialize the notif id
        For most Listeners, the notif id is a 32-bit integer'''
        # this method exists so the LogListener can override it
        (notif_id, ) = struct.unpack("!L", input_msg[0])
        return notif_id

    def handleNotif(self, input_msg):
        '''Perform basic notification parsing, purge inbox, call processNotif.
        \param input_msg - Input message string
        '''
        # separate the notification id from the notification structure
        # TODO: it might be better to use an instance flag to indicate whether 
        # we should expect log message notifications
        notif_id = 0
        try:
            notif_id = self.deserialize_notifid(input_msg)
            # Purge INBOX of this notification type
            self.purgeInbox(notif_id)            
            # Listener-specific notification processing
            self.processNotif(notif_id, input_msg[1])
        except Exception as ex:
            MSG_TMPL = 'Exception in {0} processNotif: notifId={1} msg={2}\n{3}'
            msg = MSG_TMPL.format(type(self), notif_id, input_msg, ex)
            printError(obv=msg)

    def purgeInbox(self, notifId):
        '''Purge inbox for the specified notification type
        \param notifId - Notification Id
        '''
        if not notifId in self.INBOX:
            return
            
        if len(self.INBOX[notifId]) == self.INBOX_SIZE:
            # Purge INBOX
            self.INBOX[notifId].pop(0)
            self.base_msgId = self.base_msgId + 1
    
    # Main loop for the listener
    def run(self):
        # Connect to socket
        self._connect()
        
        self.fRun = True
        # Start queue reader
        t = threading.Thread(target=self.queueReader)
        t.start()
        
        # create the poll object
        poller = zmq.Poller()
        poller.register(self.listener, zmq.POLLIN | zmq.POLLERR)
        
        while self.fRun:
            try:
            
               active = dict(poller.poll())
               if (self.listener in active and
                   (active[self.listener] & zmq.POLLIN )== zmq.POLLIN and self.fRun):
                   input_msg = copy.deepcopy(self.listener.recv_multipart())
                   
                   if self.inpQueue.qsize() >= self.inpq_max_size :
                     self.fatalQueueError(self.inpQueue.qsize())
                   else :
                     self.inpQueue.put(input_msg)
                     if self.inpQueue.qsize() >= self.inpq_threshold :
                        self.limitQueueError(self.inpQueue.qsize())
                        
            except zmq.ZMQError as ex:
               if self.fRun :
                  self.fatalZMQError(ex)
        
        self.inpQueue.put(self.QueueEnd())
        self.inpQueue.join()
        t.join()
        
    def queueReader(self) :
        while True :
           input_msg = self.inpQueue.get()
           if type(input_msg) is self.QueueEnd :
               self.inpQueue.task_done()               
               break
           if self.fRun:
              self.handleNotif(input_msg)
           self.inpQueue.task_done()   

class ListenerCollection:
   ''' Provide a data structure for storing listeners and finding listeners based on their notifications and parent process names '''
   
   class ListenerType:
      LISTENER = 1,
      LOGLISTENER = 2
   
   def __init__(self):
      self.listenerDict = dict()
      
   def addListeners(self, process_name, listeners):
      ''' Add listener to the listenerDict
          process_name - name of process
          listeners - list of tuple. Tuple consist of listener name, listener and listener type
      '''
      if process_name not in self.listenerDict:
         self.listenerDict[process_name] = []
      self.listenerDict[process_name] += listeners
      
   def getListenerByNotifType(self, notifType, process_name = None):
      ''' Find listener which provides the notification type

      Keyword arguments:
      notifType - Notification type
      process_name - process name like configdb, manager...
      Returns: listener
      '''
      if not process_name:
         for listenerList in self.listenerDict.values():
            listener = self.getListener(notifType, listenerList)
            if listener:
               return listener

      else:
         listenerList = self.listenerDict[process_name]
         listener = self.getListener(notifType, listenerList)
         if listener:
            return listener
      return None

   def getListener(self, notifType, listenerList):
      ''' Find listener name from notification type
      
      Keyword arguments:
      notifType - notification type
      listenerList - list of listener
      Returns: listener
      '''
      for listener, listener_type in listenerList:
         if notifType in listener.getNotifTypes():
            return listener
      
   def getListenerType(self, listener):
      ''' Find listener of give process and type

      Keyword arguments:
      listener - object of listener
      Returns: listener type
      '''
      for listenerList in self.listenerDict.values():
         for listenerObj, type in listenerList:
            if listener is listenerObj:
               return type
      return None
      
   def getAllListeners(self):
      ''' Get list of all listeners
      
      Returns: list of listener
      '''
      listeners = [listener for listenerList in self.listenerDict.values() for listener, type in listenerList]
      return listeners
      
   def getListenerByProcess(self, process_name, listener_type=None):
      ''' Get listener by process name and type
      process_name - process name
      listener_type - type of listener
      Returns: list of listener
      '''
      listenerList = self.listenerDict[process_name]
      if listener_type:
         listeners = [listener for listener, type in listenerList if type == listener_type]
      else:
         listeners = [listener for listener, type in listenerList]
      return listeners
      
