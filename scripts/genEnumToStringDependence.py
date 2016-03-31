#!/usr/bin/env python

import sys
import re
import os
import glob

def usage() :
   print '''usage: genEnumToStringDependence {<srcFile> | <srcRootDir>} [<dstDir>] [basePrjDir]

Generate the enumToString dependencies for a file or files in a directory

'''
   
def parseComment(l) :
   m = re.match('(.*)//.*"(.+)".*', l)
   if m :
      return (m.group(1).strip(), m.group(2).strip())
   return (l.strip(), None)

def processOneFile(inpFileName, outPath, base_dir, depend) :
   finp = open(inpFileName, "r")
   enumName = ''
   
   for line in finp:
      (enum_symbol, enum_string) = parseComment(line)
      if enum_symbol :
         # split the string into words to avoid getting confused by similar
         # enum names
         m = re.match('\s*ENUM2STR\(([\w:]+)\).*', enum_symbol)
         if m:
            enumName = m.group(1)
   if enumName :
      outFileName = os.path.splitext(os.path.basename(inpFileName))[0] + "_enum.cpp"
      if (outPath) :
         outFileName = os.path.join(outPath, outFileName)   
      if (base_dir) :
         include_header = os.path.relpath(inpFileName, base_dir)
      else :   
         include_header = os.path.basename(inpFileName)
      
      depend.append([outFileName, include_header])

   finp.close()

def printDepend(depend) :
   for d in depend :
      print d[0], ':', d[1]
   
def main(src, dst, base_dir) :
   depend = []
   if os.path.isdir(src):
      for dirpath, subdirs, files in os.walk(src):
         for currentFile in [f for f in files if f.endswith(".h") ]:
            processOneFile(os.path.join(dirpath, currentFile), dst, base_dir, depend)
   else :
      processOneFile(src, dst, base_dir, depend)
   printDepend(depend)
   
if __name__=='__main__':
   # Main procedure
   if len(sys.argv) < 2 or len(sys.argv) > 4:
      usage()
      sys.exit(1)
   (prg, src, dst, base_dir) = (sys.argv + [None, None, None, None])[:4]
   main(src, dst, base_dir)
   
