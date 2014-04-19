// Copyright (c) 2004-2013 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warning in VS2005
#else
#define _XOPEN_SOURCE 600  // For PATH_MAX on linux
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>

#include "mongoose.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
//#include <windows.h>
#include <winsvc.h>
#include <shlobj.h>
#include <process.h>
#include <time.h>
//#include <strsafe.h>

#define PATH_MAX MAX_PATH
#define S_ISDIR(x) ((x) & _S_IFDIR)
#define DIRSEP '\\'
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) * 1000)
#define WINCDECL __cdecl
#else
#include <sys/wait.h>
#include <unistd.h>
#define DIRSEP '/'
#define WINCDECL
#endif // _WIN32

#define MAX_OPTIONS 60
#define MAX_CONF_FILE_LINE_SIZE (8 * 1024)

static int exit_flag;
static char server_name[40];        // Set by init_server_name()
static char config_file[PATH_MAX];  // Set by process_command_line_arguments()
static struct mg_context *ctx;      // Set by start_mongoose()

#if !defined(CONFIG_FILE)
#define CONFIG_FILE "netport.conf"
#endif /* !CONFIG_FILE */

static void WINCDECL signal_handler(int sig_num) {
	exit_flag = sig_num;
}

static void die(const char *fmt, ...) {
	va_list ap;
	char msg[200];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

#if defined(_WIN32)
	MessageBox(NULL, msg, "Error", MB_OK);
#else
	fprintf(stderr, "%s\n", msg);
#endif

	exit(EXIT_FAILURE);
}

static void show_usage_and_exit(void) {
	const char **names;
	int i;

	fprintf(stderr, "Mongoose version %s (c) Sergey Lyubka, built %s\n",
		mg_version(), __DATE__);
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  mongoose -A <htpasswd_file> <realm> <user> <passwd>\n");
	fprintf(stderr, "  mongoose <config_file>\n");
	fprintf(stderr, "  mongoose [-option value ...]\n");
	fprintf(stderr, "\nOPTIONS:\n");

	names = mg_get_valid_option_names();
	for (i = 0; names[i] != NULL; i += 3) {
		fprintf(stderr, "  -%s %s (default: \"%s\")\n",
			names[i], names[i + 1], names[i + 2] == NULL ? "" : names[i + 2]);
	}
	fprintf(stderr, "\nSee  http://code.google.com/p/mongoose/wiki/MongooseManual"
		" for more details.\n");
	fprintf(stderr, "Example:\n  mongoose -s cert.pem -p 80,443s -d no\n");
	exit(EXIT_FAILURE);
}

#if defined(_WIN32) || defined(USE_COCOA)
static const char *config_file_top_comment =
	"# Mongoose web server configuration file.\n"
	"# For detailed description of every option, visit\n"
	"# https://github.com/valenok/mongoose/blob/master/UserManual.md\n"
	"# Lines starting with '#' and empty lines are ignored.\n"
	"# To make a change, remove leading '#', modify option's value,\n"
	"# save this file and then restart Mongoose.\n\n";

static const char *get_url_to_first_open_port(const struct mg_context *ctx) {
	static char url[100];
	const char *open_ports = mg_get_option(ctx, "listening_ports");
	int a, b, c, d, port, n;

	if (sscanf(open_ports, "%d.%d.%d.%d:%d%n", &a, &b, &c, &d, &port, &n) == 5) {
		snprintf(url, sizeof(url), "%s://%d.%d.%d.%d:%d",
			open_ports[n] == 's' ? "https" : "http", a, b, c, d, port);
	} else if (sscanf(open_ports, "%d%n", &port, &n) == 1) {
		snprintf(url, sizeof(url), "%s://localhost:%d",
			open_ports[n] == 's' ? "https" : "http", port);
	} else {
		snprintf(url, sizeof(url), "%s", "http://localhost:8080");
	}

	return url;
}

static void create_config_file(const char *path) {
	const char **names, *value;
	FILE *fp;
	int i;

	// Create config file if it is not present yet
	if ((fp = fopen(path, "r")) != NULL) {
		fclose(fp);
	} else if ((fp = fopen(path, "a+")) != NULL) {
		fprintf(fp, "%s", config_file_top_comment);
		names = mg_get_valid_option_names();
		for (i = 0; names[i] != NULL; i += 3) {
			value = mg_get_option(ctx, names[i]);
			fprintf(fp, "# %s %s\n", names[i + 1], *value ? value : "<value>");
		}
		fclose(fp);
	}
}
#endif

static void verify_document_root(const char *root) {
	const char *p, *path;
	char buf[PATH_MAX];
	struct stat st;

	path = root;
	if ((p = strchr(root, ',')) != NULL && (size_t) (p - root) < sizeof(buf)) {
		memcpy(buf, root, p - root);
		buf[p - root] = '\0';
		path = buf;
	}

	if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
		die("Invalid root directory: [%s]: %s", root, strerror(errno));
	}
}

static char *sdup(const char *str) {
	char *p;
	if ((p = (char *) malloc(strlen(str) + 1)) != NULL) {
		strcpy(p, str);
	}
	return p;
}

static void set_option(char **options, const char *name, const char *value) {
	int i;

	if (!strcmp(name, "document_root") || !(strcmp(name, "r"))) {
		verify_document_root(value);
	}

	for (i = 0; i < MAX_OPTIONS - 3; i++) {
		if (options[i] == NULL) {
			options[i] = sdup(name);
			options[i + 1] = sdup(value);
			options[i + 2] = NULL;
			break;
		}
	}

	if (i == MAX_OPTIONS - 3) {
		die("%s", "Too many options specified");
	}
}

static void process_command_line_arguments(char *argv[], char **options) {
	char line[MAX_CONF_FILE_LINE_SIZE], opt[sizeof(line)], val[sizeof(line)], *p;
	FILE *fp = NULL;
	size_t i, cmd_line_opts_start = 1, line_no = 0;

	options[0] = NULL;

	// Should we use a config file ?
	if (argv[1] != NULL && argv[1][0] != '-') {
		snprintf(config_file, sizeof(config_file), "%s", argv[1]);
		cmd_line_opts_start = 2;
	} else if ((p = strrchr(argv[0], DIRSEP)) == NULL) {
		// No command line flags specified. Look where binary lives
		snprintf(config_file, sizeof(config_file), "%s", CONFIG_FILE);
	} else {
		snprintf(config_file, sizeof(config_file), "%.*s%c%s",
			(int) (p - argv[0]), argv[0], DIRSEP, CONFIG_FILE);
	}

	fp = fopen(config_file, "r");

	// If config file was set in command line and open failed, die
	if (cmd_line_opts_start == 2 && fp == NULL) {
		die("Cannot open config file %s: %s", config_file, strerror(errno));
	}

	// Load config file settings first
	if (fp != NULL) {
		fprintf(stderr, "Loading config file %s\n", config_file);

		// Loop over the lines in config file
		while (fgets(line, sizeof(line), fp) != NULL) {
			line_no++;

			// Ignore empty lines and comments
			for (i = 0; isspace(* (unsigned char *) &line[i]); ) i++;
			if (line[i] == '#' || line[i] == '\0') {
				continue;
			}

			if (sscanf(line, "%s %[^\r\n#]", opt, val) != 2) {
				printf("%s: line %d is invalid, ignoring it:\n %s",
					config_file, (int) line_no, line);
			} else {
				set_option(options, opt, val);
			}
		}

		(void) fclose(fp);
	}

	// Handle command line flags. They override config file and default settings.
	for (i = cmd_line_opts_start; argv[i] != NULL; i += 2) {
		if (argv[i][0] != '-' || argv[i + 1] == NULL) {
			show_usage_and_exit();
		}
		set_option(options, &argv[i][1], argv[i + 1]);
	}
}
// netport 0.03
static void init_server_name(void) {
	snprintf(server_name, sizeof(server_name), "NetPort v0.03");
}

