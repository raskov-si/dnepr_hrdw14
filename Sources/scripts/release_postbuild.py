# -*- coding: utf-8 -*-

import sys, os, re
from optparse import OptionParser

########################################################################################################################

# парсим параметры командной строки
parser = OptionParser()
parser.add_option( "-i", "--input", action="store", 
					type="string", dest="input_file",
					help="path to input file *.S19")
parser.add_option( "-v", "--valuefile", action="store", 
					type="string", dest="value_file",
					help="path to value.c file")
parser.add_option( "-c", "--comment", action="store", 
					type="string", dest="comment",
					help="8 symobl of project classifier" )
(options, args) = parser.parse_args()

if not (options.comment and options.input_file and options.value_file ):
	print "You must declare -i, -o and -c. Type -h for help"
	sys.exit(1)

if len(options.comment) != 8:
	print "-c string must be 8 symbols"

########################################################################################################################
# Находим версию прошивки в Profile/Generated/value.c

val_file = options.value_file

if not os.path.isfile( val_file ):
	print "File "+val_file+" missing"
	sys.exit()

val_file_desc = open( val_file, 'r')
val_lines_list = val_file_desc.readlines()
val_lines_list = "".join(val_lines_list)

version = re.findall( r"val_CMSwNumber\[\d*\]\s=\s\"((\d|\.)*)\"", val_lines_list )
version = version[0][0]

val_file_desc.close()

########################################################################################################################
# Делаем .s19 для бутлоадера
#now_time = datetime.datetime.now()
#now_time.strftime("%d.%m.%Y %I:%M %p")
os.system( "python.exe {0}\s19adopt.py -i {1} -o {2} -c {3}".format( \
		os.path.dirname(os.path.realpath(__file__)),\
		options.input_file,\
		os.path.dirname(options.input_file)+'\\dnepr_1.4_'+version+'_date_app_rmt.s19',\
		options.comment) )
