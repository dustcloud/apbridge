#!/usr/bin/env python

import sys
import os
import subprocess

def usage(name) :
   print 'usage:\n\genEnumToStringDependence {<srcFile> | <scrRootDir>} [<dstDirCpp> <dstDirPython>]'

def processOneFile(src, path, file, dstCpp, dstPython) :
   subprocess.call(['protoc', os.path.join(path, file), '--proto_path='+src+ os.pathsep +path, '--cpp_out='+dstCpp, '--python_out='+dstPython])
   
def main(src, dstCpp, dstPython) :
   depend = []
   if dstCpp and not os.path.exists(dstCpp):
      os.makedirs(dstCpp)
   if dstPython and not os.path.exists(dstPython):
      os.makedirs(dstPython)
   if os.path.isdir(src):
      for dirpath, subdirs, files in os.walk(src):
         for currentFile in [f for f in files if f.endswith(".proto") ]:
            processOneFile(src, dirpath, currentFile, dstCpp, dstPython)
   else :
      processOneFile(src, src, dstCpp, dstPython)
   
if __name__=='__main__':
   # Main procedure
   if len(sys.argv) < 2 or len(sys.argv) > 4:
      usage()
      sys.exit(1)
   (prg, src, dstCpp, dstPython) = (sys.argv + ['.', '.'])[:4]
   main(src, dstCpp, dstPython)

