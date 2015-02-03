# -*- coding: utf-8 -*-

import sys, os, re
from optparse import OptionParser

########################################################################################################################

def TransformToAsciiHex( str ):
	return reduce(lambda x, y: x+"{0:X}".format(y), [ord(c) for c in str], "" )

def HexLen( str ):
	symbs = [c for c in str]
	return "{0:0=2X}".format(int((len(symbs)+2)/2))

def CalcSum( str ):
	symbs = [c for c in str]
	sbytes = map( lambda x, y: x+y, symbs[::2], symbs[1::2])
	sm = sum( [ int( x, 16 ) for x in sbytes ] )

	sm = (~(sm & 0xFF)) & 0xFF

	return "{0:0=2X}".format(sm)

########################################################################################################################

# парсим параметры командной строки
parser = OptionParser()
parser.add_option( "-i", "--input", action="store", 
					type="string", dest="input_file",
					help="path to input file *.S19")
parser.add_option( "-o", "--output", action="store", 
					type="string", dest="output_file",
					help="path to output file" )
parser.add_option( "-c", "--comment", action="store", 
					type="string", dest="comment",
					help="8 symobl of project classifier" )
(options, args) = parser.parse_args()

if not (options.comment and options.output_file and options.input_file):
	print "You must declare -i, -o and -c. Type -h for help"
	sys.exit(1)

if len(options.comment) != 8:
	print "-c string must be 8 symbols"

########################################################################################################################

in_fullpath = options.input_file
if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.output_file, 'w' )

print "output file: "+options.output_file

in_lines_list = in_file_desc.readlines()

# Пишем 8 байт коммента 2й строкой
str = "0000" + TransformToAsciiHex(options.comment)
str = HexLen( str ) + str
str = str + CalcSum( str )
in_lines_list.insert(1, "S0{0}\n".format( str ))


########################################################################################################################
# Пишем кол-во строк s3 предпоследней строкой

N = len(re.findall( r"S[1,2,3]{1}", "".join(in_lines_list) ))
str = "{0:0=4X}".format( N )
str = HexLen( str ) + str
str = str + CalcSum( str )

in_lines_list.insert(-1, "S5{0}\n".format( str ))

########################################################################################################################

out_file_desc.write( "".join(in_lines_list) )
out_file_desc.close
