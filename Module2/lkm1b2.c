/*
kernel version - 5.2.6
*/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_SET_ORDER _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_GET_OBJ _IOR(0x10, 0x34, int32_t*)

#define PREORDER 0
#define INORDER 1
#define POSTORDER 2

#define NUM 0
#define STR 1

#define MAX 100

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

MODULE_LICENSE("GPL"); 

static struct file_operations file_ops;
typedef struct _node{
	int val;
	char str[100];
	int len;
	struct _node *left,*right;
}Node;

typedef struct{
	Node* arr[MAX];
	int top;
}stack;

typedef struct{
	Node* arr[MAX];
	int front, rear, size;
}queue;


static DEFINE_MUTEX(char_mutex);
typedef struct _pcb{
	pid_t pid;
	Node* root;
	stack myStack;
	int order;
	int objtype;
	struct _pcb *next;
}pcb;

static pcb *head;

static int insertPCB(pid_t pid){
	
	pcb* p = head;
	pcb *temp = (pcb*)kmalloc(sizeof(pcb),GFP_KERNEL);
	temp->pid = pid;

	temp->order = -1;
	temp->objtype = -1;
	temp->myStack.top = -1;
	temp->root = NULL;
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

static int isEmpty(stack* s) 
{ 
    return s->top == -1; 
} 
  
static void push(stack* s, Node* item) 
{ 
    s->arr[++s->top] = item; 
} 
  
static void pop(stack* s) 
{ 
    if (isEmpty(s)) 
        return; 
    s->top--; 
}

static Node* top(stack* s) 
{ 
    if (isEmpty(s)) 
        return NULL; 
    return s->arr[s->top]; 
} 

static void enqueue(queue* q, Node* item) 
{ 
    q->rear = (q->rear + 1)%MAX; 
    q->arr[q->rear] = item; 
    q->size = q->size + 1; 
} 
 
static Node* dequeue(queue* q) 
{ 
	Node* item;
    if (q->size == 0) 
        return NULL; 
    item = q->arr[q->front]; 
    q->front = (q->front + 1)%MAX; 
    q->size = q->size - 1; 
    return item; 
}

static void insertNum(pcb* cur_proc, int val) 
{ 
    
    Node* x = cur_proc->root;    
    Node* y = NULL;
    Node* newnode = (Node*)kmalloc(sizeof(Node),GFP_KERNEL);
    newnode->val = val;
    newnode->len = 0;
    newnode->left = NULL;
    newnode->right = NULL;
  
    while (x != NULL) { 
        y = x; 
        if (val < x->val) 
            x = x->left; 
        else
            x = x->right; 
    } 
    if (y == NULL)
        cur_proc->root = newnode; 
    else if (val < y->val) 
        y->left = newnode; 
    else
        y->right = newnode; 
}

static void insertStr(pcb* cur_proc, char str[], int len) 
{ 
    
    Node* x = cur_proc->root;    
    Node* y = NULL;
    Node* newnode = (Node*)kmalloc(sizeof(Node),GFP_KERNEL);
    memcpy(newnode->str, str, len);
    newnode->len = len;
    newnode->val = 0;
    newnode->left = NULL;
    newnode->right = NULL;
  
    while (x != NULL) { 
        y = x; 
        if (strcmp(str, x->str) < 0) 
            x = x->left; 
        else
            x = x->right; 
    } 
    if (y == NULL)
        cur_proc->root = newnode; 
    else if (strcmp(str, y->str) < 0) 
        y->left = newnode; 
    else
        y->right = newnode; 
}


static int hasNext(stack *myStack) {
    return !isEmpty(myStack);
}

static void pushAllInorder(Node *node, stack *myStack) {
    while (node != NULL) {
        push(myStack, node);
        node = node->left;
    }
}

static Node* nextInorder(stack *myStack) {
    Node *tmpNode = top(myStack);
    pop(myStack);
    pushAllInorder(tmpNode->right, myStack);
    return tmpNode;
}

static Node* nextPreorder(stack *myStack) {
    Node *tmpNode = top(myStack);
    pop(myStack);
    if(tmpNode->right)
    	push(myStack,tmpNode->right);
    if(tmpNode->left)
    	push(myStack,tmpNode->left);
    return tmpNode;
}

static void pushAllPostorder(Node *node, stack* myStack) {
	// Check for empty tree 
    if (node == NULL && isEmpty(myStack)) 
        return; 
      
    do
    { 
        // Move to leftmost node 
        while (node) 
        { 
            // Push root's right child and then root to stack. 
            if (node->right) 
                push(myStack, node->right); 
            push(myStack, node); 
  
            // Set root as root's left child   
            node = node->left; 
        } 
  
        // Pop an item from stack and set it as root     
        node = top(myStack); 
  		pop(myStack);
        // If the popped item has a right child and the right child is not 
        // processed yet, then make sure right child is processed before root 
        if (node->right && top(myStack) == node->right) 
        { 
            pop(myStack);  // remove right child from stack 
            push(myStack, node);  // push root back to stack 
            node = node->right; // change root so that the right  
                                // child is processed next 
        } 
        else  // Else print root's data and set root as NULL 
        { 
            //printf("%d ", node->val); 
            push(myStack,node);
            break;
            //node = NULL; 
        } 
    } while (!isEmpty(myStack)); 
}

static Node* nextPostorder(stack *myStack) {
    Node *tmpNode = top(myStack);
    pop(myStack);
    pushAllPostorder(NULL, myStack);
    return tmpNode;
}


void fillInfo(Node* root, struct obj_info* temp) 
{ 
	int min=-1;
	int height, nodeCount;
	Node* node;
	queue q; 
  	q.front = 0;
  	q.rear = -1;
  	q.size = 0;
  	 
  	
    // Base Case 
    if (root == NULL){
        return; 
    }
  
    // Create an empty queue for level order tarversal 
    
    // Enqueue Root and initialize height 
    enqueue(&q, root); 
    height = 0;

    while (1) 
    { 
        // nodeCount (queue size) indicates number of nodes 
        // at current lelvel. 
        nodeCount = q.size; 
        if (nodeCount == 0){
        	temp->mindepth = min;
        	temp->maxdepth = height; 
            return; 
        }
  
        height++; 
  
        // Dequeue all nodes of current level and Enqueue all 
        // nodes of next level 
        while (nodeCount > 0) 
        { 
            node = dequeue(&q); 
            if (node->left != NULL) 
                enqueue(&q, node->left); 
            if (node->right != NULL) 
                enqueue(&q, node->right); 
            if(node->left == NULL && node->right == NULL && min == -1){
            	min = height;
            }
            if(node == root){
            	if(node->left && node->right){
            		temp->deg2cnt++;
            	}
            	else if(node->left || node->right){
            		temp->deg1cnt++;
            	}
            }
            else{
            	if(node->left && node->right){
            		temp->deg3cnt++;
            	}
            	else if(node->left || node->right){
            		temp->deg2cnt++;
            	}
            	else{
            		temp->deg1cnt++;
            	}
            }
            nodeCount--; 
        }
    }
}

static void deleteTree(pcb* cur_proc) 
{ 
	queue q;
	Node *node;

	q.front = 0;
  	q.rear = -1;
  	q.size = 0;
	    // Base Case 
    if (cur_proc->root == NULL) 
        return; 
  
    // Create an empty queue for level order traversal 
     
  
    // Do level order traversal starting from root 
    enqueue(&q, cur_proc->root); 
    while (q.size > 0) 
    { 
        node = dequeue(&q); 
  
        if (node->left != NULL) 
            enqueue(&q, node->left); 
        if (node->right != NULL) 
            enqueue(&q, node->right); 
  
        kfree(node); 
    } 
    cur_proc->root = NULL;
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

	deleteTree(cur_proc);
	while(!isEmpty(&(cur_proc->myStack))){
		pop(&(cur_proc->myStack));
	}
	while(!mutex_trylock(&char_mutex)){
	}
	removePCB(cur_proc);
	mutex_unlock(&char_mutex);

	printk(KERN_ALERT "File closed by process %d",pid);
	
	return 0;
}

static char findNum(Node* root, int val){
	Node* p = root;
	while(p){
		if(p->val == val)
			return 1;
		else if(val < p->val)
			p = p->left;
		else
			p = p->right;
	}
	return 0;
}


static char findStr(Node* root, char str[]){
	Node* p = root;
	while(p){
		if(strcmp(p->str, str) == 0)
			return 1;
		else if(strcmp(str, p->str) < 0)
			p = p->left;
		else
			p = p->right;
	}
	return 0;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
	int temp, ret;
	Node* p;
	char *buffer;
	pid_t pid = current->pid;
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

	if(cur_proc->order == -1 || cur_proc->objtype == -1)
		return -EACCES;
	
	if(!hasNext(&(cur_proc->myStack)))
		return 0;
	
	if(cur_proc->order == INORDER)
		p = nextInorder(&(cur_proc->myStack));
	else if(cur_proc->order == PREORDER)
		p = nextPreorder(&(cur_proc->myStack));
	else
		p = nextPostorder(&(cur_proc->myStack));

	if(cur_proc->objtype == NUM){
		temp = p->val;
		buffer = (char*)&temp;
		ret = sizeof(int);
		//printk(KERN_ALERT "read %d",temp);
	}
	else{
		buffer = p->str;
		ret = p->len;
		//printk(KERN_ALERT "read %s",buffer2);
	}
	
	if(copy_to_user(buf, buffer, ret))
		return -ENOBUFS;
	return ret;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	int ret = count < 256 ? count:256;
	int temp;
	unsigned char buffer[256] = {0};
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
	//printk(KERN_ALERT "write objtype=%d, cur_size=%d, total=%d",objtype,cur_size,total);
	if(!buf || !count)
		return -EINVAL;

	if(cur_proc->objtype == -1)
		return -EACCES;
	
	if(copy_from_user(buffer, buf, ret))
		return -ENOBUFS;
	
	if(cur_proc->objtype == NUM){
		temp = *((int*)buffer);
		if(findNum(cur_proc->root, temp) == 0){
			printk(KERN_ALERT "write %d",temp);
			insertNum(cur_proc,temp);
		}
	}
	else{
		if(findStr(cur_proc->root, buffer) == 0){
			printk(KERN_ALERT "write %s",buffer);
			insertStr(cur_proc,buffer,ret);
		}
	}
	
	if(cur_proc->order != -1){
		while(!isEmpty(&(cur_proc->myStack))){
			pop(&(cur_proc->myStack));
		}
		if(cur_proc->order == INORDER){
			pushAllInorder(cur_proc->root, &(cur_proc->myStack));
		}
		else if(cur_proc->order == PREORDER){
			push(&(cur_proc->myStack), cur_proc->root);
		}
		else if(cur_proc->order == POSTORDER){
			pushAllPostorder(cur_proc->root, &(cur_proc->myStack));
		}
	}
	
	return ret;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct search_obj temp;
	char tmp;
	unsigned char tmp2;
	struct obj_info temp2;
	pid_t pid = current->pid;
	pcb *cur_proc;
	temp2.deg1cnt = 0;
    temp2.deg2cnt = 0;
    temp2.deg3cnt = 0;
    temp2.maxdepth = 0;
    temp2.mindepth = 0;

    
	while(!mutex_trylock(&char_mutex)){
	}
	cur_proc = findPCB(pid);
	mutex_unlock(&char_mutex);
	if(cur_proc == NULL){
		printk(KERN_ALERT "write - invalid pid");
		return -EACCES;
	}

	switch(cmd) {
		case PB2_SET_TYPE:
			copy_from_user(&tmp2, (unsigned char*) arg, sizeof(unsigned char));
			if(cur_proc->objtype != -1){
				deleteTree(cur_proc);
				while(!isEmpty(&(cur_proc->myStack))){
					pop(&(cur_proc->myStack));
				}
			}
			if(tmp2 == 0xFF){
				cur_proc->objtype = NUM;
			}
			else if(tmp2 == 0xF0){
				cur_proc->objtype = STR;
			}
			else{
				return -EINVAL;
			}
			break;
		case PB2_SET_ORDER:
			if(cur_proc->objtype == -1)
				return -EACCES;
			copy_from_user(&tmp, (char*) arg, sizeof(char));
			while(!isEmpty(&(cur_proc->myStack))){
				pop(&(cur_proc->myStack));
			}
			if(tmp == 0x00){
				cur_proc->order = INORDER;
				pushAllInorder(cur_proc->root, &(cur_proc->myStack));
			}
			else if(tmp == 0x01){
				cur_proc->order = PREORDER;
				push(&(cur_proc->myStack), cur_proc->root);
			}
			else if(tmp == 0x02){
				cur_proc->order = POSTORDER;
				pushAllPostorder(cur_proc->root, &(cur_proc->myStack));
			}
			else{
				return -EINVAL;
			}
			break;
		case PB2_GET_INFO:
			if(cur_proc->objtype == -1)
				return -EACCES;
			fillInfo(cur_proc->root, &temp2);
			copy_to_user((struct search_obj*)arg, &temp2, sizeof(temp2));
			break;
		case PB2_GET_OBJ:
			if(cur_proc->objtype == -1)
				return -EACCES;
			copy_from_user(&temp, (struct search_obj*)arg, sizeof(temp));
			if(cur_proc->objtype == NUM && temp.objtype != 0xFF)
				return -EINVAL;
			if(cur_proc->objtype == STR && temp.objtype != 0xF0)
				return -EINVAL;
			if(cur_proc->objtype == NUM)
				temp.found = findNum(cur_proc->root, temp.int_obj);
			else
				temp.found = findStr(cur_proc->root, temp.str);
			copy_to_user((struct search_obj*)arg, &temp, sizeof(temp));
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int hello_init(void)
{
	struct proc_dir_entry *entry = proc_create("partb_2", 0, NULL, &file_ops);
	if(!entry)
		return -ENOENT;
	file_ops.owner = THIS_MODULE;
	file_ops.write = write;
	file_ops.read = read;
	file_ops.open = open;
	file_ops.release = release;
	file_ops.unlocked_ioctl = ioctl;
	mutex_init(&char_mutex);
	printk(KERN_ALERT "Hello, world\n");
	return 0;
} 
static void hello_exit(void)
{
	remove_proc_entry("partb_2",NULL);
	mutex_destroy(&char_mutex);
	printk(KERN_ALERT "Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);