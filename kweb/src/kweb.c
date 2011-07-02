#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/init.h>
#include<asm/uaccess.h>
#include<linux/sched.h>
#include<linux/string.h>
#include<linux/sysfs.h>
#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/workqueue.h>
#include<linux/file.h>
#include<linux/fcntl.h>
#include<linux/net.h>
#include<linux/in.h>
#include<linux/ctype.h>
#include<linux/slab.h>
#include<linux/net.h>
#include<net/sock.h>
#include "ksocket.h"
#include "httpconf.h"
#include "log.h"
#include "tokenize.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gaurav Tungatkar");
MODULE_DESCRIPTION("Simple http server module");

#define KWEB_ENTRY kweb
#define RWPERM 0644
#define LISTEN_PORT_ENTRY "listen_port"
#define DOCUMENT_ROOT_ENTRY "document_root"
#define DIR_INDEX_ENTRY "directory_index"
#define CONTENT_TYPE_ENTRY "content_type"
#define BUFFSIZE (1*1024)
#define MAX_FILENAME 512
#define HTTP_HDR_SIZE 512
#define HTTP_URI_SIZE 1024
#define HTTP_STATUS_SIZE 1024
#define HTTP_GET 11
#define SP 0x20 
#define CRLF "\r\n"

#define KSOCKCLOSE(_ksk)		\
{					\
	if(_ksk != NULL)		\
		kclose(_ksk);		\
	_ksk = NULL;			\
}
/*
kweb : Web server as kernel module. 2 workqueues are used, one to monitor start parameter and other to run the web server 
*/
/* module parameter to start/stop server */
static int start = 0;

/*proc fs entries */
static struct proc_dir_entry *kweb;
static struct proc_dir_entry *listen_port, *dir_index;
static struct proc_dir_entry *document_root, *content_type;

/*listen on this socket */
ksocket_t listen_fd;