static void *mongoose_callback(enum mg_event ev, struct mg_connection *conn) {
	if (ev == MG_EVENT_LOG) {
		printf("%s\n", (const char *) mg_get_request_info(conn)->ev_data);
	}

	// Returning NULL marks request as not handled, signalling mongoose to
	// proceed with handling it.
	return NULL;
}
static int gnLog=0;
void mylog(char *p)
{
	FILE *fp;
	int f=1;   /////////////// debug
	time_t t;
	struct tm *now;
	char str[100];

	if(f>0) return;

	time(&t);
	now=localtime(&t);
	sprintf(str,("%d-%d %02d:%02d:%02d"),now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec);

	fp=fopen("mylog.txt","a+t");
	if(fp==NULL)return;
	if(gnLog==0)fprintf(fp," ================================== \n");
	fprintf(fp,"%d %s %s",gnLog++,str,p);
	fclose(fp);
	return;
}
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
#if 0
void ErrorExit(PTSTR lpszFunction) 

	// Format a readable error message, display a message box, 
	// and exit from the application.
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}
#endif
void CreateChildProcess()
	// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
	TCHAR szCmdline[]=TEXT("plink -telnet -P 2000 192.168.1.77");///////////////////////////////////////////////// debug\\hello or child
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 
	int i;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(NULL, 
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if ( ! bSuccess ) {
		i=1;
		//ErrorExit(TEXT("CreateProcess"));
	}
	else 
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}

DWORD RunSilent(char* strFunct, char* strstrParams)
{
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	char Args[4096];
	char *pEnvCMD = NULL;
	char *pDefaultCMD = "CMD.EXE";
	DWORD rc;
	int ret;

	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_HIDE;//////////////////////////// 

	Args[0] = 0;

	pEnvCMD = getenv("COMSPEC");

	if(pEnvCMD){

		strcpy(Args, pEnvCMD);
	}
	else{
		strcpy(Args, pDefaultCMD);
	}

	// "/c" option - Do the command then terminate the command window
	strcat(Args, " /c "); 
	//the application you would like to run from the command window
	strcat(Args, strFunct);  
	strcat(Args, " "); 
	//the parameters passed to the application being run from the command window.
	strcat(Args, strstrParams); 

	if (!CreateProcess( NULL, Args, NULL, NULL, FALSE,
		CREATE_NEW_CONSOLE, 
		NULL, 
		NULL,
		&StartupInfo,
		&ProcessInfo))
	{
		return GetLastError();             
	}

	//printf("pause..\n");
	//getch();
	//WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	if(!GetExitCodeProcess(ProcessInfo.hProcess, &rc))	rc = 0;

	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);

	//printf(" terminate ?\n");
	//getch();
	//TerminateProcess(ProcessInfo.hProcess,0);

	return rc;

}

static int getHex(char *str)
{
	int i=0,ret;
	ret = sscanf(str,"%d",&i);
	if( strcmp(str,"0x") == 0 || strcmp(str,"0X") == 0 ){
		ret = sscanf(str,"%x",&i);
	}
	return i;
}

void getTimeStr(char *str)
{
	time_t t;
	struct tm *now;
	time(&t);
	now=localtime(&t);
	sprintf(str,("%02d:%02d:%02d"),now->tm_hour,now->tm_min,now->tm_sec);
}
void getTimeTStr(time_t *pt,char *str)
{
	//time_t t;
	struct tm *now;
	time(pt);
	now=localtime(pt);
	sprintf(str,("%02d:%02d:%02d"),now->tm_hour,now->tm_min,now->tm_sec);
}

// MCASTADDR : 226.1.1.1
// MCASTPORT : 4321
// sendbuf :
int sleaf(char *MCASTADDR,int MCASTPORT,char *sendbuf,int nLen)
{
	DWORD dwInterface,          // Local interface to bind to
		dwMulticastGroup;     // Multicast group to join
	//dwCount;              // Number of messages to send/receive
	short iPort;                // Port number to use

	struct sockaddr_in  local,		remote;
	SOCKET              sock, sockM;
	//TCHAR               sendbuf[BUFSIZE];
	int                 len = sizeof(struct sockaddr_in),		optval;

	dwInterface = INADDR_ANY;
	dwMulticastGroup = inet_addr(MCASTADDR);
	iPort = MCASTPORT;

	// Create the socket. In Winsock 2, you do have to specify the
	// multicast attributes that this socket will be used with.
	//
	if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF 
		| WSA_FLAG_MULTIPOINT_D_LEAF 
		| WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("socket failed with: %d\n", WSAGetLastError());
		//WSACleanup();
		return -2;
	}
	// Bind to the local interface. This is done to receive data.
	local.sin_family = AF_INET;
	local.sin_port   = htons(iPort);
	local.sin_addr.s_addr = dwInterface;

	if (bind(sock, (struct sockaddr *)&local, 
		sizeof(local)) == SOCKET_ERROR)
	{
		printf("bind failed with: %d\n", WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -3;
	}
	// Set up the SOCKADDR_IN structure describing the multicast 
	// group we want to join
	//
	remote.sin_family      = AF_INET;
	remote.sin_port        = htons(iPort);
	remote.sin_addr.s_addr = dwMulticastGroup;
	//
	// Change the TTL to something more appropriate
	//

	optval = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
		(char *)&optval, sizeof(int)) == SOCKET_ERROR)
	{
		printf("setsockopt(IP_MULTICAST_TTL) failed: %d\n",
			WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -4;
	}
	// Disable loopback if needed
	//
#if 0
	if (bLoopBack)
	{
		optval = 0;
		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
			(char *)&optval, sizeof(optval)) == SOCKET_ERROR)
		{
			printf("setsockopt(IP_MULTICAST_LOOP) failed: %d\n",
				WSAGetLastError());
			closesocket(sock);
			WSACleanup();
			return -1;
		}
	}
#endif
	// Join the multicast group.  Note that sockM is not used 
	// to send or receive data. It is used when you want to 
	// leave the multicast group. You simply call closesocket() 
	// on it.
	//
	if ((sockM = WSAJoinLeaf(sock, (SOCKADDR *)&remote, 
		sizeof(remote), NULL, NULL, NULL, NULL, 
		JL_SENDER_ONLY)) == INVALID_SOCKET)              //  JL_BOTH : send and rcv
	{
		printf("WSAJoinLeaf() failed: %d\n", WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -5;
	}

	// Send data
	//sprintf(sendbuf, "server 1: This is a test: ");
	if (sendto(sock, (char *)sendbuf, nLen, 0,
		(struct sockaddr *)&remote, 
		sizeof(remote)) == SOCKET_ERROR)
	{
		printf("sendto failed with: %d\n", 
			WSAGetLastError());
		closesocket(sockM);
		closesocket(sock);
		//WSACleanup();
		return -7;
	}
	// Leave the multicast group by closing sock.
	// For nonrooted control and data plane schemes, WSAJoinLeaf
	// returns the same socket handle that you pass into it.
	//
	closesocket(sockM);
	closesocket(sock);

	//WSACleanup();
	return 0;
}

int mcChangeip(char *newip,char *pmask)
{
	char sendbuf[1024];
	int nLen=0;

	sprintf(sendbuf,"changeip %s %s",newip,pmask);
	nLen=strlen(sendbuf);
	sleaf("226.1.1.1",4321,sendbuf,nLen);

	return 0;
}

int mcGetip()
{
	char sendbuf[1024];
	int nLen=0;
	char szTime[40];

	getTimeStr(szTime);
	sprintf(sendbuf,"getip 226.1.1.2 4322 192.168.1.11 %s",szTime);
	nLen=strlen(sendbuf);
	sleaf("226.1.1.1",4321,sendbuf,nLen);

	return 0;
}

// mypara
static int gNum=0; //接收到广播包的次数 max:4096
static char gStatus[500];
static char gRcv[1000];
static char gErr[200];

static char serverip[100];
static char servermac[100];
static char servername[100];
static char hhmmssFound[30];
static time_t tFound;

static int port_connect_8=0;
static int status_connect_8=0;

static int nfSearch=0;
// 0: unknown
// 1: searching
// 2: found
// 3: error
// 4: time out

