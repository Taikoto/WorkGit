
@set version=1.7.2Normal
@echo =============================================================
@echo II                     Log gripping tool                II
@echo II                     1.7.2  Normal                    II

@echo =============================================================

@set pcTime= %DATE% %TIME:~0,8%

@echo  Logtool %version%  >> %dirName%/pcTime
@echo  %pcTime%  >> %dirName%/pcTime
@echo ---------------------------------------------------

@echo   1.  Start Crawl main，events，radio log
@adb logcat -b main -b system -b radio -b events  -v time > %dirName%/all_user.log
@echo       Crawl Kernel process list end
@echo ---------------------------------------------------


@echo ===================================================
@echo II                      End                      II
@echo ===================================================

pause