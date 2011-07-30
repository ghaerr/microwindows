@echo .
@echo .
@echo This batch file copies the Makefiles from the djgpp directory into
@echo the directories as required
@echo .
@echo Please run this batch file from the src/contrib/djgpp directory
@pause

ren ..\..\Makefile Makeold
copy Makefile ..\..\Makefile
ren ..\..\mwin\Makefile Makeold
copy Makefile-mwin ..\..\mwin\Makefile
ren ..\..\nanox\Makefile Makeold
copy Makefile-nanox ..\..\nanox\Makefile
ren ..\..\mwin\bmp\Makefile Makeold
copy Makefile-bmp ..\..\mwin\bmp\Makefile
ren ..\..\engine\Makefile Makeold
copy Makefile-engine ..\..\engine\Makefile
ren ..\..\fonts\Makefile Makeold
copy Makefile-fonts ..\..\fonts\Makefile
ren ..\..\drivers\Makefile Makeold
copy Makefile-drivers ..\..\drivers\Makefile

@echo All Makefile copied, old Makefiles renamed

rem copy font file required
copy X6x13.c ..\..\fonts