import os
import sys
import glob
import re
from argparse import ArgumentParser

'''
        dnToStr <src-dir> <dst-file>
    
    Generate file <dst-file> with function for translate value of DN_API_xx 
    constants to symbolic name according definition in
    <src-dir>/dn_*.h files
    
    Structure of dn_to_str.py file:
    # Dictionary for DN_API_ST_ constants
    dnconst_MoteSt = {
        int-val : symbolic-name,
    }
    # Value of DN_API_ST_ constant to string
    def dnToStrMoteSt (val) :
    
    # Dictionary for DN_API_AP_CLK_ constants
    dnconst_ClkSrc = {
        int-val : symbolic-name,
    }
    # Value of DN_API_AP_CLK_ constant to string
    def dnToStrClkSrc (val) :
'''

FUN_PREFIX     = 'dnToStr'
DICT_PREFIX    = 'dnconst_'

TMPL_CONST = '''
# Dictionary for {DN_PREFIX} constants
{DICT_PREFIX}{POSTFIX} = {{
{BODY}
}}

def {FUN_PREFIX}{POSTFIX} (val) :
    \'\'\'
    Convert integer value of {DN_PREFIX} constant to string
    \'\'\'
    val = int(val)
    if val in {DICT_PREFIX}{POSTFIX} :
        return {DICT_PREFIX}{POSTFIX}[val]
    else :
        return str(val)
'''

PREFIX_POSTFIX = [
    ('DN_API_ST_',     'MoteSt'), 
    ('DN_API_AP_CLK_', 'ClkSrc'),
]

class TranslateDN2Str(object) :
    def __init__(self) :
        self.trTabl = { p[0] : {} for p in PREFIX_POSTFIX}
        
    def processFile(self, srcFile) :
        with open(srcFile, 'rt') as fileIn :
            for line in fileIn :
                line = line.strip()
                if line.startswith('#define') :
                    words = line.split()
                    for pp in PREFIX_POSTFIX :
                        if words[1].startswith(pp[0]) :
                            self.trTabl[pp[0]][int(words[2])] = words[1]
                            break

    def generate(self, fileName) :
        with open(fileName, 'wt') as fileOut :
            for pp in PREFIX_POSTFIX :
                d = self.trTabl[pp[0]]
                b = ',\n'.join(['   {} : "{}"'.format(v, d[v]) for v in d])
                f = {'DICT_PREFIX' : DICT_PREFIX, 'DN_PREFIX' : pp[0], 'POSTFIX' : pp[1], 
                     'FUN_PREFIX' : FUN_PREFIX, 'BODY' : b}
                fileOut.write(TMPL_CONST.format(**f))

def main(args) :
    parser = ArgumentParser()
    parser.add_argument('src', action='store', help = 'Source directory')
    parser.add_argument('dst', action='store', help = 'Destantion file')
    args = parser.parse_args(sys.argv[1:])
    if not args :
        return

    if not os.path.isdir(args.src) :
        raise Exception('Directory "{}" does not exist'.format(args.src))
        
    tr = TranslateDN2Str()
    for currentFile in glob.glob( os.path.join(args.src, 'dn_*.h') ):    
        tr.processFile(currentFile)
    tr.generate(args.dst)
          
if __name__=='__main__':
   main(sys.argv[1:])
          