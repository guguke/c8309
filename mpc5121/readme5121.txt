5121 nand 烧写步骤:
1. 烧u-boot, 
   烧写文件, 实验室机器, d:\chen\u-boot.fec.103.bin
2. 开主机 192.168.1.24
3. 启动u-boot , 按回车键进入u-boot命令提示状态
4. 输入命令 run tftp2nand + 回车, 命令结束,完成nand烧写
5. 重新启动板子, 等待linux启动, 登录提示时, 使用用户名 root 登录, 密码也是root
6. linux登录后输入   ./demo_qt + 回车, 启动qt的始终演示程序



