module_param(start, int, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

static struct workqueue_struct *kweb_wq, *http_wq;
static int server_started = 0;
static int cleanup = 0;
static struct http_server_config cfg_k, cfg;

/*workqueue which runs the web server */
void http_server_wq(struct work_struct * ignore)
{
	connection_handler(&cfg);
}

static DECLARE_DELAYED_WORK(svr, http_server_wq);

/*workqueue for monitoring start parameter */
void start_monitor_wq(struct work_struct * ignore);
static DECLARE_DELAYED_WORK(monitor_start, start_monitor_wq);
void start_monitor_wq(struct work_struct * ignore)
{	

	if(start == 1)
	{	
		if(!server_started)
		{
			/*if server has not been started, create a workqueue and			 start the server in it */
			server_started = 1;
			http_wq = create_workqueue("http_queue");
			if(http_wq == NULL) {
				goto out;
			} 
			//clone the current configuration and use this clone in http_server
			//now cfg_k can change freely, but wont come into effect
			//until the server is started again
			memcpy((void*)&cfg,(void*)&cfg_k, sizeof(cfg_k));
			queue_delayed_work(http_wq, &svr, 15);
			cleanup = 1;
		}
	}
	else
	{
		/* if start = 0 and server is already running, shut it down*/
		if(server_started)
		{
			cancel_delayed_work(&svr);
			KSOCKCLOSE(listen_fd);
			flush_workqueue(http_wq);
			destroy_workqueue(http_wq);
			server_started = 0;
			cleanup = 0;
			
		}
	}
	/* periodically keep running this function*/
	queue_delayed_work(kweb_wq, &monitor_start, 10);
out:
	return;
}
 
/* read/write procedures for various procfs entries*/
static int proc_read_listen_port(char *page, char **start,
			off_t off, int count,
			int *eof, void *data)
{
	int len;
	if(off > 0)
	{	
		*eof = 1;
		return 0;
	}
	len = sprintf(page, "%u\n",  cfg_k.listen_port);
	return len;
}
static int proc_write_listen_port(struct file *file,
			const char *buffer,
			unsigned long count,
			void *data)
{

	int temp;
#define MAX_PORT_LEN_CHARS 6 
	char port[10];
	if(count > MAX_PORT_LEN_CHARS)	
	{
		//don't try to convert that string.
		LOGK("Bad /proc/kweb/listen_port value\n");
		return -EFAULT;
	}
		
	if(copy_from_user(port, buffer, count)){
		return -EFAULT;
	}
	port[count] = '\0';
	temp = simple_strtoul(port, NULL, 10);
	if(temp > 65535)
	{
		LOGK("Bad /proc/kweb/listen_port value\n");
		return -EFAULT;
	}
	cfg_k.listen_port = temp;
	return count;
}
static int proc_read_content_type(char *page, char **start,
			off_t off, int count,
			int *eof, void *data)
{
	int len, i, l = 0;
	char *str = page;
	if(off > 0)
	{	
		*eof = 1;
		return 0;
	}
	for(i = 0; i < cfg_k.f_type_cnt; i++)
	{
		len = sprintf(str, "%s %s\r\n", 
			cfg_k.filetypes[i].extension, 
			cfg_k.filetypes[i].type);
		l = l + len;
		str += len;
		if(count < l)
			return -EFAULT;
	}
	*str = 0;
	return l;
}
static int proc_write_content_type(struct file *file,
			const char *buffer,
			unsigned long count,
			void *data)
{

	int len;
	char *buf;
	char token[MAX_BUF];
	int next_token = DEFAULT;
	if(count > MAX_BUF)	
	{
		//don't try to convert that string.
		LOGK("Bad /proc/kweb/content_type value\n");
		return -EFAULT;
	}
	if(count <= 0)
		return 0;
	buf = kmalloc(count+1, GFP_KERNEL);
	if(buf == NULL)
	{
		return -EFAULT;
	}
	if(copy_from_user(buf, buffer, count))
	{
		return -EFAULT;
	}
	buf[count] = '\0';
	token[0] = '\0';
	cfg_k.f_type_cnt = 0;
	while(tokenize(&buf, token) > 0)
	{
		if(token[0] == '.')
			next_token = FILE_TYPE;
		len = strlen(token);
		switch(next_token)
		{	
			case FILE_TYPE:
				if(len > sizeof(cfg_k.filetypes[0].extension))
				{

					LOGK("content_type:File type size exceeded\n");
					return -ENOSPC;

				}
				strcpy(cfg_k.filetypes[cfg_k.f_type_cnt].extension,token);
				next_token = FILE_TYPE_EXT;
				break;
			case FILE_TYPE_EXT:
				if(len > sizeof(cfg_k.filetypes[0].type))
				{
					//LOG
					LOGK("content_type:Extension size exceeded\n");
					return -ENOSPC;

				}
				strcpy(cfg_k.filetypes[cfg_k.f_type_cnt].type,token);
				cfg_k.f_type_cnt++;
				next_token = DEFAULT;
				break;
			default:
				LOGK("Error in content_type");
				return 0;
		}
	}	
	return count;
}
static int proc_read_dir_index(char *page, char **start,
			off_t off, int count,
			int *eof, void *data)
{
/* only 2 or less default index file names supported */
	int len;
	if(off > 0)
	{	
		*eof = 1;
		return 0;
	}
	if(cfg_k.dir_indx_cnt == 2)
		len = sprintf(page, "%s %s",  cfg_k.dir_index[0].filename,
				cfg_k.dir_index[1].filename);
	else
		len = sprintf(page, "%s",  cfg_k.dir_index[0].filename);
	return len;
}
static int proc_write_dir_index(struct file *file,
			const char *buffer,
			unsigned long count,
			void *data)
{

	int  i;
#define MAX_PORT_LEN_CHARS 6 
	//printk("write routine called\n");
	char *dir_indx;
	if(count > MAX_BUF)	
	{
		//don't try to convert that string.
		LOGK("Bad /proc/kweb/directory_index value\n");
		return -EFAULT;
	}
	dir_indx = kmalloc(count+1, GFP_KERNEL);	
	if(dir_indx == NULL)
	{
		LOGK("mem alloc failure in proc_write_dir_index\n");
		return -EFAULT;
	}
	if(copy_from_user(dir_indx, buffer, count)){
		return -EFAULT;
	}
	dir_indx[count] = '\0';
	cfg_k.dir_indx_cnt = 0;
	for(i = 0; i < 2; i++)
	{
		if(tokenize(&dir_indx, cfg_k.dir_index[i].filename) <= 0)
			break;
		cfg_k.dir_indx_cnt++;
	}
	kfree(dir_indx);
	return count;
}
static int proc_read_document_root(char *page, char **start,
			off_t off, int count,
			int *eof, void *data)
{
	int len;
	if(off > 0)
	{	
		*eof = 1;
		return 0;
	}
	len = sprintf(page, "%s\n",  cfg_k.document_root);
	return len;
}
static int proc_write_document_root(struct file *file,
			const char *buffer,
			unsigned long count,
			void *data)
{

	//printk("write routine called\n");
	if(count > MAX_BUF)	
	{
		//don't try to convert that string.
		LOGK("Bad /proc/kweb/document_root value\n");
		return -ENOSPC;
	}
		
	if(copy_from_user(cfg_k.document_root, buffer, count)){
		return -EFAULT;
	}
	cfg_k.document_root[count] = '\0';
	return count;
}
/*
 * tokenize()
 * str = pointer to string that has to be tokenized
 * dest = destination where separate token will be stored
 *
 * This function separates tokens from the given string. Tokens can be of
 * maximum MAX_TOKEN_SIZE . After a call to tokenize, str points to the first
 * character after the current token; the string is consumed by the tokenize
 * routine.
 * RETURNS: length of current token
 *          -1 if error
 */
int tokenize(char **str, char *dest)
{
        int count = 0;
	if(**str == '\0')
		return 0;
        while(isspace(**str))
                (*str)++;
        while(!isspace(**str) && (**str != '\0'))
        {
                *dest++ = *(*str)++;
                count++;
                if(count >= MAX_TOKEN_SIZE)
                {
                        count = -1;
                        break;
                }
        }
        *dest = '\0';
        return count;
}
int connection_handler(struct http_server_config *cfg)
{
        ksocket_t  new_fd = NULL;
	int set = 1;
        struct sockaddr_in listener_addr, client_addr;
        int addr_len = 0;
        char p[50];
        /* Standard server side socket sequence*/
//	allow_signal(SIGTERM);

        if((listen_fd = ksocket(AF_INET, SOCK_STREAM, 0)) == NULL)
        {
                LOGK( "socket() failure\n");
                return -1;
        }
        if (ksetsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &set,
                                sizeof(int)) == -1) {
                LOGK( "setsockopt() failed");
                return -1;
        }
        memset(&listener_addr, 0, sizeof(listener_addr));
        listener_addr.sin_family = AF_INET;
        listener_addr.sin_port = htons(cfg->listen_port);
        listener_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(listener_addr.sin_zero, '\0', sizeof(listener_addr.sin_zero));

        if(kbind(listen_fd, (struct sockaddr*)&listener_addr, sizeof
                                listener_addr) < 0)
        {
                LOGK( "bind() failed\n");
                return -1;
        
        }

        if(klisten(listen_fd, PENDING_CONN) == -1)
        {
                LOGK( "listen() failed\n");
                return -1;
        }
        sprintf(p, "HTTP server listening on port:%d\n", cfg->listen_port);
        LOGK( p);
        while(1)
        {
		while(1) {
			/*NON-BLOCKING accept. After each call, poll the value of start.
			 if start = 0, return*/
			new_fd = kaccept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
			if(new_fd != NULL)break;
			
			if(start == 0)
			{
				LOGK("Kweb Server shutting down..");
				return 0;
			}
		}
		LOGK("kweb:new connection\n");
                if(new_fd == NULL)
                {
                        //log
                        LOGK( "accept() failed\n");
                        return -1;
                
                }
		http_server(cfg, new_fd);
		KSOCKCLOSE(new_fd);


        }
	KSOCKCLOSE(listen_fd);
        return 0;

}
/*http server engine - parsing and contructing messages and sending them*/
void http_server(struct http_server_config *cfg, ksocket_t sockfd)
{

        int numbytes;
        char request_method[5];
        char version[10];
        int method;
        int ftype;
	char *uri;
	char *filepath;
	char *status;
	char *header;
	char *buffer;
	char *request;
	struct file 	*filep;
	mm_segment_t	old_fs;
	char *requestptr;
        int errflag = 0;
        int blksize = 0;
	uri = kmalloc(HTTP_URI_SIZE, GFP_KERNEL);
	filepath = kmalloc(HTTP_URI_SIZE, GFP_KERNEL);
	status = kmalloc(HTTP_STATUS_SIZE, GFP_KERNEL);
	header=kmalloc(HTTP_HDR_SIZE, GFP_KERNEL); 
	buffer =kmalloc(BUFFSIZE, GFP_KERNEL);
	request = kmalloc(BUFFSIZE+1 , GFP_KERNEL);
	memset(uri, 0, HTTP_URI_SIZE);
	memset(filepath, 0, HTTP_URI_SIZE);
	memset(status, 0, HTTP_HDR_SIZE);
	memset(header, 0, HTTP_HDR_SIZE);
	memset(buffer, 0, BUFFSIZE);
	memset(request, 0, BUFFSIZE);
	if(uri == NULL ||
		filepath == NULL ||
		status == NULL ||
		header == NULL ||
		buffer == NULL ||
		request == NULL)
	{
		LOGK("memory alloc failure in http_server()\n");
		kclose(sockfd);
		return;
	}
        if((numbytes = krecv(sockfd, (void *)request, BUFFSIZE, 0)) <= 0)
        {
                LOGK( "read from socket failed");
                return;
        }
        requestptr = request;
	LOGK("kweb:HTTP request received\n");
        if((method = valid_method_string(&requestptr, request_method)) == -1)
        {
                //ERROR in Request
                snprintf(status, 
                        HTTP_STATUS_SIZE,
                        "HTTP/1.0 400 Bad Request: Invalid Method: %s\r\n", 
                        request_method);
                LOGK( status);
                errflag = 1;
        }
        if(!errflag && (method == HTTP_GET))
        {
                //tokenize URI
                //check that the method name and URI are separated by exactly 1
                //SP character
                //requestptr should now be pointing at a SP character. If the
                //next character is SP as well, invalid request
                if(valid_uri(&requestptr, cfg, uri) == -1)
                {
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 400 Bad Request: Invalid URI: %s\r\n", 
                                uri);
                        LOGK( status);
                        //ERROR in request
                        errflag = 1;
                
                }
                
        
        }
        if(!errflag)
        {
                if(valid_version(&requestptr, cfg, version) == -1)
                {
                        //ERROR
                        //HTTP/1.0 400 Bad Request: Invalid HTTP-Version:
                        //<requested HTTP version>
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 400 Bad Request: Invalid HTTP-Version: %s\r\n",
                                version);
                        LOGK( status);
                        errflag = 1;
                }

        }
        if(!errflag)
        {
                if((ftype = valid_filetype(&requestptr, cfg, uri)) == -1)
                {
                        //ERROR
                        snprintf(status, 
                                HTTP_STATUS_SIZE,
                                "HTTP/1.0 501 Not Implemented: %s\r\n",
                                 uri);
                        LOGK( status);
                        errflag = 1;
                }

        }
        //seems like request came up fine! Now lets see if we can read the file
        if(!errflag)
        {
                /*all file paths relative to document root */
                strncat(filepath, cfg->document_root, HTTP_URI_SIZE);
                strncat(filepath, uri, HTTP_URI_SIZE);
                filep = filp_open(filepath, 00, O_RDONLY);
		if (IS_ERR(filep)||(filep==NULL) ||(filep->f_op->read==NULL))
                {
                        snprintf(status,HTTP_STATUS_SIZE,
                                "HTTP/1.0 404 Not Found: %s\r\n", 
                                 uri);
                        LOGK( status);
                        errflag = 1;
                        //file not found..
                
                }

        }
        if(!errflag)
        {
                int filelen;
                //find file size
                old_fs = get_fs();
		set_fs(KERNEL_DS);
		 
		if((filelen = generic_file_llseek(filep, 0, SEEK_END)) < 0)
                {
                        //error
                        LOGK( "lseek() failed\n");
                }
                generic_file_llseek(filep, 0, SEEK_SET);
		set_fs(old_fs);
                snprintf(status, HTTP_STATUS_SIZE, 
                                "HTTP/1.0 200 Document Follows\r\n");
                LOGK( status);
                LOGK( filepath);
                snprintf(header, HTTP_HDR_SIZE, 
                                "Content-Type: %s\r\nContent-Length: %d\r\n\n",
                                cfg->filetypes[ftype].type,filelen );
                /*send status and header */
                //strncat(status, header, strlen(header));
		ksend(sockfd, status, strlen(status), MSG_MORE);
                ksend(sockfd, header, strlen(header), MSG_MORE);
                /*write the file contents to socket */
                old_fs = get_fs();
		set_fs(KERNEL_DS);
		while ( (blksize = filep->f_op->read(filep, buffer, BUFFSIZE, &filep->f_pos)) > 0 ) {
                        ksend(sockfd,buffer,blksize, 0);
                }
		set_fs(old_fs);
		fput(filep);
        
        }
        else
        {
                /*Request had error. Send the appropriate status*/
                ksend(sockfd, status, strlen(status), 0);
                LOGK( "Error in processing request\n");

        }



        kfree(uri);
        kfree(filepath);
        kfree(status);
        kfree(header); 
	kfree(buffer);
        kfree(request);
        return;
}
int valid_method_string(char **request, char *request_method)
{
        /*only GET method supported */
        if((tokenize(request, request_method) != 3)
                        ||(strcmp(request_method, "GET") != 0))
        {
                LOGK("Invalid method\n");
                return -1;
        }
        else
        {
                return HTTP_GET;
        }

}
int valid_version(char **request, struct http_server_config *cfg, 
                char *version)
{
        /* HTTP versions 1.0 and 1.1 messages are accepted
         */
        if((tokenize(request, version) <= 0)
                        ||((strcmp(version, "HTTP/1.1") != 0) && (strcmp(version,
                                                "HTTP/1.0") != 0)))
        {
                LOGK( "Version not supported\n");
                return -1;
        }
        else
        {
                return 0;
        }


}
int valid_uri(char **request, struct http_server_config *cfg, 
                char *uri)
{
        /*if it sees 2 or more leading spaces(SP) - thats invalid URI*/
        if(*(*(request)+1) == SP)
        {
                LOGK( "Invalid URI\n");
                return -1;
        }
        
        if((tokenize(request, uri) <= 0))
        {
                LOGK( "Invalid URI\n");
                return -1;
        }
        else
        {
                //cannot refer to the parent directory
                if(uri[0] == '.' && uri[1] == '.')
                {
                        LOGK( "Invalid URI\n");
                        return -1;
                }
                //if just '/' , append the default index file name
                if((uri[0] == '/') && (uri[1] == '\0'))
                        strcat(uri,
                           cfg->dir_index[0].filename);
        }
        return 0;

}

