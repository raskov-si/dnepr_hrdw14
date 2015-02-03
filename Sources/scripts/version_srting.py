# -*- coding: utf-8 -*-

import subprocess, re, datetime, sys, os

########################################################################################################################
# ����-�����

date_str = str(datetime.datetime.now())

########################################################################################################################
# ���� ���������� �� git'�

# ���������, ������� �� ������� �����
p = subprocess.Popen( "git.exe diff-index --minimal --name-only HEAD --".split(), shell=False, stdout=subprocess.PIPE )
res = p.stdout.read()
dirty_copy_bl = res != "" # True ����� ������� ����� �������

# ��� �������
p = subprocess.Popen( "git.exe rev-parse HEAD".split(), shell=False, stdout=subprocess.PIPE )
sha1_str = p.stdout.read() # ��� ���
sha1_str = sha1_str.strip()
sha1_str = sha1_str[:10]
p.wait()

# ���� �������� ������� �����
p = subprocess.Popen( ["git.exe", "branch"], shell=False, stdout=subprocess.PIPE )
branch_str = p.stdout.read()
p.wait()

branch_str = re.findall(r"\* (.*)", branch_str ) # �������� ������� �����
if len(branch_str):
	branch_str = branch_str[0]
else:
	branch_str =  os.getcwd()

########################################################################################################################
# ������

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
