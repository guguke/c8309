#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>


main()
{
	char i[400];
	int j;
	for(j=0;j<10;j++){
		scanf("%s",i);
		printf(" %d hello %s\n",j,i);
		if(0==strcmp(i,"quit")) break;
	}
	Sleep(5000);
	return 0;
}