// CNCA name list
char gNameCNCA[1500];
int gnCNC[50];
int gnNumCNC=0;

int rmChar(char *p,int ch)
{
	int len,i;
	len =strlen(p);
	for(i=0;i<len;i++){
		if( (0x0ff & ch) == (0x0ff & p[i]) ) p[i]= 0x20;
	}
	return 0;
}

int getRealName(int *pn,char *pCNCA)
{
	FILE *fp;
	char buf[1000];
	char *p;
	int ret,i,n;
	char sz[1000];
	int num=0;

	memset(pCNCA,0,1500);
	for(i=0;i<50;i++)pn[i]=-1;
	gnNumCNC=0;

	//RunSilent("del","out.txt");
	//RunSilent("setupc.exe","--output out.txt list");
	//Sleep(1000);

	fp=fopen("out.txt","rt");
	if(fp==NULL) return 0;
	for(;;){
		p=fgets(buf,900,fp);
		if(p==NULL ) break;
		rmChar(buf,'=');
		rmChar(buf,',');
		ret=sscanf(buf,"%s%s%s%s%s",sz,sz+100,sz+200,sz+300,sz+400);
		if( ret<5) continue;
		if(0!=memcmp(sz,"CNCA",4))continue;
		sscanf(sz+4,"%d",&n);
		pn[n]=1;

		pCNCA[20*n]=0;
		//for(i=0;i<ret;i++)printf(" %d: %s ",i,sz+i*100);
		//printf("  n:::::%d\n",n);

		if(0==strcmp("RealPortName",sz+300)){
			strcpy(pCNCA+n*20,sz+400);
		}
		else if(0==strcmp("RealPortName",sz+100)){
			strcpy(pCNCA+n*20,sz+200);
		}
		//printf("%s\n",buf);
	}
	fclose(fp);
	for(i=0;i<50;i++){
		if(pn[i]<0)break;
		pn[i]=num;
		num++;
	}
	return num;
}

void disconnectAll()
{
	int i,n;
	char szexe[30],szargv[100];
	char sz[100];

	mylog(" disconnect ALL \n");
	strcpy(szexe,"taskkill");
	sprintf(szargv," /f /t /fi \"IMAGENAME eq h4c*\" /im *");
	RunSilent(szexe,szargv);
	sleep(1);

	status_connect_8=0;
	return;
}

// create hub4com cmd line para file
// *pcom : serial port name \\.\cncbc0  
// *pip : ip addr
// *pfname : conf file name
static void createConf(char *pcom,int port,char *pip,char *pfname)
{
	char p1[]="\n"
		"_BEGIN_\n"
		"  --create-filter=escparse,com,parse\n"
		"  --create-filter=pin2con,com,connect: --connect=dcd\n"
		"  --create-filter=pinmap,com,pinmap:--rts=cts --dtr=dsr\n"
		"  --create-filter=linectl,com,lc:--br=local --lc=local\n"
		"  --add-filters=0:com\n"
		"  --create-filter=telnet,tcp,telnet: --comport=client\n"
		"  --create-filter=pinmap,tcp,pinmap:--rts=cts --dtr=dsr --break=break\n"
		"  --create-filter=linectl,tcp,lc:--br=remote --lc=remote\n"
		"  --add-filters=1:tcp\n"
		"  --octs=off\n";
	//"  \\.\CNCB0\n"
	//"  --use-driver=tcp\n"
	//"  *192.168.1.225:3001\n"
	//"_END_\n"
	//		"\n";
	char p2[]="  --use-driver=tcp\n";
	char p3[]="_END_\n\n";

	FILE *fp;
	fp=fopen(pfname,"wt");
	if(fp==NULL) return;
	fprintf(fp,"%s",p1);
	fprintf(fp,"  %s\n",pcom);
	fprintf(fp,"%s",p2);
	fprintf(fp,"  %s:%d\n",pip,port);
	fprintf(fp,"%s",p3);

	fclose(fp);
	return;
}
void mcGetipThread()
{
	int i;

	mylog(" == mcGetipThread()\n");

	gNum=0;
	for(i=0;i<100;i++){
		if(gNum>0)break;
		mcGetip();
		sleep(6);
	}

	mylog(" == exit mcGetipThread\n");
	_endthread();
	return;
}
//		RunSilent("taskkill"," /f /t /im t.exe");
// disconnect port connected
void disconnect8()
{
	int i,n;
	char szexe[30],szargv[100];
	char sz[100];

	mylog(" disconnect \n");
	strcpy(szexe,"taskkill");
	for(i=0;i<8;i++){
		n=1<<i;
		if( (n & status_connect_8) > 0 ){
			sprintf(szargv," /f /t /im h4c%d.exe",i);
			RunSilent(szexe,szargv);

			sprintf(sz," disconnect port %d\n",i);
			mylog(sz);
			sleep(1);
		}
	}
	status_connect_8=0;
	return;
}
// port connect 
void connect8()
{
	int i,n;
	char szexe[30],szargv[100];
	char portName[20],confName[20];
	int port=3000;
	char sz[100];

	for(i=1;i<8;i++){
		n=1<<i;
		if( (n & port_connect_8) > 0 ){
			sprintf(sz," connect8() ==> connect port:%d\n",i);
			mylog(sz);
			// createConf
			sprintf(portName,"\\\\.\\CNCB%d",i);
			sprintf(confName,"h4c%d.txt",i);
			createConf(portName,port+i,serverip,confName);
			//createConf("\\\\.\\CNCB0",3001,"192.168.1.225","h4c0.txt");
			sprintf(szexe,"h4c%d.exe",i);
			sprintf(szargv,"--load=h4c%d.txt,_BEGIN_,_END_",i);
			RunSilent(szexe,szargv);
		}
	}
	status_connect_8=port_connect_8;
	return;
}
// for test
void connect1()
{
	int i=1,n;
	char szexe[30],szargv[30];

	sprintf(szexe,"h4c%d.exe",i);
	sprintf(szargv,"--load=h4c%d.txt,_BEGIN_,_END_",i);
	RunSilent(szexe,szargv);

	return;
}

static void my_init() {
	char *value;
	char sz[100];

	mylog("    ==== my_init() ====\n");
	value=mg_get_option(ctx,"port_connect_8");
	port_connect_8=getHex(value);

	if( status_connect_8 != 0 ) disconnect8();						// disconnect8

	strcpy(serverip,"unknown");
	strcpy(servermac,"unknown");
	strcpy(servername,"unknown");
	strcpy(hhmmssFound,"unknown");

	disconnectAll();

	port_connect_8 = 255;
	strcpy(sz,serverip);
	strcpy(serverip,"192.168.1.77");
	connect8();
	strcpy(serverip,sz);

	RunSilent("del","out.txt");
	RunSilent("setupc.exe","--output out.txt list");
	//getRealName(gnCNC,gNameCNCA);
}