int valid_filetype(char **request, struct http_server_config *cfg, 
                char *uri)
{
        int i = 0, validstr = -1;  
        int j;
        /* entry in the form of
         * .html text/html
         * So get the extension. */
        while(uri[i] != '.') 
        { 
                if(uri[i] != '\0')
                        i++;
                else 
                        return -1;

        }
        /*Check if this extension is present in supported extensions*/
        for(j = 0; j < cfg->f_type_cnt; j++)
        {
                if(!strcmp((uri+i), cfg->filetypes[j].extension))
                        validstr = j;
        }
        if(validstr < 0)
        {
                LOGK( "Invalid filetype\n");
                return -1;
        }
        return validstr;

}


int kweb_module_init(void)
{
	int rv = 0, ret = 0;

	//create kweb directory in /procfs
	kweb = proc_mkdir("kweb", NULL);
	if(kweb == NULL){
		rv = -ENOMEM;
		goto out;
	}
	
	/*create listen port entry*/
	listen_port = create_proc_entry(LISTEN_PORT_ENTRY, RWPERM, KWEB_ENTRY);
	if(listen_port == NULL){
		rv = -ENOMEM;
		goto out;
	}
	listen_port->read_proc = proc_read_listen_port;
	listen_port->write_proc = proc_write_listen_port;
	/*create document root entry */
	document_root = create_proc_entry(DOCUMENT_ROOT_ENTRY, RWPERM, KWEB_ENTRY);
	if(document_root == NULL){
		rv = -ENOMEM;
		goto out;
	}
	document_root->read_proc = proc_read_document_root;
	document_root->write_proc = proc_write_document_root;
	
	dir_index= create_proc_entry(DIR_INDEX_ENTRY, RWPERM, KWEB_ENTRY);
	if(dir_index == NULL){
		rv = -ENOMEM;
		goto out;
	}
	dir_index->read_proc = proc_read_dir_index;
	dir_index->write_proc = proc_write_dir_index;
	content_type = create_proc_entry(CONTENT_TYPE_ENTRY, RWPERM, KWEB_ENTRY);
	if(content_type == NULL){
		rv = -ENOMEM;
		goto out;
	}
	content_type->read_proc = proc_read_content_type;
	content_type->write_proc = proc_write_content_type;


	/*work queue to monitor start parameter. when start is set to 1,
	 it will spawn the server*/
	kweb_wq = create_workqueue("kweb_queue");
	if(kweb_wq == NULL) {
		rv = -ENOMEM;
		goto out;
	} 
	
	/* default configuration values*/
	cfg_k.listen_port = 8000;
	cfg_k.f_type_cnt = 0;
#define ftctr cfg_k.f_type_cnt
	strcpy(cfg_k.document_root,"/home/csc573/www"); 
	
	strcpy(cfg_k.filetypes[ftctr].extension, ".png");
	strcpy(cfg_k.filetypes[ftctr++].type, "image/png");
	strcpy(cfg_k.filetypes[ftctr].extension, ".html");
	strcpy(cfg_k.filetypes[ftctr++].type, "text/html");
	strcpy(cfg_k.filetypes[ftctr].extension, ".jpeg");
	strcpy(cfg_k.filetypes[ftctr++].type, "image/jpeg");
	strcpy(cfg_k.filetypes[ftctr].extension, ".jpg");
	strcpy(cfg_k.filetypes[ftctr++].type, "image/jpeg");
	strcpy(cfg_k.filetypes[ftctr].extension, ".txt");
	strcpy(cfg_k.filetypes[ftctr++].type, "text/plain");
	strcpy(cfg_k.filetypes[ftctr].extension, ".gif");
	strcpy(cfg_k.filetypes[ftctr++].type, "image/gif");
	strcpy(cfg_k.filetypes[ftctr].extension, ".bmp");
	strcpy(cfg_k.filetypes[ftctr++].type, "image/bmp");

	strcpy(cfg_k.dir_index[0].filename, "index.html");
	strcpy(cfg_k.dir_index[1].filename, "index.htm");
	
#undef ftctr 
	ret = queue_delayed_work(kweb_wq, &monitor_start, 100);
		

	LOGK("Kweb module init successful\n");
out:
	return rv;
}
void kweb_module_cleanup(void)
{
	/*clean up proc fs entries*/
	remove_proc_entry(DIR_INDEX_ENTRY, KWEB_ENTRY);
	remove_proc_entry(DOCUMENT_ROOT_ENTRY, KWEB_ENTRY);
	remove_proc_entry(LISTEN_PORT_ENTRY, KWEB_ENTRY);
	remove_proc_entry(CONTENT_TYPE_ENTRY, KWEB_ENTRY);
	remove_proc_entry("kweb", NULL);

	/* gracefully shut down the web server*/
	start = 0;
	if(cleanup)
	{
		cancel_delayed_work(&svr);
		flush_workqueue(http_wq);
		KSOCKCLOSE(listen_fd);
		destroy_workqueue(http_wq);
		server_started = 0;
	}
	
	cancel_delayed_work(&monitor_start);
	flush_workqueue(kweb_wq);
	destroy_workqueue(kweb_wq);
	LOGK("Kweb module exit successful..\n");
} 
module_init(kweb_module_init);
module_exit(kweb_module_cleanup);
