# -*- coding: utf-8 -*-

import sys, os, re
from optparse import OptionParser

########################################################################################################################

# парсим параметры командной строки
parser = OptionParser()

parser.add_option( "-p", "--profile-handlers", action="store", 
					type="string", dest="profile_handlers_file",
					help="path to profile_handlers.c file" )

(options, args) = parser.parse_args()

if not (options.profile_handlers_file):
	print "You must declare -i, -o and -c. Type -h for help"
	sys.exit(1)


########################################################################################################################
# Убираем тела функций в Profile/Generated/profile_handlers.c

prof_file = options.profile_handlers_file

if not os.path.isfile( prof_file ):
	print "File "+prof_file+" missing"
	sys.exit()
print "Profile handlers file: " + prof_file

pr_file_desc = open( prof_file, 'r' )
pr_lines_list = pr_file_desc.readlines()
pr_file_desc.close()

pr_lines_list = "".join( pr_lines_list )

# в выделенном тексте заменяем {*} на ;
pr_lines_list = re.sub( r"\s*\{[\d\s\w;(),=\!]*\}", r";", pr_lines_list )
# удаляем пустые строки
pr_lines_list = re.sub( r"(\n){2,}", r"\n", pr_lines_list )

pr_file_desc = open( prof_file, 'w' )
pr_file_desc.write( pr_lines_list )
pr_file_desc.close()