int rleaf(char *MCASTADDR,int MCASTPORT,char *recvbuf,char *pErr)
{
	DWORD dwInterface,          // Local interface to bind to
		dwMulticastGroup;     // Multicast group to join
	short iPort;                // Port number to use

	struct sockaddr_in  local,		remote,		from;
	SOCKET              sock, sockM;
	int                 len = sizeof(struct sockaddr_in),		optval,		ret;
	DWORD               i=0;
	char rbuf[1000];
	char szquit[200];
	char szip[100];
	char szmac[100];
	char szname[100];
	int nn;

	mylog("  == rleaf()\n");

	dwInterface = INADDR_ANY;
	dwMulticastGroup = inet_addr(MCASTADDR);
	iPort = MCASTPORT;

	// Create the socket. In Winsock 2, you do have to specify the
	// multicast attributes that this socket will be used with.
	//
	if ((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF 
		| WSA_FLAG_MULTIPOINT_D_LEAF 
		| WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		sprintf(pErr,"socket failed with: %d\n", WSAGetLastError());
		//WSACleanup();
		return -1;
	}
	// Bind to the local interface. This is done to receive data.
	local.sin_family = AF_INET;
	local.sin_port   = htons(iPort);
	local.sin_addr.s_addr = dwInterface;

	if (bind(sock, (struct sockaddr *)&local, 
		sizeof(local)) == SOCKET_ERROR)
	{
		sprintf(pErr,"bind failed with: %d\n", WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -2;
	}
	// Set up the SOCKADDR_IN structure describing the multicast 
	// group we want to join
	//
	remote.sin_family      = AF_INET;
	remote.sin_port        = htons(iPort);
	remote.sin_addr.s_addr = dwMulticastGroup;
	//
	// Change the TTL to something more appropriate
	//

	optval = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
		(char *)&optval, sizeof(int)) == SOCKET_ERROR)
	{
		sprintf(pErr,"setsockopt(IP_MULTICAST_TTL) failed: %d\n",
			WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -1;
	}
	// Join the multicast group.  Note that sockM is not used 
	// to send or receive data. It is used when you want to 
	// leave the multicast group. You simply call closesocket() 
	// on it.
	//
	if ((sockM = WSAJoinLeaf(sock, (SOCKADDR *)&remote, 
		sizeof(remote), NULL, NULL, NULL, NULL, 
		JL_RECEIVER_ONLY)) == INVALID_SOCKET)                  // JL_BOTH : SEND AND RCV
	{
		sprintf(pErr,"WSAJoinLeaf() failed: %d\n", WSAGetLastError());
		closesocket(sock);
		//WSACleanup();
		return -1;
	}

	for(;;){
		strcpy(gStatus,"thread rcv 1");
		// Receive data
		if ((ret = recvfrom(sock, rbuf, 500, 0,	(struct sockaddr *)&from, &len)) == SOCKET_ERROR)	{
			sprintf(pErr,"recvfrom failed with: %d\n", 			WSAGetLastError());
			//closesocket(sockM);
			//closesocket(sock);

			//WSACleanup();
			ret = -7;
			break;
		}

		//    WSAETIMEDOUT
		rbuf[ret] = 0;
		memcpy(recvbuf,rbuf,ret);  // copy local to global
		recvbuf[ret]=0;
		//printf("RECV: '%s' from <%s>\n", recvbuf, inet_ntoa(from.sin_addr));

		// Leave the multicast group by closing sock.
		// For nonrooted control and data plane schemes, WSAJoinLeaf
		// returns the same socket handle that you pass into it.
		//

		rbuf[50]=0;
		nn=sscanf(rbuf,"%s",szquit);
		if(nn==1){    // quit test
			if(strcmp(szquit,"quitmongoosetest")==0){
				ret = 0;
				strcpy(gErr,"quit mongoose nomal");
				break;
			}
			else if(strcmp(szquit,"rgetip")==0){      // rcv normal multicast
				nn=sscanf(recvbuf,"%s%s%s%s%s%s%s%s%s",szquit,szquit,szquit,szquit,szquit,szquit,
					szip,szmac,szname);
				if(nn==9){
					gNum++;
					if( 0 != strcmp(serverip,szip) ){
						mylog(" rleaf() ==> connect 8 : new ip\n");
						if( status_connect_8 != 0 ) disconnect8();						// disconnect8

						strcpy(serverip,szip);
						strcpy(servermac,szmac);
						strcpy(servername,szname);
						if( port_connect_8 != 0 ){
							mylog(" rleaf() ==> connect 8 : new connect\n");
							connect8();
						}
					}
				}
			}
		}
	}
	closesocket(sockM);
	closesocket(sock);

	strcpy(gStatus,"thread exit");
	//WSACleanup();
	return ret;
}

void mcRcvThread(void)
{
	int ret;
	gRcv[0]=0;
	gErr[0]=0;
	strcpy(gRcv,"nothing\n");

	mylog(" == mcRcvThread()\n");

	for(;;){
		ret=rleaf("226.1.1.2",4322,gRcv,gErr);
		if(ret<=0){
			strcpy(gErr,"multicast rcv thread rcv nothing\n");
			break;
		}
	}
	mylog(" == exit mcRcvThread()\n");
	_endthread();
	return;
}

#define MAXLEN      2048
// mcPort :4321
// pAddr: 226.1.1.1
// to : timeout 1000    if(to==0) timeout not set
int multicast_rcv(char *pAddr,int mcPort,int to,char *pRcv,char *pErr)
{
	char pLocal[100];
	//int i;
	SOCKET          s;
	SOCKADDR_IN     mcAddr;
	int             nMcLen;
	char            buf[MAXLEN];      
	struct ip_mreq  ipmr;
	SOCKADDR_IN     localAddr;
	//WSADATA         wsaData;
	int             nErr;      

	sprintf(pErr," ok ");
	pRcv[0]=0;
	strcpy(pLocal,"192.168.1.11");
#if 0	
	if(WSAStartup(0x0202, &wsaData) != 0) 	{
		//ReportErr("WSAStartup(..)");
		return -1;
	};
#endif
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s == INVALID_SOCKET)        	{
		//WSAReportErr("socket(...)");
		closesocket(s);
		//WSACleanup();
		sprintf(pErr," create socket error ");
		return -2;
	} 
	////////////////////////////////////////// timeout
	if(to>0){
		int ntimeout=to;
		unsigned int nnt=sizeof(ntimeout);
		setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&ntimeout,nnt);
	}

	memset(&localAddr, 0, sizeof(localAddr));      
	localAddr.sin_family        = AF_INET;
	localAddr.sin_addr.s_addr   = htonl(INADDR_ANY)/*inet_addr(MC_ADDR)*/;
	//localAddr.sin_addr.s_addr   = inet_addr(pLocal);
	localAddr.sin_port          = htons(mcPort);     
	nErr = bind(s, (struct sockaddr*)&localAddr, sizeof(localAddr));
	if(nErr == SOCKET_ERROR) 	{
		//WSAReportErr("bind(...)");
		closesocket(s);
		sprintf(pErr," bind error ");
		//WSACleanup();
		return -3;
	}
	/* Join multicast group */
	ipmr.imr_multiaddr.s_addr  = inet_addr(pAddr);
	ipmr.imr_interface.s_addr  = htonl(INADDR_ANY);
	//ipmr.imr_interface.s_addr  = inet_addr(pLocal);
	nErr = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipmr, sizeof(ipmr));
	if(nErr == SOCKET_ERROR) 	{
		//WSAReportErr("setsockopt(.. IP_ADD_MEMBERSHIP ..)");
		closesocket(s);
		sprintf(pErr," join error %d ",nErr);
		//WSACleanup();
		return -4;
	}
	mcAddr.sin_family       = AF_INET;
	mcAddr.sin_addr.s_addr  = inet_addr(pAddr);
	mcAddr.sin_port         = htons(mcPort);
	nMcLen = sizeof(mcAddr);  
	//for(i=0;i<5;i++)	{
	//sleep(2);
	nErr = recvfrom(s, buf, MAXLEN, 0, (struct sockaddr*)&mcAddr, &nMcLen);
	if(nErr == SOCKET_ERROR)
	{
		printf("error or timeout :%d\n",nErr);
		//continue;
	}
	else {
		if(nErr>=0)buf[nErr]=0;
		printf("%s\n", buf);
	}
	//}
	//WSACleanup();

	return 0;
}


static const char *html_form =
	"<html><body>POST example."
	"<form method=\"POST\" action=\"/handle_post_request\">"
	"Input 1: <input type=\"text\" name=\"input_1\" /> <br/>"
	"Input 2: <input type=\"text\" name=\"input_2\" /> <br/>"
	"<input type=\"submit\" />"
	"</form></body></html>";
static const char *html_form_run =
	"<html><body>run silent"
	"<form method=\"POST\" action=\"/handle_post_request_run\">"
	"exe : <input type=\"text\" name=\"input_1\" /> <br/>"
	"argv: <input type=\"text\" name=\"input_2\" /> <br/>"
	"<input type=\"submit\" value=\"run\" />"
	"</form></body></html>";
static const char *changeip_form =
	"<html><body><h3>修改串口服务器ip地址</h3>"
	"<form method=\"POST\" action=\"/changeip_done\">";
