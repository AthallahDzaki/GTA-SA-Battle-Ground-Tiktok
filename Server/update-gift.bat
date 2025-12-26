@echo off

set /p "tiktok=Enter Your Tiktok Username (Jangan ada Spasi): "

call node etc/update-gift.js %tiktok%

echo Update done
pause