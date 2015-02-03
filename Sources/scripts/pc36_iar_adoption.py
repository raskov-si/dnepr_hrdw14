# -*- coding: utf-8 -*-
# Программа для перевода исходников, сделанных ProfileCreator 3.5 для CodeWarrior,
# к виду, понятному IAR'у

import os, sys, re
from optparse import OptionParser

################################################################################
################################################################################
# main

# парсим параметры командной строки
parser = OptionParser()
parser.add_option( "-i", "--input", action="store", 
					type="string", dest="input_file",
					help="input file name such as profile.c")
parser.add_option( "-d", "--input-dir", action="store", 
					type="string", dest="in_dir",
					help="path to input files")
parser.add_option( "-o", "--output-dir", action="store", 
					type="string", dest="out_dir",
					help="Path to output files")

(options, args) = parser.parse_args()

if ((not parser.has_option("-i")) or
	(not parser.has_option("-d")) or
	(not parser.has_option("-o"))):
	print "You must declare -i, -o and -d"
	sys.exit()

 
################################################################################
# profile.c

in_fullpath = options.in_dir+"\\"+options.input_file

if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.out_dir + "\\" + options.input_file, 'w' )

in_lines_list = in_file_desc.readlines()

# удаляем 8-9 строки
del in_lines_list[7:9]
in_lines_list = "".join(in_lines_list)

# составляем список ta*
ta_list = re.findall(r"u8 (ta\d*)\[", in_lines_list )

# заменяем u8 ta* на _Pragma("location=\"profile\"") const 
in_lines_list = re.sub( r"(u8 ta\d*)", r"""_Pragma("location=\\"profile\\"") const \1""", in_lines_list)

# заменяем PARAM pT* на то же самое
in_lines_list = re.sub( r"(PARAM pT\d*=)", r"""_Pragma("location=\\"profile\\"") const \1""", in_lines_list)

# делаем PARAM ... (void*)___, (слово) };
in_lines_list = re.sub( r"const PARAM (.*),(((?!NULL).)*),(\w*)\};", r"""const PARAM \1,(void*)\2, \4};""", in_lines_list)
# делаем PARAM ... (void*)___ };
in_lines_list = re.sub( r"const PARAM (.*),(((?!NULL).)*)\};", r"""const PARAM \1,(void*)\2};""", in_lines_list)

# находим последний pT* (с нулевым alter_length) -- его никто не смотрит, поэтому его нужно где-то использовать
last_pT_number = re.findall( r"(pT\d*)={0x0};\n", in_lines_list )

out_file_desc.write(in_lines_list)
out_file_desc.close()

# profile.c готов!

################################################################################
# делаем ta_involver.c, в котором читаем все ta*, чтобы их не удалили за ненадобностью

out_file_desc = open( options.out_dir + "\\ta_and_pT_involver.c", 'w' )

out_string = "#include \"Profile/inc/sys.h\"\n\n"
out_string = out_string + "extern PARAM const " + last_pT_number[0] + ";\n\n"
for ta in ta_list:
	out_string = out_string+"extern const unsigned char " + ta + "[];\n"

out_string = out_string + "\nextern const s8 lang_pack[] ;\nextern const u8 padding[] ;\n\n "

out_string = out_string + "\nvoid ta_involver()\n{\n"
out_string = out_string + "\tunsigned char buff;\n"
out_string = out_string + "\tPARAM pt_buff;\n\n"

out_string = out_string + "\tbuff = lang_pack[0];\n\tbuff = padding[0];\n\n"

for ta in ta_list:
	out_string = out_string+"\tbuff = " + ta + "[0];\n"

out_string = out_string + "\n\tpt_buff.alter_length = " + last_pT_number[0] + ".alter_length ;\n"
out_string = out_string + "}\n"

out_file_desc.write(out_string)
out_file_desc.close()

################################################################################
# value.c

in_fullpath = options.in_dir+"\\value.c"

if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.out_dir + "\\value.c", 'w' )

in_lines_list = in_file_desc.readlines()

# убираем 4 первых строки
del in_lines_list[0:4]
del in_lines_list[-1]
in_lines_list = "".join(in_lines_list)

# в начале каждой строки добавляем _Pragma(...)
# in_lines_list = re.sub( r"(.*)\n", r"""_Pragma("location=\"settings\"") const \1\n""", in_lines_list)

out_file_desc.write(in_lines_list)
out_file_desc.close

# тоже готов

################################################################################
# profile_ix.c

in_fullpath = options.in_dir+"\\profile_ix.c"

if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.out_dir + "\\profile_ix.c", 'w' )

in_lines_list = in_file_desc.readlines()

# убираем const у PARAM_INDEX 
in_lines_list[5] = in_lines_list[5][6:]
in_lines_list = "".join(in_lines_list)

out_file_desc.write(in_lines_list)
out_file_desc.close

################################################################################
# profile_handlers.c

# в начале каждой строки добавляем _Pragma(...)
in_fullpath = options.in_dir+"\\profile_handlers.c"

if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.out_dir + "\\profile_handlers.c", 'w' )

for s in in_file_desc.readlines():
	out_file_desc.write(s.replace("u32 dyn_par_access(PARAM* p_id, ", "u32 dyn_par_access(PARAM_INDEX* p_id, "))

out_file_desc.close


################################################################################
# langpack.c

in_fullpath = options.in_dir+"\\langpack.c"

if not os.path.isfile( in_fullpath ):
	print "File "+in_fullpath+" missing"
	sys.exit()

print "input file: "+in_fullpath

in_file_desc = open( in_fullpath, 'r')
out_file_desc = open( options.out_dir + "\\langpack.c", 'w' )

in_lines_list = in_file_desc.readlines()

# убираем 2 первых строки
del in_lines_list[6:9]
del in_lines_list[7]
in_lines_list = in_lines_list[0:-2]
in_lines_list = "".join(in_lines_list)

# заменяем u8 langpack_header на _Pragma("location=\\"rom_langpack\\"") const
in_lines_list = re.sub( r"(u8 langpack_header)", r"""_Pragma("location=\\"rom_langpack\\"") const \1""", in_lines_list)
# заменяем s8 lang_pack на _Pragma("location=\\"rom_langpack\\"") const
in_lines_list = re.sub( r"(s8 lang_pack)", r"""_Pragma("location=\\"rom_langpack\\"") const \1""", in_lines_list)
# заменяем u8 padding на _Pragma("location=\\"rom_langpack\\"") const
in_lines_list = re.sub( r"(u8 padding)", r"""_Pragma("location=\\"rom_langpack\\"") const \1""", in_lines_list)

# ищем кавычку в кавычках и ставим экран
in_lines_list = re.sub( r"(''')", r"""'\\''""", in_lines_list)

out_file_desc.write(in_lines_list)
out_file_desc.close


