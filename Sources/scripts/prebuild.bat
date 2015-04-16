cd /d %1

%1\Sources\Profile\Generator\ProfileCreator.exe  %1\Sources\Profile\Generator\profCU.txt %1\Sources\Profile\profCU.c -d -float 
python.exe %1\Sources\scripts\pc36_iar_adoption.py -iprofCU.c -d %1\Sources\Profile -o %1\Sources\Profile\Generated
copy %1\Sources\Profile\profile_def.h %1\Sources\Profile\Generated\profile_def.h

del %1\Sources\Profile\profile_def.h
del %1\Sources\Profile\langpack.c
del %1\Sources\Profile\profCU.c
del %1\Sources\Profile\profile_handlers.c
del %1\Sources\Profile\profile_ix.c
del %1\Sources\Profile\value.c
 
python.exe %1\Sources\scripts\version_srting.py %1\Sources\version
python.exe %1\Sources\scripts\profile_handlers_cut.py -p %1\Sources\Profile\Generated\profile_handlers.c
python.exe %1\Sources\scripts\par2c.py -i %1\Sources\scripts\powersequencer.par -o %1\Sources\T8_Dnepr_SequencerSettings
