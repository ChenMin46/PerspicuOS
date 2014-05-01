#!/usr/local/bin/python

from optparse import OptionParser
import os
import re
import subprocess
import sys

# regular expression for file name in objdump output
objFilenamePattern = re.compile(r'^(\S+):\s+file format.+$')

# regular expression for address in objdump output
addressPattern = re.compile(r'^ ([0-9a-f]{4,16}) ')

# regular expression for useful bytes in objdump output
bytesPattern = re.compile(r'^ [0-9a-f]{4,16} ((([0-9a-f][0-9a-f]){1,4} ){1,4})')

# regular expression for mov cr0 reg instruction
movcr0Pattern  = re.compile(r'0f22[048c][0-7]')
movcr0Overlap = 2

# regular expression for mov cr3 reg instruction
movcr3Pattern  = re.compile(r'0f22[159d][89a-f]')
movcr3Overlap = 2

# regular expression for mov cr4 reg instruction
movcr4Pattern  = re.compile(r'0f22[26ae][0-7]')
movcr4Overlap = 2

# regular expression for wrmsr instruction
wrmsrPattern  = re.compile(r'0f30')
wrmsrOverlap = 1

# check a byte string for the given pattern
# and return True if the pattern is not found
def searchPattern(pattern, byteString):
    return not re.search(pattern, byteString)

def processInput(fileObject, pattern, overlap):
    overlapData = ""
    objFilename = ""
    notFound = True
    for inputLine in fileObject:
        (line, address, newObjFilename, newOverlapData) = parseInputLine(inputLine, overlap)
        if newObjFilename:
            objFilename = newObjFilename
        notFound = searchPattern(pattern, overlapData + line)
        if not notFound:
            print objFilename + ": Pattern found near address " + address
        overlapData = newOverlapData
        #print (line, address, newObjFilename, newOverlapData)

def parseInputLine(inputLine, overlap=0):
    # try to find the object file name
    nameMatch = re.match(objFilenamePattern, inputLine)
    if nameMatch:
        objFilename = nameMatch.group(1)
        return ("", "", objFilename, "")

    # try to find the address that should be in the beginning
    # of every valid line
    addressMatch = re.match(addressPattern, inputLine)
    if not addressMatch:
        return ("", "", "", "")
    address = addressMatch.group(1)

    # get the useful bytes
    bytesString = re.search(bytesPattern, inputLine).group(1)

    # clean whitespace to get the final line
    line = re.sub(r'\s', "", bytesString)

    # get requested overlap
    if overlap > 0:
        overlapData = line[(-2)*overlap:]
    else:
        overlapData = ""

    # return result
    return (line, address, "", overlapData)

def applyObjdump(filename):
    cmd = ["objdump","-s","-z","-j",".text",filename]
    tmpFile = os.tmpfile()
    subprocess.check_call(cmd, stdout=tmpFile)
    return tmpFile

# command line options parser
parser = OptionParser()
parser.add_option("-f","--file",action="store",type="string",dest="filename")
parser.add_option("--write-to-cr0",action="store_true",default=False,dest="cr0Check")
parser.add_option("--write-to-cr3",action="store_true",default=False,dest="cr3Check")
parser.add_option("--write-to-cr4",action="store_true",default=False,dest="cr4Check")
parser.add_option("--wrmsr",action="store_true",default=False,dest="wrmsrCheck")

# main
def main() :
    # parse command line arguments
    (options, args) = parser.parse_args()

    # find out input
    if not options.filename and not args:
        print "scanner-objdump.py: Error: No input file was given!"
        raise OSError
    elif not options.filename:
        filename = args[0]
    else:
        filename = options.filename

    # apply objdump
    tmpFile = applyObjdump(filename)

    # process input file contents
    if options.cr0Check:
        tmpFile.seek(0)
        processInput(tmpFile, movcr0Pattern, movcr0Overlap)

    if options.cr3Check:
        tmpFile.seek(0)
        processInput(tmpFile, movcr3Pattern, movcr3Overlap)

    if options.cr4Check:
        tmpFile.seek(0)
        processInput(tmpFile, movcr4Pattern, movcr4Overlap)

    if options.wrmsrCheck:
        tmpFile.seek(0)
        processInput(tmpFile, wrmsrPattern, wrmsrOverlap)

    # close the temporary file
    tmpFile.close()

if __name__ == '__main__':
    main()

