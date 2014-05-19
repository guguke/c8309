20140515:
  dir grs: general run silent, 
        build vc2005, .net framework 2.0
  dir gr : run normal, not silent

file install0.nsi == install.nsi.v0
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
  xp: c8309\com0com\i386\certmgr.exe
plink.exe           <= pkg putty
hub4com.exe        <== pkg hub4com
   copy to com0com\i386 , rename to h4c*.exe   h4c0.exe h4c1.exe h4c2.exe   
mongoose.exe            rename netport.exe


