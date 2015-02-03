# -*- coding: utf-8 -*-
# Программа для перевода файла конфигурации программы
# Texas Instruments UCD9080 EVM GUI (File->Save Configuration)
# код Си

import os, subprocess, re, datetime, sys
from optparse import OptionParser


################################################################################
################################################################################
# main

# парсим параметры командной строки
parser = OptionParser()
parser.add_option( "-i", "--input", action="store", 
					type="string", dest="input_file",
					help="path to input file *.par")
parser.add_option( "-o", "--output", action="store", 
					type="string", dest="output_files",
					help="path to output files _par_.h and _par_value.c" )
(options, args) = parser.parse_args()

istr1 = 5 	# номер строки файла.par с которой считаем значения 
istr2 = 517	# номер строки файла.par на которой заканчиваем считать значения 

# TODO: не работает проверка, надо добиться того, чтобы работала
if (not parser.has_option("-o")) or (not parser.has_option("-i")):
	print "Необходимо задать параметры -o и -i"
	sys.exit()
if not os.path.isfile( options.input_file ):
	print "Входной файл"+options.output_files+" отсуствует"
	sys.exit()

print "input file: "+options.input_file
print "output files: "+options.output_files+".h",
print " and "+options.output_files+".c"

in_file_desc = open( options.input_file, 'r')
out_c_file_desc = open( options.output_files+".c", 'w')
out_h_file_desc = open( options.output_files+".h", 'w')

################################################################################

# берём хэш из git'а последней версии файла конфигурации секвенсора
p = subprocess.Popen( ('git.exe log --pretty=format:"%%H" %s' % options.input_file).split(), shell=False, stdout=subprocess.PIPE )
conf_sha = p.stdout.read()
conf_sha = conf_sha.strip()
conf_sha = conf_sha[1:-1]
print 'Config sha: ', conf_sha

################################################################################

out_h_file_desc.write( "#define		SEQ_FLASH_LEN {0}\n".format( istr2-istr1 ) )
out_h_file_desc.write( "extern const u8 sequencer_flash_chars[SEQ_FLASH_LEN];\n\n" )

out_h_file_desc.write( "#define     SEQ_USERSPACE_LEN {0}\n".format( 128 ) )
out_h_file_desc.write( "extern const u8 sequencer_userspace_chars[SEQ_USERSPACE_LEN];\n" )

out_c_file_desc.write( '#include "support_common.h"\n' )
out_c_file_desc.write( '#include "%s"\n\n' % (options.output_files+".h") )
out_c_file_desc.write( '_Pragma("location=\\"power_sequencer_settings\\"") const u8 sequencer_flash_chars[SEQ_FLASH_LEN] = {\n' )

in_lines_list = in_file_desc.readlines()
nms_in_line = 0
for numline in range(istr1, istr2-1):
	out_c_file_desc.write( "0x"+in_lines_list[numline].strip()+", " )
	nms_in_line = nms_in_line + 1
	if nms_in_line >= 10:
		nms_in_line = 0
		out_c_file_desc.write( "\n" )
numline = numline+1
out_c_file_desc.write( "0x"+in_lines_list[numline].strip()+"\n};\n\n" )

out_c_file_desc.write( '_Pragma("location=\\"power_sequencer_settings\\"") const u8 sequencer_userspace_chars[SEQ_USERSPACE_LEN]={\n' )
nms_in_line = 0
for i in range( len(conf_sha) ):
	out_c_file_desc.write( "0x"+conf_sha[i] + ", " )
	nms_in_line = nms_in_line + 1
	if nms_in_line >= 10:
		nms_in_line = 0
		out_c_file_desc.write( "\n" )
for i in range( len(conf_sha), 127):
	if( i % 4 == 0 ):
		out_c_file_desc.write( "0xDE, " );
	elif( i % 4 == 1 ):
		out_c_file_desc.write( "0xAD, " );
	elif( i % 4 == 2 ):
		out_c_file_desc.write( "0xBE, " );
	else:
		out_c_file_desc.write( "0xEF, " );
	nms_in_line = nms_in_line + 1
	if nms_in_line >= 10:
		nms_in_line = 0
		out_c_file_desc.write( "\n" )
out_c_file_desc.write( "0xEF\n};\n")
out_c_file_desc.close()