static const char *changeip_form1 =
	"新ip地址: <input type=\"text\" name=\"input_1\" /> <br/>"
	"子网掩码: <input type=\"text\" name=\"input_2\" /> <br/>"
	"<input type=\"submit\" value=\"提交\" />"
	"</form></body></html>";

static const char *changename_form =
	"<html><body><h3>修改串口设备名称</h3>"
	"<form method=\"POST\" action=\"/changename_done\">"
	"旧名称: ";
static const char *changename_form1 =
	"新名称: <input type=\"text\" name=\"input_2\" /> <br/>"
	"<input type=\"submit\" value=\"提交\" />"
	"</form></body></html>";

static const char *html_multi_rcv =
	"<html><body>start multicast rcv"
	"</body></html>";
static const char *html_multi_rcv_error =
	"<html><body>start multicast rcv error"
	"</body></html>";

static const char *html_header =
	"<html><body>";
static const char *html_end =
	"</body></html>";

/////////////// cyx
static int begin_request_handler(struct mg_connection *conn) {
	const struct mg_request_info *ri = mg_get_request_info(conn);
	char post_data[1024], input1[sizeof(post_data)], input2[sizeof(post_data)];
	int post_data_len;
	char buf[2048];
	char buf1[2048];
	int i;
	time_t t;

	if (!strcmp(ri->uri, "/handle_post_request")) {
		// User has submitted a form, show submitted data and a variable value
		post_data_len = mg_read(conn, post_data, sizeof(post_data));

		// Parse form data. input1 and input2 are guaranteed to be NUL-terminated
		mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
		mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"Submitted data: [%.*s]\n"
			"Submitted data length: %d bytes\n"
			"input_1: [%s]\n"
			"input_2: [%s]\n",
			post_data_len, post_data, post_data_len, input1, input2);
		return 1;
	} //else {
	if (!strcmp(ri->uri, "/changename_done")) { //          
		time(&t);
		// User has submitted a form, show submitted data and a variable value
		post_data_len = mg_read(conn, post_data, sizeof(post_data));

		// Parse form data. input1 and input2 are guaranteed to be NUL-terminated
		mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
		mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));

		sprintf(buf1,"change CNCA%s PortName=COM#,RealPortName=%s",input1,input2);
		RunSilent("setupc.exe",buf1);
		Sleep(2000);
		RunSilent("del","out.txt");
		RunSilent("setupc.exe","--output out.txt list");
		Sleep(2000);

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"<a href=\"/?t=%d\">返回首页</a>\n",t);
		//"%s %s %s<a href=\"/?t=%d\">home</a>\n",input1,input2,buf1,t);
		return 1;
	}
	if (!memcmp(ri->uri, "/changename",11)) { //                       change port name
		sprintf(buf1,"<input type=\"hidden\" name=\"input_1\" value=\"%s\" /><br />",ri->uri+11);
		sprintf(buf,"%s%s<br />%s%s",
			changename_form,gNameCNCA+20*atoi(ri->uri+11),buf1,changename_form1);

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);
		return 1;
	} //else {
	if (!strcmp(ri->uri, "/changeip_done")) { //          
		time(&t);
		// User has submitted a form, show submitted data and a variable value
		post_data_len = mg_read(conn, post_data, sizeof(post_data));

		// Parse form data. input1 and input2 are guaranteed to be NUL-terminated
		mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
		mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));

		mcChangeip(input1,input2);
		Sleep(1000);

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"串口服务器重新启动,大约需要30秒......<br /><a href=\"/?t=%d\">返回首页</a>\n",t);
		//"%s %s %s<a href=\"/?t=%d\">home</a>\n",input1,input2,buf1,t);
		return 1;
	}
	if (!strcmp(ri->uri, "/changeip")) { //                       change port name
		sprintf(buf1,"原ip地址: %s<br />",serverip);
		sprintf(buf,"%s%s<br />%s",
			changeip_form,buf1,changeip_form1);

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);
		return 1;
	} //else {
	if (!strcmp(ri->uri, "/handle_post_request_run")) { //                        <== runsilent.html
		// User has submitted a form, show submitted data and a variable value
		post_data_len = mg_read(conn, post_data, sizeof(post_data));

		// Parse form data. input1 and input2 are guaranteed to be NUL-terminated
		mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1));
		mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2));

		RunSilent(input1,input2);

		// Send reply to the client, showing submitted form values.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"Submitted data: [%.*s]\n"
			"Submitted data length: %d bytes\n"
			"input_1: [%s]\n"
			"input_2: [%s]\n",
			post_data_len, post_data, post_data_len, input1, input2);
		return 1;
	} //else {
	else if (!strcmp(ri->uri, "/index.html")) {
		// Show HTML form.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(html_form), html_form);
		return 1;
	}
	else if (!strcmp(ri->uri, "/") || !strcmp(ri->uri, "/index")) {
		time(&t);
		buf[0]=0;
		if(gNum>0){
			sprintf(buf1,"<table border=\"1\"><caption>网络串口服务器</caption>");
			strcat(buf,buf1);
			sprintf(buf1,"<tr align=\"center\"><td>IP地址:</td><td>%s <a href=\"/changeip?t=%d\">修改</a></td></tr>",serverip,t);
			strcat(buf,buf1);
			sprintf(buf1,"<tr align=\"center\"><td>MAC地址:</td><td>%s</td></tr></table>",servermac);
			strcat(buf,buf1);
		}
		gnNumCNC=getRealName(gnCNC,gNameCNCA);
		if(gnNumCNC==8){
			sprintf(buf1,"<table border=\"1\"><tr><td></td><td>本机串口设备名</td><td>网络串口名</td></tr>");
			strcat(buf,buf1);
			for(i=0;i<gnNumCNC;i++){
				sprintf(buf1,"<tr align=\"center\"><td>%d</td><td>%s <a href=\"/changename%d?t=%d\">改名</a></td><td>网络串口%d</td></tr>",i+1,gNameCNCA+20*gnCNC[i],i,t,i+1);
				strcat(buf,buf1);
			}
			sprintf(buf1,"</table>");
			strcat(buf,buf1);
		}
		else{
			sprintf(buf,"<table><tr><td>未安装驱动程序</td></tr></table>");/////////// 
		}
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);
		return 1;
	}
	else if (!strcmp(ri->uri, "/runsilent.html")) { // runsilent.html => html_form_run => handle_post_request_run 
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(html_form_run), html_form_run);
		return 1;
	}
	else if (!strcmp(ri->uri, "/statustest.html")) {
		sprintf(buf1,"%s status: %s  == num:%d %s",html_header,gStatus,gNum,html_end);
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf1), buf1);
		return 1;
	}
	else if (!strcmp(ri->uri, "/testmc.html")) {
		//ret=multicast_rcv("226.1.1.1",4321,0,mcRcv,mcErr);
		//ret = rleaf("226.1.1.1",4321,mcRcv,mcErr);
		sprintf(buf,"%s ret str: %s %s num:%d %s",html_header,gRcv,gErr,gNum,html_end);
		// Show HTML form.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);
		return 1;
	}
	else if (!strcmp(ri->uri, "/test001.html")) {// multicast rcv str: ip and mac, time , count
		//ret=multicast_rcv("226.1.1.1",4321,0,mcRcv,mcErr);
		//ret = rleaf("226.1.1.1",4321,mcRcv,mcErr);
		sprintf(buf,"ip: %s, mac: %s, hostname:%s %s, num: %d",serverip,servermac,servername,hhmmssFound,gNum);
		mylog("test001.html\n");
		// Show HTML form.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);

		createConf("\\\\.\\CNCB0",3001,"192.168.1.225","h4c0.txt");
		return 1;
	}
	else if (!strcmp(ri->uri, "/testsilent.html")) {// multicast rcv str: ip and mac, time , count
		//RunSilent("t.exe","mongoose");
		connect1();
		sprintf(buf,"ip: %s, mac: %s, hostname:%s %s, num: %d, port_connect_8:%d",
			serverip,servermac,servername,hhmmssFound,gNum,port_connect_8);
		// Show HTML form.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);

		return 1;
	}
	else if (!strcmp(ri->uri, "/testkill.html")) {// multicast rcv str: ip and mac, time , count
		RunSilent("taskkill"," /f /t /im t.exe");
		sprintf(buf,"ip: %s, mac: %s, hostname:%s %s, num: %d",serverip,servermac,servername,hhmmssFound,gNum);
		// Show HTML form.
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);

		return 1;
	}
	else if (!strcmp(ri->uri, "/urlsearchserver.html")) {
		nfSearch=1; // searching
		mcGetip();
		//sprintf(buf,"%s ret str: %s %s num:%d %s",html_header,gRcv,gErr,gNum,html_end);
		sprintf(buf,"searching");
		mg_printf(conn, "HTTP/1.0 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n%s",
			(int) strlen(buf), buf);
		return 1;
	}
	else return 0;
	//return 1;  // Mark request as processed
}
 
