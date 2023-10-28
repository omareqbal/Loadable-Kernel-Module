#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
int main(){
	int fd,val,i,n,val2,objtype;
	fd = open("/proc/partb_1",O_RDWR);
	if(fd<0){
		perror("open error");
		return 1;
	}

	unsigned char temp[2];
	temp[0]=0xFF;
	temp[1]=10;
	n=temp[1];
	write(fd,temp,sizeof(temp));

	srand(time(0));
	for(i=0;i<n;i++){
		val = rand()%100;
		printf("%d ",val);
		if(!write(fd,(const void*)&val,sizeof(val)))
			perror("write error");
	}
	//for(i=0;i<1e9;i++);
	printf("\n");
	for(i=0;i<n;i++){
		if(!read(fd,(void*)&val2,sizeof(val)))
			perror("read error");
		printf("%d ",val2);
	}
	printf("\n");

	
	temp[0]=0xF0;
	temp[1]=3;
	n=temp[1];
	write(fd,temp,sizeof(temp));

	char buffer[100];
	strcpy(buffer,"hello");
	write(fd,buffer,sizeof(buffer));
	strcpy(buffer,"helloworld");
	write(fd,buffer,sizeof(buffer));
	strcpy(buffer,"goodbye");
	write(fd,buffer,sizeof(buffer));

	memset(buffer,0,sizeof(buffer));
	for(i=0;i<n;i++){
		if(!read(fd,buffer,sizeof(buffer)))
			perror("");
		printf("%s ",buffer);
	}
	printf("\n");
	close(fd);
	return 0;
}