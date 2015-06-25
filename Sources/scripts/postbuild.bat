cd /d %1

python.exe %1\Sources\scripts\bincopy.py cat %1\Release\Exe\Dnepr.s19 --output %1\Release\Exe\Dnepr_wide.s19
python.exe %1\Sources\scripts\release_postbuild.py -v %1\Sources\Profile\Generated\value.c -c DNEP0020 -i %1\Release\Exe\Dnepr_wide.s19
del %1\Release\Exe\Dnepr_wide.s19

