/*
kernel version - 5.2.6
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL"); 

static struct file_operations file_ops;
static DEFINE_MUTEX(char_mutex);
typedef struct _pcb{
	pid_t pid;
	int arr_num[100];
	char arr_str[100][100];
	int total;
	int obj_type;
	int cur_size;
	int cur;
	struct _pcb *next;
}pcb;

static pcb *head;

static int insertPCB(pid_t pid){
	
	pcb* p = head;
	pcb *temp = (pcb*)kmalloc(sizeof(pcb),GFP_KERNEL);
	temp->pid = pid;
	temp->total = 0;
	temp->obj_type = -1;
	temp->cur_size = 0;
	temp->cur = 0;
	temp->next = NULL;

	if(head==NULL){
		head = temp;
		return 0;
	}	
	while(p->next){
		if(pid == p->pid){
			kfree(temp);
			return -1;
		}
		p = p->next;
	}
	if(pid == p->pid){
		kfree(temp);
		return -1;
	}
	p->next = temp;

	return 0;
}

static int removePCB(pcb *temp){
	pcb *p,*prev;
	if(head==NULL || temp==NULL){
		return -1;
	}
	if(head->pid == temp->pid){
		p = head;
		head = head->next;
		kfree(p);
		return 0;
	}
	p = head;
	prev = head;
	while(p){
		if(p->pid == temp->pid){
			prev->next = p->next;
			kfree(p);
			return 0;
		}
		prev = p;
		p = p->next;
	}

	return -1;
}

static pcb* findPCB(pid_t pid){
	pcb* p = head;
	while(p){
		if(p->pid == pid){
			return p;
		}
		p = p->next;
	}
	return NULL;
}

static int open(struct inode *inodep, struct file *filep){

	pid_t pid = current->pid;
	while(!mutex_trylock(&char_mutex)){
	}
	if(insertPCB(pid)==-1){
		printk(KERN_ALERT "open - pid already exists");
		mutex_unlock(&char_mutex);
		return -EACCES;
	}
	mutex_unlock(&char_mutex);
	printk(KERN_ALERT "File opened by process %d",pid);

	return 0;
}

static int release(struct inode *inodep, struct file *filep){
	pid_t pid = current->pid;
	pcb *cur_proc;
	while(!mutex_trylock(&char_mutex)){
	}
	cur_proc = findPCB(pid);
	mutex_unlock(&char_mutex);
	if(cur_proc == NULL){
		printk(KERN_ALERT "release - invalid pid");
		return -EINVAL;
	}


	while(!mutex_trylock(&char_mutex)){
	}
	removePCB(cur_proc);
	mutex_unlock(&char_mutex);
	printk(KERN_ALERT "File closed by process %d",pid);
	return 0;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
	pid_t pid = current->pid;
	int ret;
	char *buffer;
	pcb *cur_proc;
	while(!mutex_trylock(&char_mutex)){	
	}
	cur_proc = findPCB(pid);
	mutex_unlock(&char_mutex);
	if(cur_proc == NULL){
		printk(KERN_ALERT "read - invalid pid");
		return -EACCES;
	}
	
	if(!buf || !count)
		return -EINVAL;

	if(cur_proc->cur_size < cur_proc->total || cur_proc->cur >= cur_proc->total){
		return -EACCES;
	}

	if(cur_proc->obj_type==0){
		buffer = (char*)&(cur_proc->arr_num[cur_proc->cur]);
		ret = sizeof(cur_proc->arr_num[cur_proc->cur]);
		cur_proc->cur++;
	}
	else{
		buffer = cur_proc->arr_str[cur_proc->cur];
		ret = sizeof(cur_proc->arr_str[cur_proc->cur]);
		cur_proc->cur++;
	}
	if(cur_proc->cur == cur_proc->total){
		cur_proc->cur_size = 0;
		cur_proc->total = 0;
		cur_proc->obj_type = -1;
	}
	
	if(copy_to_user(buf, buffer, ret))
		return -ENOBUFS;	
	return ret;
}

void sortnum(int arr[], int n){
	int i,j,temp;
	for (i = 0; i < n; i++){   
	    for (j = 0; j < n-1; j++){
	        if (arr[j] > arr[j+1]){
	        	temp = arr[j];
	        	arr[j] = arr[j+1];
	        	arr[j+1] = temp;
	        }
		}
	}	
}

void sortstr(char arr[][100], int n){
	int i,j;
	char buffer[100];
	for (i = 0; i < n; i++){   
	    for (j = 0; j < n-1; j++){
	    	if (strcmp(arr[j],arr[j+1]) > 0){
	        	strcpy(buffer, arr[j]);
	        	strcpy(arr[j], arr[j+1]);
	        	strcpy(arr[j+1], buffer);
	        }
		}
	}
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	int ret = count < 100 ? count:100;
	int temp;
	unsigned char buffer[100] = {0};
	pid_t pid = current->pid;
	pcb *cur_proc;
	while(!mutex_trylock(&char_mutex)){
	}
	cur_proc = findPCB(pid);
	mutex_unlock(&char_mutex);
	if(cur_proc == NULL){
		printk(KERN_ALERT "write - invalid pid");
		return -EACCES;
	}
	if(!buf || !count)
		return -EINVAL;

	if(copy_from_user(buffer, buf, ret))
		return -ENOBUFS;
	
	if(cur_proc->obj_type==-1){
		if(buffer[0]==0xFF){
			cur_proc->obj_type = 0;
		}
		else{
			cur_proc->obj_type = 1;
		}
		cur_proc->total = buffer[1];
		cur_proc->cur = 0;
	}
	else if(cur_proc->cur_size < cur_proc->total){
		if(cur_proc->obj_type==0){
			temp = *((int*)buffer);
			cur_proc->arr_num[cur_proc->cur_size] = temp;
		}
		else{
			strcpy(cur_proc->arr_str[cur_proc->cur_size],buffer);
		}
		cur_proc->cur_size++;
	}

	if(cur_proc->cur_size == cur_proc->total){
		
		if(cur_proc->obj_type==0){
			sortnum(cur_proc->arr_num, cur_proc->total);
		}
		else{
			sortstr(cur_proc->arr_str, cur_proc->total);	
		}
		
	}
	return ret;
}

static int hello_init(void)
{
	struct proc_dir_entry *entry = proc_create("partb_1", 0, NULL, &file_ops);
	if(!entry)
		return -ENOENT;
	file_ops.owner = THIS_MODULE;
	file_ops.write = write;
	file_ops.read = read;
	file_ops.open = open;
	file_ops.release = release;
	mutex_init(&char_mutex);
	printk(KERN_ALERT "Hello\n");
	return 0;
} 
static void hello_exit(void)
{
	remove_proc_entry("partb_1",NULL);
	mutex_destroy(&char_mutex);
	printk(KERN_ALERT "Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);