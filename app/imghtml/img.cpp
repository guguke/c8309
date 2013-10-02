// img.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "string.h"
#include <atlbase.h>
#include <atlconv.h>
#include <direct.h>

int _tmain(int argc, _TCHAR* argv[])
{
	USES_CONVERSION;
	char sz0[]=
"<!DOCTYPE html>\n"
"<html>\n"
"<body>\n"
"<h2>Norwegian Mountain Trip</h2>\n";
//"<img border=\"0\" src=";
//"http://img.lelertys.com/tupian/2010/49/1.jpg"
//	alt="Pulpit rock" >
//</body>
//</html>
	char sz1[]=
//"alt=\"Pulpit rock\" >\n"
"</body>\n"
"</html>\n";
	FILE *fp;
	char filename[100];
	char http[300];
	char cwd[300];
	int zero;
	int i=11,n;

	getcwd(cwd,200);
	printf("current work dir : %s \n",cwd);

	printf("\nhttp (http://...../../) : ");
	scanf("%s",http);
	printf("\n filename : ");
	scanf("%s",filename);
	printf("\n num : ");
	scanf("%d",&n);
	printf("format %%d (0,1,2,3  02d 03d) : ");
	scanf("%d",&zero);

	fp=fopen(filename,"wt");
	if(fp==NULL){
		printf("\n file open error\n");
		return -1;
	}

	fprintf(fp,"%s",sz0);
	for(i=0;i<n;i++){
		switch(zero){
		case 2:
			fprintf(fp,"<img src=\"%s%02d.jpg\" >\n",http,i+1);
			break;
		case 3:
			fprintf(fp,"<img src=\"%s%03d.jpg\" >\n",http,i+1);
			break;
		default:
			fprintf(fp,"<img src=\"%s%d.jpg\" >\n",http,i+1);
			break;
		}
	}
	fprintf(fp,"%s",sz1);

	fclose(fp);
	printf("\n save file as : %s\n",filename);
	return 0;
}

