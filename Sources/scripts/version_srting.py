# -*- coding: utf-8 -*-

import subprocess, re, datetime, sys, os

########################################################################################################################
# дата-время

date_str = str(datetime.datetime.now())

########################################################################################################################
# берём информацию из git'а

# проверяем, грязная ли рабочая копия
p = subprocess.Popen( "git.exe diff-index --minimal --name-only HEAD --".split(), shell=False, stdout=subprocess.PIPE )
res = p.stdout.read()
dirty_copy_bl = res != "" # True когда рабочая копия грязная

# хэш коммита
p = subprocess.Popen( "git.exe rev-parse HEAD".split(), shell=False, stdout=subprocess.PIPE )
sha1_str = p.stdout.read() # сам хэш
sha1_str = sha1_str.strip()
sha1_str = sha1_str[:10]
p.wait()

# берём название текущей ветки
p = subprocess.Popen( ["git.exe", "branch"], shell=False, stdout=subprocess.PIPE )
branch_str = p.stdout.read()
p.wait()

branch_str = re.findall(r"\* (.*)", branch_str ) # название текущей ветки
if len(branch_str):
	branch_str = branch_str[0]
else:
	branch_str =  os.getcwd()

########################################################################################################################
# строка

if len(sys.argv) < 2:
	sys.exit()
else:
	outfile_str = sys.argv[1]

out_file_desc = open( outfile_str, 'w' )


if dirty_copy_bl:
	out_file_desc.write( "const char sVerion[] = \"*" + branch_str + ":" + sha1_str +
					"\";\n" )
else:
	out_file_desc.write( "const char sVerion[] = \"" + branch_str + ":" + sha1_str +
					"\";\n" )
out_file_desc.write( "const char sCompileDate[] = \"" + date_str + "\";\n" )
out_file_desc.close()