static void start_mongoose(int argc, char *argv[]) {
	char *options[MAX_OPTIONS];
	int i;
	//char pRcv[1024],pErr[200];

	// Edit passwords file if -A option is specified
	if (argc > 1 && !strcmp(argv[1], "-A")) {
		if (argc != 6) {
			show_usage_and_exit();
		}
		exit(mg_modify_passwords_file(argv[2], argv[3], argv[4], argv[5]) ?
EXIT_SUCCESS : EXIT_FAILURE);
	}

	// Show usage if -h or --help options are specified
	if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		show_usage_and_exit();
	}

	/* Update config based on command line arguments */
	process_command_line_arguments(argv, options);

	/* Setup signal handler: quit on Ctrl-C */
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	/* Start Mongoose */
	ctx = mg_start(&mongoose_callback, NULL, (const char **) options);
	callbacks.begin_request = begin_request_handler;

	//multicast_rcv("226.1.1.1",4321,0,pRcv,pErr);

	for (i = 0; options[i] != NULL; i++) {
		free(options[i]);
	}

	if (ctx == NULL) {
		die("%s", "Failed to start Mongoose.");
	}

	mylog(" == start_mongoose() : before my_init,mcRcv,mcGetip\n");
	my_init();
	_beginthread((void (__cdecl *)(void *))mcRcvThread,0,0);;         // cyx
	_beginthread((void (__cdecl *)(void *))mcGetipThread,0,0);;         // cyx
	// using thread !!
	//for(i=0;i<5;i++){
	//	sleep(1000);
	//	if(gNum<1){
	//		mcGetip();
	//		sleep(2000);
	//	}
	//}
}

#ifdef _WIN32
enum {
	ID_ICON = 100, ID_QUIT, ID_SETTINGS, ID_SEPARATOR, ID_INSTALL_SERVICE,
	ID_REMOVE_SERVICE, ID_STATIC, ID_GROUP, ID_SAVE, ID_RESET_DEFAULTS,
	ID_STATUS, ID_CONNECT,

	// All dynamically created text boxes for options have IDs starting from
	// ID_CONTROLS, incremented by one.
	ID_CONTROLS = 200,

	// Text boxes for files have "..." buttons to open file browser. These
	// buttons have IDs that are ID_FILE_BUTTONS_DELTA higher than associated
	// text box ID.
	ID_FILE_BUTTONS_DELTA = 1000
};
static HICON hIcon;
static SERVICE_STATUS ss;
static SERVICE_STATUS_HANDLE hStatus;
static const char *service_magic_argument = "--";
static NOTIFYICONDATA TrayIcon;

static void WINAPI ControlHandler(DWORD code) {
	if (code == SERVICE_CONTROL_STOP || code == SERVICE_CONTROL_SHUTDOWN) {
		ss.dwWin32ExitCode = 0;
		ss.dwCurrentState = SERVICE_STOPPED;
	}
	SetServiceStatus(hStatus, &ss);
}

static void WINAPI ServiceMain(void) {
	ss.dwServiceType = SERVICE_WIN32;
	ss.dwCurrentState = SERVICE_RUNNING;
	ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	hStatus = RegisterServiceCtrlHandler(server_name, ControlHandler);
	SetServiceStatus(hStatus, &ss);

	while (ss.dwCurrentState == SERVICE_RUNNING) {
		Sleep(1000);
	}
	mg_stop(ctx);

	ss.dwCurrentState = SERVICE_STOPPED;
	ss.dwWin32ExitCode = (DWORD) -1;
	SetServiceStatus(hStatus, &ss);
}


static void show_error(void) {
	char buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL);
	MessageBox(NULL, buf, "Error", MB_OK);
}

static void *align(void *ptr, DWORD alig) {
	ULONG ul = (ULONG) ptr;
	ul += alig;
	ul &= ~alig;
	return ((void *) ul);
}

static int is_boolean_option(const char *option_name) {
	return !strcmp(option_name, "enable_directory_listing") ||
		!strcmp(option_name, "enable_keep_alive");
}

static int is_filename_option(const char *option_name) {
	return !strcmp(option_name, "cgi_interpreter") ||
		!strcmp(option_name, "global_auth_file") ||
		!strcmp(option_name, "put_delete_auth_file") ||
		!strcmp(option_name, "access_log_file") ||
		!strcmp(option_name, "error_log_file") ||
		!strcmp(option_name, "ssl_certificate");
}

static int is_directory_option(const char *option_name) {
	return !strcmp(option_name, "document_root");
}

static int is_numeric_options(const char *option_name) {
	return !strcmp(option_name, "num_threads");
}

static void my_save_config() {
	FILE *fp;
	char *value;
	const char **options, *name, *default_value;
	int i;

	mylog("   === my_save_config() ===\n");

	fp = fopen("tmp.txt","wt");
	fprintf(fp, "%s", config_file_top_comment);
	options = mg_get_valid_option_names();
	for (i = 0; options[i] != NULL; i += 3) {
		name = options[i + 1];
		value=mg_get_option(ctx,name);
		default_value = options[i + 2] == NULL ? "" : options[i + 2];
		// If value is the same as default, skip it
		if (strcmp(value, default_value) != 0) {
			fprintf(fp, "%s %s\n", name, value);
		}
	}
	fclose(fp);
}

static void save_config(HWND hDlg, FILE *fp) {
	char value[2000];
	const char **options, *name, *default_value;
	int i, id;

	mylog("     === save_config() === \n");

	fprintf(fp, "%s", config_file_top_comment);
	options = mg_get_valid_option_names();
	for (i = 0; options[i] != NULL; i += 3) {
		name = options[i + 1];
		id = ID_CONTROLS + i / 3;
		if (is_boolean_option(name)) {
			snprintf(value, sizeof(value), "%s",
				IsDlgButtonChecked(hDlg, id) ? "yes" : "no");
		} else {
			GetDlgItemText(hDlg, id, value, sizeof(value));
		}
		default_value = options[i + 2] == NULL ? "" : options[i + 2];
		// If value is the same as default, skip it
		if (strcmp(value, default_value) != 0) {
			fprintf(fp, "%s %s\n", name, value);
		}
	}
	my_save_config();
}

