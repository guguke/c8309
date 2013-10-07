com0com ver 3.0.0  2013.07.14
file changed:
cncport.inf
com0com.inf
comport.inf
NSIS\install.nsi
setup\setup.cpp
sys\pnp.c

// 以下文件copy至WinDDK生成的对应xp的驱动目录下(com0com\i386) ,
com0com.cat  dir : com0com\i386
com0com.cer
com0com.pvk
com0com.pfx

certmgr.exe  ( <= winDDK\76.....\bin\x86\certmgr.exe
  xp: c8309\com0com\x86\certmgr.exe