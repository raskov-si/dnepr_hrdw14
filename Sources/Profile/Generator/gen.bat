%1\Sources\Profile\Generator\ProfileCreator.exe  %1\Sources\Profile\Generator\profCU.txt %1\Sources\Profile\profCU.c -d -float 
python.exe %1\Sources\scripts\pc36_iar_adoption.py -iprofCU.c -d %1\Sources\Profile -o %1\Sources\Profile\Generated
cp %1\Sources\Profile\profile_def.h %1\Sources\Profile\Generated\profile_def.h

rm %1\Sources\Profile\profile_def.h
rm %1\Sources\Profile\langpack.c
rm %1\Sources\Profile\profCU.c
rm %1\Sources\Profile\profile_handlers.c
rm %1\Sources\Profile\profile_ix.c
rm %1\Sources\Profile\value.c