static BOOL CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lP) {
	FILE *fp;
	int i;
	const char *name, *value, **options = mg_get_valid_option_names();

	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(hDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_SAVE:
			EnableWindow(GetDlgItem(hDlg, ID_SAVE), FALSE);
			if ((fp = fopen(config_file, "w+")) != NULL) {
				save_config(hDlg, fp);
				fclose(fp);
				mg_stop(ctx);
				start_mongoose(__argc, __argv);
			}
			EnableWindow(GetDlgItem(hDlg, ID_SAVE), TRUE);
			break;
		case ID_RESET_DEFAULTS:
			for (i = 0; options[i] != NULL; i += 3) {
				name = options[i + 1];
				value = options[i + 2] == NULL ? "" : options[i + 2];
				if (is_boolean_option(name)) {
					CheckDlgButton(hDlg, ID_CONTROLS + i / 3, !strcmp(value, "yes") ?
BST_CHECKED : BST_UNCHECKED);
				} else {
					SetWindowText(GetDlgItem(hDlg, ID_CONTROLS + i / 3), value);
				}
			}
			break;
		}

		for (i = 0; options[i] != NULL; i += 3) {
			name = options[i + 1];
			if ((is_filename_option(name) || is_directory_option(name)) &&
				LOWORD(wParam) == ID_CONTROLS + i / 3 + ID_FILE_BUTTONS_DELTA) {
					OPENFILENAME of;
					BROWSEINFO bi;
					char path[PATH_MAX] = "";

					memset(&of, 0, sizeof(of));
					of.lStructSize = sizeof(of);
					of.hwndOwner = (HWND) hDlg;
					of.lpstrFile = path;
					of.nMaxFile = sizeof(path);
					of.lpstrInitialDir = mg_get_option(ctx, "document_root");
					of.Flags = OFN_CREATEPROMPT | OFN_NOCHANGEDIR;

					memset(&bi, 0, sizeof(bi));
					bi.hwndOwner = (HWND) hDlg;
					bi.lpszTitle = "Choose WWW root directory:";
					bi.ulFlags = BIF_RETURNONLYFSDIRS;

					if (is_directory_option(name)) {
						SHGetPathFromIDList(SHBrowseForFolder(&bi), path);
					} else {
						GetOpenFileName(&of);
					}

					if (path[0] != '\0') {
						SetWindowText(GetDlgItem(hDlg, ID_CONTROLS + i / 3), path);
					}
			}
		}

		break;

	case WM_INITDIALOG:
		SendMessage(hDlg, WM_SETICON,(WPARAM) ICON_SMALL, (LPARAM) hIcon);
		SendMessage(hDlg, WM_SETICON,(WPARAM) ICON_BIG, (LPARAM) hIcon);
		SetWindowText(hDlg, "Mongoose settings");
		SetFocus(GetDlgItem(hDlg, ID_SAVE));
		for (i = 0; options[i] != NULL; i += 3) {
			name = options[i + 1];
			value = mg_get_option(ctx, name);
			if (is_boolean_option(name)) {
				CheckDlgButton(hDlg, ID_CONTROLS + i / 3, !strcmp(value, "yes") ?
BST_CHECKED : BST_UNCHECKED);
			} else {
				SetDlgItemText(hDlg, ID_CONTROLS + i / 3, value == NULL ? "" : value);
			}
		}
		break;
	default:
		break;
	}

	return FALSE;
}

static void add_control(unsigned char **mem, DLGTEMPLATE *dia, WORD type,
	DWORD id, DWORD style, WORD x, WORD y,
	WORD cx, WORD cy, const char *caption) {
		DLGITEMTEMPLATE *tp;
		LPWORD p;

		dia->cdit++;

		*mem = align(*mem, 3);
		tp = (DLGITEMTEMPLATE *) *mem;

		tp->id = (WORD)id;
		tp->style = style;
		tp->dwExtendedStyle = 0;
		tp->x = x;
		tp->y = y;
		tp->cx = cx;
		tp->cy = cy;

		p = align(*mem + sizeof(*tp), 1);
		*p++ = 0xffff;
		*p++ = type;

		while (*caption != '\0') {
			*p++ = (WCHAR) *caption++;
		}
		*p++ = 0;
		p = align(p, 1);

		*p++ = 0;
		*mem = (unsigned char *) p;
}

static void show_settings_dialog() {
#define HEIGHT 15
#define WIDTH 400
#define LABEL_WIDTH 80

	unsigned char mem[4096], *p;
	const char **option_names, *long_option_name;
	DWORD style;
	DLGTEMPLATE *dia = (DLGTEMPLATE *) mem;
	WORD i, cl, x, y, width, nelems = 0;
	static int guard;

	static struct {
		DLGTEMPLATE template; // 18 bytes
		WORD menu, class;
		wchar_t caption[1];
		WORD fontsiz;
		wchar_t fontface[7];
	} dialog_header = {{WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_VISIBLE |
		DS_SETFONT | WS_DLGFRAME, WS_EX_TOOLWINDOW, 0, 200, 200, WIDTH, 0},
		0, 0, L"", 8, L"Tahoma"};

	if (guard == 0) {
		guard++;
	} else {
		return;
	}

	(void) memset(mem, 0, sizeof(mem));
	(void) memcpy(mem, &dialog_header, sizeof(dialog_header));
	p = mem + sizeof(dialog_header);

	option_names = mg_get_valid_option_names();
	for (i = 0; option_names[i] != NULL; i += 3) {
		long_option_name = option_names[i + 1];
		style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
		x = 10 + (WIDTH / 2) * (nelems % 2);
		y = (nelems/2 + 1) * HEIGHT + 5;
		width = WIDTH / 2 - 20 - LABEL_WIDTH;
		if (is_numeric_options(long_option_name)) {
			style |= ES_NUMBER;
			cl = 0x81;
			style |= WS_BORDER | ES_AUTOHSCROLL;
		} else if (is_boolean_option(long_option_name)) {
			cl = 0x80;
			style |= BS_AUTOCHECKBOX;
		} else if (is_filename_option(long_option_name) ||
			is_directory_option(long_option_name)) {
				style |= WS_BORDER | ES_AUTOHSCROLL;
				width -= 20;
				cl = 0x81;
				add_control(&p, dia, 0x80,
					ID_CONTROLS + (i / 3) + ID_FILE_BUTTONS_DELTA,
					WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
					(WORD) (x + width + LABEL_WIDTH + 5),
					y, 15, 12, "...");
		} else {
			cl = 0x81;
			style |= WS_BORDER | ES_AUTOHSCROLL;
		}
		add_control(&p, dia, 0x82, ID_STATIC, WS_VISIBLE | WS_CHILD,
			x, y, LABEL_WIDTH, HEIGHT, long_option_name);
		add_control(&p, dia, cl, ID_CONTROLS + (i / 3), style,
			(WORD) (x + LABEL_WIDTH), y, width, 12, "");
		nelems++;
	}

	y = (WORD) (((nelems + 1) / 2 + 1) * HEIGHT + 5);
	add_control(&p, dia, 0x80, ID_GROUP, WS_CHILD | WS_VISIBLE |
		BS_GROUPBOX, 5, 5, WIDTH - 10, y, " Settings ");
	y += 10;
	add_control(&p, dia, 0x80, ID_SAVE,
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		WIDTH - 70, y, 65, 12, "Save Settings");
	add_control(&p, dia, 0x80, ID_RESET_DEFAULTS,
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		WIDTH - 140, y, 65, 12, "Reset to defaults");
	add_control(&p, dia, 0x82, ID_STATIC,
		WS_CHILD | WS_VISIBLE | WS_DISABLED,
		5, y, 180, 12, server_name);

	dia->cy = ((nelems + 1) / 2 + 1) * HEIGHT + 30;
	DialogBoxIndirectParam(NULL, dia, NULL, DlgProc, (LPARAM) NULL);
	guard--;
}

