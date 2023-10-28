#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_SET_ORDER _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_GET_OBJ _IOR(0x10, 0x34, int32_t*)

struct search_obj{
	unsigned char objtype;
	char found;
	int32_t int_obj;
	char str[100];
	int32_t len;
};

struct obj_info{
	int32_t deg1cnt;
	int32_t deg2cnt;
	int32_t deg3cnt;
	int32_t maxdepth;
	int32_t mindepth;
};

int main(){
	int fd,val,i,n,val2;
	unsigned char objtype;
	char order;
	fd = open("/proc/partb_2",O_RDWR);
	if(fd<0){
		perror("open error");
		return 1;
	}
	printf("Integer values - \n");
	objtype = 0xFF;
	ioctl(fd, PB2_SET_TYPE, &objtype);
	n=10;
	srand(time(0));
	for(i=0;i<n;i++){
		val = rand()%100;
		printf("%d ",val);
		if(write(fd,(const void*)&val,sizeof(val))<0)
			perror("write error");
	}
	printf("\n\n");

	printf("Inorder - \n");
	order = 0x00;
	ioctl(fd, PB2_SET_ORDER, &order);
	for(i=0;i<2;i++){
		read(fd,(void*)&val2,sizeof(val));
		printf("%d ",val2);
	}
	val = rand()%100;
	printf("write %d\n",val);
	if(write(fd,(const void*)&val,sizeof(val))<0)
		perror("write error");

	while(read(fd,(void*)&val2,sizeof(val))>0){
		printf("%d ",val2);
	}
	printf("\n");

	printf("Preorder - \n");
	order = 0x01;
	ioctl(fd, PB2_SET_ORDER, &order);
	while(read(fd,(void*)&val2,sizeof(val))>0){
		printf("%d ",val2);
	}
	
	printf("\n");

	printf("Postorder - \n");
	order = 0x02;
	ioctl(fd, PB2_SET_ORDER, &order);
	while(read(fd,(void*)&val2,sizeof(val))){
		printf("%d ",val2);
	}
	printf("\n");

	struct search_obj temp1,temp2;
	printf("Enter number to search - \n");
	scanf("%d",&(temp1.int_obj));
	temp1.objtype = 0xFF;
	ioctl(fd, PB2_GET_OBJ, &temp1);
	printf("found = %d\n",temp1.found);
	
	printf("Enter number to search - \n");
	scanf("%d",&(temp2.int_obj));
	temp2.objtype = 0xFF;
	ioctl(fd, PB2_GET_OBJ, &temp2);
	printf("found = %d\n",temp2.found);

	struct obj_info temp;
    temp.deg1cnt = 0;
    temp.deg2cnt = 0;
    temp.deg3cnt = 0;
    temp.maxdepth = 0;
    temp.mindepth = 0;

    ioctl(fd, PB2_GET_INFO, &temp);
  	printf("deg1cnt = %d\n",temp.deg1cnt);
  	printf("deg2cnt = %d\n",temp.deg2cnt);
  	printf("deg3cnt = %d\n",temp.deg3cnt);
	printf("maxdepth = %d\n",temp.maxdepth);
	printf("mindepth = %d\n",temp.mindepth);


	printf("\nString values - \n");

	objtype = 0xF0;
	ioctl(fd, PB2_SET_TYPE, &objtype);
	n=4;
	char buffer[100];
	strcpy(buffer,"hello");
	printf("%s ",buffer);
	if(write(fd,buffer,strlen(buffer)+1)<0)
		perror("write error");

	strcpy(buffer,"qwerty");
	printf("%s ",buffer);
	if(write(fd,buffer,strlen(buffer)+1)<0)
		perror("write error");

	strcpy(buffer,"goodbye");
	printf("%s ",buffer);
	if(write(fd,buffer,strlen(buffer)+1)<0)
		perror("write error");

	strcpy(buffer,"hey");
	printf("%s ",buffer);
	if(write(fd,buffer,strlen(buffer)+1)<0)
		perror("write error");


	char buffer2[100];
	printf("\n\nInorder - \n");
	order = 0x00;
	ioctl(fd, PB2_SET_ORDER, &order);
	while(read(fd,(void*)buffer2,sizeof(buffer2))>0){
		printf("%s ",buffer2);
	}
	printf("\n");

	printf("Preorder - \n");
	order = 0x01;
	ioctl(fd, PB2_SET_ORDER, &order);
	while(read(fd,(void*)buffer2,sizeof(buffer2))>0){
		printf("%s ",buffer2);
	}
	printf("\n");

	printf("Postorder - \n");
	order = 0x02;
	ioctl(fd, PB2_SET_ORDER, &order);
	while(read(fd,(void*)buffer2,sizeof(buffer2))>0){
		printf("%s ",buffer2);
	}
	printf("\n");

	scanf("%s",temp1.str);
	temp1.objtype = 0xF0;
	ioctl(fd, PB2_GET_OBJ, &temp1);
	printf("found = %d\n",temp1.found);
	
	scanf("%s",temp2.str);
	temp2.objtype = 0xF0;
	ioctl(fd, PB2_GET_OBJ, &temp2);
	printf("found = %d\n",temp2.found);
	temp.deg1cnt = 0;
    temp.deg2cnt = 0;
    temp.deg3cnt = 0;
    temp.maxdepth = 0;
    temp.mindepth = 0;

    ioctl(fd, PB2_GET_INFO, &temp);
  	printf("deg1cnt = %d\n",temp.deg1cnt);
  	printf("deg2cnt = %d\n",temp.deg2cnt);
  	printf("deg3cnt = %d\n",temp.deg3cnt);
	printf("maxdepth = %d\n",temp.maxdepth);
	printf("mindepth = %d\n",temp.mindepth);
	close(fd);
	return 0;
}