static int manage_service(int action) {
	static const char *service_name = "Mongoose";
	SC_HANDLE hSCM = NULL, hService = NULL;
	SERVICE_DESCRIPTION descr = {server_name};
	char path[PATH_MAX + 20];  // Path to executable plus magic argument
	int success = 1;

	if ((hSCM = OpenSCManager(NULL, NULL, action == ID_INSTALL_SERVICE ?
GENERIC_WRITE : GENERIC_READ)) == NULL) {
	success = 0;
	show_error();
	} else if (action == ID_INSTALL_SERVICE) {
		GetModuleFileName(NULL, path, sizeof(path));
		strncat(path, " ", sizeof(path));
		strncat(path, service_magic_argument, sizeof(path));
		hService = CreateService(hSCM, service_name, service_name,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
			path, NULL, NULL, NULL, NULL, NULL);
		if (hService) {
			ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &descr);
		} else {
			show_error();
		}
	} else if (action == ID_REMOVE_SERVICE) {
		if ((hService = OpenService(hSCM, service_name, DELETE)) == NULL ||
			!DeleteService(hService)) {
				show_error();
		}
	} else if ((hService = OpenService(hSCM, service_name,
		SERVICE_QUERY_STATUS)) == NULL) {
			success = 0;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);

	return success;
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam) {
		static SERVICE_TABLE_ENTRY service_table[] = {
			{server_name, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
			{NULL, NULL}
		};
		int service_installed;
		char buf[200], *service_argv[] = {__argv[0], NULL};
		POINT pt;
		HMENU hMenu;
		char http8080[1024];
		time_t t;

		switch (msg) {
		case WM_CREATE:
			if (__argv[1] != NULL &&
				!strcmp(__argv[1], service_magic_argument)) {
					start_mongoose(1, service_argv);
					StartServiceCtrlDispatcher(service_table);
					exit(EXIT_SUCCESS);
			} else {
				start_mongoose(__argc, __argv);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case ID_QUIT:
				disconnectAll();
				mg_stop(ctx);
				Shell_NotifyIcon(NIM_DELETE, &TrayIcon);
				PostQuitMessage(0);
				return 0;
			case ID_SETTINGS:
				show_settings_dialog();
				break;
			case ID_INSTALL_SERVICE:
			case ID_REMOVE_SERVICE:
				manage_service(LOWORD(wParam));
				break;
			case ID_CONNECT:
				time(&t);
				printf("[%s]\n", get_url_to_first_open_port(ctx));
				sprintf(http8080,"%s?t=%d",get_url_to_first_open_port(ctx),t);
				//ShellExecute(NULL, "open", get_url_to_first_open_port(ctx),
				ShellExecute(NULL, "open", http8080,
					NULL, NULL, SW_SHOW);
				break;
			}
			break;
		case WM_USER:
			switch (lParam) {
			case WM_RBUTTONUP:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
				hMenu = CreatePopupMenu();
				//AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_SEPARATOR, server_name);
				//AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");
				service_installed = manage_service(0);
				snprintf(buf, sizeof(buf), "NT service: %s installed",
					service_installed ? "" : "not");
				//AppendMenu(hMenu, MF_STRING | MF_GRAYED, ID_SEPARATOR, buf);
				//AppendMenu(hMenu, MF_STRING | (service_installed ? MF_GRAYED : 0),
				//	ID_INSTALL_SERVICE, "Install service");
				//AppendMenu(hMenu, MF_STRING | (!service_installed ? MF_GRAYED : 0),
				//	ID_REMOVE_SERVICE, "Deinstall service");
				//AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");
				//AppendMenu(hMenu, MF_STRING, ID_CONNECT, "Start browser");
				AppendMenu(hMenu, MF_STRING, ID_CONNECT, "打开");
				//AppendMenu(hMenu, MF_STRING, ID_SETTINGS, "Edit Settings");
				AppendMenu(hMenu, MF_SEPARATOR, ID_SEPARATOR, "");
				//AppendMenu(hMenu, MF_STRING, ID_QUIT, "Exit");
				AppendMenu(hMenu, MF_STRING, ID_QUIT, "退出");
				GetCursorPos(&pt);
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hWnd, NULL);
				PostMessage(hWnd, WM_NULL, 0, 0);
				DestroyMenu(hMenu);
				break;
			}
			break;
		case WM_CLOSE:
			mg_stop(ctx);
			Shell_NotifyIcon(NIM_DELETE, &TrayIcon);
			PostQuitMessage(0);
			return 0;  // We've just sent our own quit message, with proper hwnd.
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdline, int show) {
	WNDCLASS cls;
	HWND hWnd;
	MSG msg;

	init_server_name();
	memset(&cls, 0, sizeof(cls));
	cls.lpfnWndProc = (WNDPROC) WindowProc;
	cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	cls.lpszClassName = server_name;

	RegisterClass(&cls);
	hWnd = CreateWindow(cls.lpszClassName, server_name, WS_OVERLAPPEDWINDOW,
		0, 0, 0, 0, NULL, NULL, NULL, NULL);
	ShowWindow(hWnd, SW_HIDE);

	TrayIcon.cbSize = sizeof(TrayIcon);
	TrayIcon.uID = ID_ICON;
	TrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	TrayIcon.hIcon = hIcon = LoadImage(GetModuleHandle(NULL),
		MAKEINTRESOURCE(ID_ICON),
		IMAGE_ICON, 16, 16, 0);
	TrayIcon.hWnd = hWnd;
	snprintf(TrayIcon.szTip, sizeof(TrayIcon.szTip), "%s", server_name);
	TrayIcon.uCallbackMessage = WM_USER;
	Shell_NotifyIcon(NIM_ADD, &TrayIcon);

	while (GetMessage(&msg, hWnd, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Return the WM_QUIT value.
	return msg.wParam;
}
#elif defined(USE_COCOA)
#import <Cocoa/Cocoa.h>

@interface Mongoose : NSObject<NSApplicationDelegate>
	- (void) openBrowser;
- (void) shutDown;
@end

	@implementation Mongoose
	- (void) openBrowser {
		[[NSWorkspace sharedWorkspace]
openURL:[NSURL URLWithString:
		[NSString stringWithUTF8String:"http://www.yahoo.com"]]];
}
- (void) editConfig {
	create_config_file(config_file);
	[[NSWorkspace sharedWorkspace]
openFile:[NSString stringWithUTF8String:config_file]
withApplication:@"TextEdit"];
}
- (void)shutDown{
	[NSApp terminate:nil];
}
@end

	int main(int argc, char *argv[]) {
		init_server_name();
		start_mongoose(argc, argv);

		[NSAutoreleasePool new];
		[NSApplication sharedApplication];

		// Add delegate to process menu item actions
		Mongoose *myDelegate = [[Mongoose alloc] autorelease];
		[NSApp setDelegate: myDelegate];

		// Run this app as agent
		ProcessSerialNumber psn = { 0, kCurrentProcess };
		TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
		SetFrontProcess(&psn);

		// Add status bar menu
		id menu = [[NSMenu new] autorelease];

		// Add version menu item
		[menu addItem:[[[NSMenuItem alloc]
		//initWithTitle:[NSString stringWithFormat:@"%s", server_name]
initWithTitle:[NSString stringWithUTF8String:server_name]
action:@selector(noexist) keyEquivalent:@""] autorelease]];

		// Add configuration menu item
		[menu addItem:[[[NSMenuItem alloc]
initWithTitle:@"Edit configuration"
action:@selector(editConfig) keyEquivalent:@""] autorelease]];

		// Add connect menu item
		[menu addItem:[[[NSMenuItem alloc]
initWithTitle:@"Open web root in a browser"
action:@selector(openBrowser) keyEquivalent:@""] autorelease]];

		// Separator
		[menu addItem:[NSMenuItem separatorItem]];

		// Add quit menu item
		[menu addItem:[[[NSMenuItem alloc]
initWithTitle:@"Quit"
action:@selector(shutDown) keyEquivalent:@"q"] autorelease]];

		// Attach menu to the status bar
		id item = [[[NSStatusBar systemStatusBar]
statusItemWithLength:NSVariableStatusItemLength] retain];
		[item setHighlightMode:YES];
		[item setImage:[NSImage imageNamed:@"mongoose_22x22.png"]];
		[item setMenu:menu];

		// Run the app
		[NSApp activateIgnoringOtherApps:YES];
		[NSApp run];

		mg_stop(ctx);

		return EXIT_SUCCESS;
}
#else
int main(int argc, char *argv[]) {
	init_server_name();
	start_mongoose(argc, argv);
	printf("%s started on port(s) %s with web root [%s]\n",
		server_name, mg_get_option(ctx, "listening_ports"),
		mg_get_option(ctx, "document_root"));
	while (exit_flag == 0) {
		sleep(1);
	}
	printf("Exiting on signal %d, waiting for all threads to finish...",
		exit_flag);
	fflush(stdout);
	mg_stop(ctx);
	printf("%s", " done.\n");

	return EXIT_SUCCESS;
}
#endif /* _WIN32 */
