///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//////////// *********** Simple File Sync ************* //////////////////
/////////////////////    by Gaurav Dass         //////////////////////////
/////////////////////    dassg@rpi.edu          //////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>       
#include <unistd.h>      
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define PAKTYPE 10
#define FILLEN 512


void conf(int fd)   // Gives a confirmation to the waiting party
{

    char a[10];      // define a variable   
    snprintf(a,10,"%d",42);   // put it into a variable cleanly
    
    if( send(fd, a, 10, 0) < 0 )    // send 42 over the channel
    {
                    if(errno == ECONNRESET) // connection reset handler
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
         printf("\n SEND error!!! \n"); // tell client / server about error
    }
    
}

void ack(int fd)    // waits for a response after a successful operation
{
    int nbuf;
    int a[10];
    if((nbuf = recv(fd, a, 10, 0)) < 0) // receive 42 over the channel
    {
            printf("\n RECV error!!! \n");      // tell client / server about receive error
                    if(errno == EAGAIN)
        {
            printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
            exit(1);
        }
        
        if(errno == ECONNRESET)             // connection reset!!
        {
            printf("\n CONNECTION RESET!!!\n");
            exit(1);
        }
    }
//     a[10] = '\0';
    
}

int is_valid(char * b)      // parses a string to see if there is a number within also incase of any alphanumerics present, it discards the string (not a clean string) Basically a wrapper around strtol to make it more robust
{    
    char * pu;
    int ret = strtol(b,&pu,10);     // do a simple strtol to get number if present
        
    if(ret == 0)                    // if return value zero then not valid string
    {
        return 0;
    }
    char * p = NULL;
    for(p = b; (*p) !='\0'; p++ )                // invalid string in form of alphanumerics may be given, so checks for any alphabets along with numbers
    {                                                   // if finds any immediately returns 0                 
        if( ( ((*p) >= 'a') && ( (*p) <= 'z' ) ) || (  ((*p) >= 'A') && ( (*p) <= 'Z' )  ))
        {
            return 0;
        }

        if( !( ((*p) >= '0') && ( (*p) <= '9' ) )  )    // anything that is not a number DISCARD
        {
            return 0;
        }
        
    }
    
    return ret;                                          // returns the number parsed out of the string 
}

int do_socket_stuff_TCP_server( int port_no)        // creates a socket for TCP SERVER operations
{
    struct sockaddr_in servaddr;                                    
    int file_on = 1;
    int lisfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // TCP socket created
    if(setsockopt(lisfd, SOL_SOCKET, SO_REUSEADDR , (char*)&file_on, sizeof(int)) < 0)      // want to reuse socket later on again
    {
        printf("\n SETSOCKOPT FAILED ERRNO :::  %d \n",errno);
    }
    bzero( &servaddr, sizeof(servaddr));  // zero the socket address structure 

    servaddr.sin_family = AF_INET;       
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port_no); // servaddr object structure defined                     
    socklen_t s_len = sizeof(servaddr); 
    
    if(bind(lisfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)     // binding the socket
    {
        printf("\nBIND FAILED!!!! ERRNO: %d\n",errno);
    }
    getsockname(lisfd,(struct sockaddr *)&servaddr,&(s_len)); 
    return lisfd;
}

char * getMD5(char *filla) // makes the hash and tucks it neatly into a string
{
    unsigned char hash[MD5_DIGEST_LENGTH];      // md5 hash container
    unsigned char fbuf[2048];                   // file buffer fragments
    int nread;
    char *hass;
    int i;
    hass = (char*)malloc(33);
    MD5_CTX mdContext;
    
    FILE *filep = fopen (filla, "rb");      // open file for reading
    MD5_Init(&mdContext);

    while((nread = fread (fbuf,1,2048,filep)) != 0) /// read file
    {
    MD5_Update(&mdContext, fbuf, nread);        // generating the hash over the whole file
    }

    fclose(filep);      // close file

    MD5_Final(hash,&mdContext);     // create the hash

    for(i=0;i<16;i++) // convert the hash into string format
    {
    snprintf(&(hass[i*2]),16*2,"%02x",(unsigned int)hash[i]);   // putting the hash into a string to save headache
    }
    
    return hass;
    
}

void getty(int commfd,char * fila)    // for all your getting needs (READ from source) / recv
{   // a wrapper around recv to allow file reception, file overwritten at each go
    
    char rbuf[2048];    // file buffer 
    char nbytes[10];    // filesize container
    
    FILE *fp = fopen(fila, "w+");
    
    if(fp == NULL)
    {
        printf("\n FILE OPENING ERROR \n");
        return;    
    }
    
    bzero(rbuf, 2048);  
    bzero(nbytes, 10);  // zero it all before using
    
    int nbuf = 0;
    // recv filesize
    
    if((nbuf = recv(commfd, nbytes, 10, 0)) < 0) // recv filesize first, if this is not zero proceed with receiving
    {
            printf("\n RECV ERROR!!! \n");
                    if(errno == EAGAIN)
        {
            printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
            exit(1);
        }
        
        if(errno == ECONNRESET)     // connection reset
        {
            printf("\n CONNECTION RESET!!!\n");
            exit(1);
        }
    }
    nbytes[9] = '\0';           // set last bit to '\0'
    
    char *uu;
    int fsiz = strtol(nbytes,&uu,10);
    
//     printf("\nFilesize received %d \n",fsiz);
    
    if (fsiz)       // get file only if filesize not zero
    {
    
    while((nbuf = recv(commfd, rbuf, 2048, 0)) > 0)     // receive the file over the communication channel
    {           
//         printf("\nFile RECVD::: %d \n",nbuf);
        int wbuf = fwrite(rbuf,sizeof(char),nbuf,fp);   // write the file from the buffer
        if(wbuf < nbuf)         // if the write buffer is less than the number of bytes received then
        {

        printf("\n WRITE FAILED!! TRYING AGAIN!!! \n");
        wbuf = fwrite(rbuf,sizeof(char),nbuf,fp);
        if(wbuf < nbuf)
        {
        printf("\n FAILED!!! EXITING!! \n");
        exit(1);
        }
        
        }
        
        if(nbuf < 2048)             // The last packet is always lesser than N bytes
        {
            break;
        }
        bzero(rbuf, 2048);
    }
    if(nbuf < 0)                    // recv failed!
    {   
       printf("\n RECV failed!! getty \n");

        if(errno == EAGAIN)
        {
            printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
            exit(1);
        }
        
        if(errno == ECONNRESET)
        {
            printf("\n CONNECTION RESET!!!\n");
            exit(1);
        }
            return;
    }
    }
    
//     printf("\nFile List Obtained!!! \n");
    fclose(fp); 
}

void putty(int commfd,char * fila)    // for all your putting needs (WRITE to source) / send
{ // a wrapper around send to allow file sending, file must already exist
    struct stat st;
    stat(fila, &st);
    int fils = st.st_size;
    char nbytes[10];
    snprintf(nbytes,10,"%d",fils);
    
//     printf("\n putting file %s \n",fila);
    char sbuf[2048]; 
    FILE *fs = fopen(fila, "r");
    if(fs == NULL)
    {
        printf("\n FILE NOT FOUND!!!! \n");
        return;
    }

    bzero(sbuf, 2048); 
    int nbuf; 
    // send filesize
    if( send(commfd, nbytes, 10, 0) < 0 )       // send filesize if filesize not zero proceed with sending
    {            if(errno == ECONNRESET)
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
         printf("\n SEND OVER!!! \n");
    }
    

    
    if(fils)            // send file only if filesize not zero
    {
    
    while((nbuf = fread(sbuf, sizeof(char), 2048, fs)) > 0)
    {

        if(send(commfd, sbuf, nbuf, 0) < 0)
        {
                        if(errno == ECONNRESET)
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
            
            printf("\n Failed to send file!!!! TRYING AGAIN \n");
            if(send(commfd, sbuf, nbuf, 0) < 0)
            {
            printf("\n SENDING FAILED!!! CONNECTION ERROR!!!! BYE!! \n");
            return;
            }
        }
        
        if(nbuf < 2048)         // Last packet has filesize less than N
        {
            break;
        }
        
        bzero(sbuf, 2048);
    }
    
        if(nbuf < 0)            // fread errror! 
    {
       printf("\n fread failed errno %d \n",errno);
/*
            return;*/

    }
    
    }
    fclose(fs);
}

void get_file_list(char * motti, char *d)
{
    FILE * fp = NULL;
    struct dirent *pent = NULL;
    char cwd[300];
    getcwd(cwd, sizeof(cwd));
 
    DIR *pdir = NULL;
    pdir = opendir(motti); 
    int i = 1;

    snprintf(d,100,"%s/.4220_file_list.txt",motti);             // determine full directory for a file! 
    if(access( d, F_OK ) != -1 ) {remove(d);}   // if file already exists remove it

    fp = fopen(d,"w+");                                     // using w+ to force overwriting the files!
    if(pdir ==  NULL){printf("\nFile Directory Inaccessible!!\n");exit(1);}	
    
    while( (pent = readdir(pdir)) )
    {
        if(pent == NULL){printf("\nCheck Your files / you may not have permission to access this folder. \n"); exit(1);}
        char filnam[512];
        strcpy(filnam,pent->d_name);
        if(filnam[0] == '.') { continue; }              // skip all the hidden files
        if(pent->d_type != DT_REG){ continue; }         // skip anything that is not a regular file
        
        char fnew[512];
        snprintf(fnew,512,"%s/%s/%s",cwd,motti,filnam);
//         fnew[nnn] = '\0';
        
        struct stat fst;   
        bzero(&fst,sizeof(fst));
  
   if(stat(fnew,&fst) == -1) 
   { 
       printf("stat() failed with errno %d\n",errno); exit(-1); 
       
   }
   fprintf(fp,"\n :=%d:\t\t:#%s:\t\t:.%s:\t\t:+%ld*\n\n",i,filnam,getMD5(fnew),fst.st_mtime);       // writing to file
    //  below is the format followed for ENCODING the file attributes to .4220_file_list
   // fprintf(fp,"\n :=INDEX:\t\t:#FILENAME:\t\t:.HASH:\t\t:+TIMESTAMP*\n\n");
   
   i++;
    }
    fclose(fp);
    closedir(pdir);

}

int read_file_list(FILE * fp, char * hash, int * index, char * filnam, char * time_s)
{   // fprintf(fp,"\n :=INDEX:\t\t:-FILENAME:\t\t:.HASH:\t\t:+TIMESTAMP*\n\n");
    // again above is the format followed while saving
    char c;
    while( (c = fgetc(fp)) != ':' )
    {
        if(c == EOF)    // EOF likely encountered by first traversor
        {
            return 0;
        }
        
    }
    c = fgetc(fp);      // traversing the :=INDEX: pattern
    
    char no[10];
    int l = 0;
        if(c == '=')
        {
            while((c = fgetc(fp)) != ':')
            {
                no[l++] = c;
            }
        }
    char *uu;
    int ind = strtol(no,&uu,10);
    (*index) = ind;            
    
    /*
    if((*index) == 0)
    {
        printf("\n  INDEX is ZERO!! \n");
    }*/

    char ffnam[512];
    bzero(ffnam,512);
    while( (c = fgetc(fp)) != ':' )
    {
        
    }
    c = fgetc(fp);          // Traversing the :#FILENAME: Pattern
    l = 0;
    
        if(c == '#')        
        {
            while((c = fgetc(fp)) != ':')
            {
                ffnam[l++] = c;
            }
        }
    ffnam[l] = '\0';

    snprintf(filnam,512,"%s",ffnam);
//     printf("\nFilename read from .4220_file_list ::: %s\n",filnam);
//     sleep(3);
    
    while( (c = fgetc(fp)) != ':' )
    {
        
    }
    c = fgetc(fp);
    l = 0;
    
        if(c == '.')        // Traversing the :.HASH: Pattern
        {
            while((c = fgetc(fp)) != ':')
            {
                hash[l++] = c;
            }
        }
    

    while( (c = fgetc(fp)) != ':' )
    {
        
    }
    c = fgetc(fp);
    l = 0;
    
        if(c == '+')
        {
            while((c = fgetc(fp)) != '*')
            {
                time_s[l++] = c;    // Traversing the :+TIMESTAMP* Pattern
            }
        }
//         char * u;
//      long ui = strtol(time_s,&u,10);
        
//     printf("\nIndex: %d Filename %s Hash %s Timestamp %ld \n",(*index), filnam, hash, ui);
//     sleep(2);
    
    return 1;
}


void run_server(int port_no) // runs the server
{
// FILE *fp = NULL;
char temp[] = "motti.XXXXXX";   
char *tmp = mkdtemp(temp);  // temporary directory creation
char rbuf[PAKTYPE];         
char fnbuf[FILLEN];         
char hashF[33];         

if(tmp == NULL)    
{
    printf("\n TMPDIR Creation Error!!! BYE!!! \n");
    exit(1);
}
else
{
    printf("\ntmpdir created in %s \n \n",tmp); // printing out tempdir name to make it easy for the user to test this 
}

    int lisfd = do_socket_stuff_TCP_server(port_no);    // get TCP socket

    while(1)
    {
    
    char d[100];
    bzero(d,100);

    get_file_list(tmp,d);   // generate a NEW .4220_file_list

    if(listen(lisfd, 10) < 0)                          // listen for clients
    {
        printf("\n Listening Failed!!! \n");
    }

    int comm;
    socklen_t sin_size = sizeof(struct sockaddr_in);  
    struct sockaddr_in addr_remote;

    if((comm = accept(lisfd, (struct sockaddr *)&addr_remote,&sin_size )) < 0 )
    {   // accepting the connection
        printf("\n ERROR IN ACCEPT!!! \n");
    }

    putty(comm,d);        // send client .4220_file_list
    int serv_cli = 1;
    
        while(serv_cli)     // servicing one client
        {
            
        int puff,ret,puff2,puff3;     
                
            bzero(rbuf,PAKTYPE);
            if((puff = recv(comm, rbuf, PAKTYPE, 0)) < 0) // get packet type from client
            {
                
        if(errno == EAGAIN)
        {
            printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
            exit(1);
        }
        
        if(errno == ECONNRESET)
        {
            printf("\n CONNECTION RESET!!!\n");
            exit(1);
        }
        return;
            }
            
            bzero(fnbuf,FILLEN);
            if((puff2 = recv(comm, fnbuf, FILLEN, 0)) < 0) // get filename from client to perform that operation on
            {
            printf("\n RECV failed!! puffy2 with errno %d \n",errno);

            if(errno == EAGAIN)
            {
                printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
                exit(1);
            }
            
            if(errno == ECONNRESET)
            {
                printf("\n CONNECTION RESET!!!\n");
                exit(1);
            }
                        return;
                    
            }
            
            bzero(hashF,33);            
            if((puff3 = recv(comm, hashF, 33, 0)) < 0) // get hash from client to print 
            {
            printf("\n RECV failed!! puffy2 with errno %d \n",errno);

            if(errno == EAGAIN)
            {
                printf("\n TIMEOUT! CHECK YOUR CONNECTION!!\n");
                exit(1);
            }
            
            if(errno == ECONNRESET)
            {
                printf("\n CONNECTION RESET!!!\n");
                exit(1);
            }
                        return;
                    
            }
            
            hashF[32] = '\0';                   // adding this \0 character to denote last element in the array
            
                ret = is_valid(rbuf);   // cleanly parse the packet type
                char sfname[512];
                snprintf(sfname,512,"%s/%s",tmp,fnbuf);    // snprintf to get the name for the file

                switch (ret)
                {
                    case 1: 
                        // concatenate directory before fnbuf
                        putty(comm,sfname);
                        ack(comm);
                        // client wants missing files give client files (put for server)
                            break;
                    case 2:
                        // concatenate directory before fnbuf
                        // server has those files in the temporary folder
                        putty(comm,sfname);
                        ack(comm);
                        // client wants ood files give client files (put for server)
                            break;
                    case 3:
                        // concatenate directory before fnbuf
                        printf("[server] Detected different & newer file: %s \n",sfname);                        printf("[server] Downloading %s : %s \n",sfname,hashF);
                        getty(comm,sfname);
                        conf(comm);
                        // server needs to get NEW files from client  (get for server)

                            break;
                    case 4:
                        // concatenate directory before fnbuf
                        printf("[server] Detected different & newer file: %s \n",sfname);                        printf("[server] Downloading %s : %s \n",sfname,hashF);
                        getty(comm,sfname);
                        conf(comm);
                        // server has older files (get from client)
                            break;
                    case 5:
                        serv_cli = 0;// client wants to leave, say goodbye
                        shutdown(comm, SHUT_WR);
                        close(comm);
                            break;
                    default:  shutdown(comm, SHUT_WR); close(comm); printf("\n Restarting Server!! \n ");     serv_cli = 0;
                 
                }
                            
            if(serv_cli == 0)       // server has finished servicing this client!
            {
                printf("\n[server] Current Client operations finished, going back to listening\n\n\n");
            }
            
        }
            
    }
    
//   fclose(fp);  
}

int take_action(char * hash, int * index, char * filnamE, char * time_s, char * exe)    // for handling cases 1 , 2 and 4
{
    // ALL CASES EXPLAINED BELOW
    /*
        case 1: // client wants missing files give client files (put for server)
                break;
        case 2: // client wants ood files give client files (put for server)
                break;
        case 3: // server needs to get NEW files from client  (get for server)
                break;
        case 4: // server has older files (get from client)
                break;
        case 5: // client wants to leave, say goodbye

     */
    
    char cwd[512];
    getcwd(cwd, sizeof(cwd));   // get current working directory needed for stat
    int found_flag = 0;
    int paktype = 1;            // default paktype is always 1
    struct stat fst;  
    
    DIR *pdir = NULL;
    pdir = opendir(cwd);    // open current working directory
    int i = 1;
    char filnam[512];
    if(pdir ==  NULL){printf("\nFile Directory Inaccessible!!\n");exit(1);}	

    struct dirent *pent = NULL;
    while( (pent = readdir(pdir)) )       // traversing all the client files
    {
        if(pent == NULL){printf("\nCheck Your files / you may not have permission to access this folder. \n"); exit(1);}

        bzero(filnam,512);
        strcpy(filnam,pent->d_name);// get a filename
//         filnam = pent->d_name;
        if(filnam[0] == '.') { continue; }      // skip hidden files
        if(strcmp("file_sync.c",filnam) == 0){continue;}    // skip the metadata
        if(strcmp(exe,filnam) == 0){continue;}    // skip the executable
        if(pent->d_type != DT_REG){ continue; } // skip anything not a regular file
        
        if(strcmp(filnam,filnamE) == 0)     // is this file there?
        {
            found_flag = 1;
            break;
        }
        
            
   i++;
    }
    
    if(found_flag)  // file found somewhere! what next?
    {
        char *hashcurr = getMD5(filnam);        // get hash for the current file
        
        if(strcmp(hashcurr,hash))   // are the hashes same?
        {
        bzero(&fst,sizeof(fst));
        
        char fnam[512];
//         printf("\ncwd %s\n",cwd);
//         printf("\nfnam %s\n",filnam);
        snprintf(fnam,512,"%s/%s",cwd,filnam);
//         fnam[nn] = '\0';
        
        if(stat(fnam,&fst) == -1){ printf("stat() failed with errno %d\n",errno); exit(-1);}
        char *l;
        long orit = strtol(time_s,&l,10);
        if(fst.st_mtime > orit) // is this file newer?
        {
            paktype = 4;    // server has older files give server new files
        }
        else
        {
            paktype = 2;    // client has older version, get from server
        }
            
        }
        else
        {
            paktype = 109;      // this file is same skip this file (hash same)
        }
    }
    else
    {
        paktype = 1;        // client does not have this file
    }
    closedir(pdir);
    return paktype;
}

void client_process_pak(int pakno, char filnam[], int commfd, char * hash)
{   // processes all packet types for the client
    
    char sbuf[PAKTYPE];                     
    char FNAM[FILLEN];                      
    char hashF[33];                         
    
    snprintf(sbuf, PAKTYPE, "%d", pakno);    // wanted to be absoutely sure in cleanly formatting the filenames and packet type
    snprintf(FNAM, FILLEN, "%s", filnam);
    snprintf(hashF, 33, "%s", hash);

        if(send(commfd, sbuf, PAKTYPE, 0) < 0)   // sending packet type
        {
            if(errno == ECONNRESET)
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
            printf("\n send error!!  \n");
        }
        
        if(send(commfd, FNAM, FILLEN, 0) < 0)   // sending filename
        {
                        if(errno == ECONNRESET)
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
            printf("\n send error!!  \n");
        }


        if(send(commfd, hashF, 33, 0) < 0)   // sending hash
        {
            if(errno == ECONNRESET)
            {
                printf("\n Connection Reset!! \n");
                exit(1);
            }
            printf("\n send error!!  \n");
        }
        
    
    if(pakno == 1)
    {
        // these messages need to be printed only for the client
        printf("[client] Detected different & newer file: %s\n",FNAM);
        printf("[client] Downloading %s : %s\n",FNAM,hashF);
        
        getty(commfd,FNAM); // get file from server
        conf(commfd);       // send response (SERVER WAITS)
    
    }
    else if(pakno == 2)
    {
       
        printf("[client] Detected different & newer file: %s\n",FNAM);
        printf("[client] Downloading %s : %s\n",FNAM,hashF);
        
        getty(commfd,FNAM); // get file from server
        conf(commfd);       // send response (SERVER WAITS)
    
    }
    else if(pakno == 3)
    {
        putty(commfd,FNAM); // put file to server
        ack(commfd);        // get response (CLIENT WAITS)
    }
    else if(pakno == 4)
    {

        putty(commfd,FNAM); // put file to server 
        ack(commfd);        // get response (CLIENT WAITS) 
    }
    else if(pakno == 5)
    {

        close(commfd);      // close all comunication
    }
    else
    {
        
        printf("\n This is a lengthy program, some mistake occured!! \n");
        // there is something wrong with your system if you encounter this error
        // packettypes are hardcoded into the program itself, at this stage decided by the client, the only cause that can give them an unknown value at this point in the program is memory corruption
    }

    
    
}

void run_client(int port_no, char * exe)    // runs the client (DOES BULK OF THE WORK)
{
    FILE *fp = NULL;
    char d[] = ".4220_file_list.txt";

    struct sockaddr_in rem;
    
    int connfd;
    if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create socket
    {
        printf("\n SOCKET CREATION ERROR!!!! \n");
        exit(1);
    }
//     printf("\n POr NO:: %d \n",port_no);
    rem.sin_family = AF_INET;                               
    rem.sin_port = htons(port_no);                          
    inet_pton(AF_INET, "127.0.0.1", &rem.sin_addr);         
    rem.sin_addr.s_addr = inet_addr("127.0.0.1");   // remote socket address struct
    bzero(&(rem.sin_zero), 8);                  // do socket stuff for the client
    
    
    if (connect(connfd, (struct sockaddr *)&rem, sizeof(struct sockaddr)) == -1)
    {
        printf("\nError Connecting to server , bye! You're doing something wrong, DELETE EVERYTHING AND RESTART ERRNO:: %d \n",errno);
        exit(1);
    }
    
    if( access( d, F_OK ) != -1 ) {remove(d);} // .4220_file_list exists, remove file

    getty(connfd,d);                           // get the file list
    fp = fopen(d,"r");                         // open in only read mode
//     sleep(7);
    
    while(1)
    {
        // TRAVERSE EACH OF THE FILENAMES FROM THE RECEIVED FILE LIST (.4220_file_list)
        // THAT IS, traverse each server file and match it against the client files
        char hash[33],filnamE[512],time_s[30];
        int index;
    
        if(!read_file_list(fp, hash,&index, filnamE, time_s))           // reading each server file received in form of a file list
        {
            break;
        }
       int pak = take_action(hash, &index,filnamE,time_s,exe);   // 1,2 or 4
       if(pak == 109){continue;};   // if 109 then skip that file
       client_process_pak(pak,filnamE, connfd,hash); // process packet as explained before
       // process pak
    }
    
  fclose(fp);  

  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  // ^^^^^ The above has cases for client to server file transfer
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  // vvvvv The below has a case for server to client file transfer
  ///////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////
  
//   printf("\n Now processing files in the client in the server does not have!!! \n");
//   sleep(5);
  
    char filnam[512];
    char cwd[512];
    getcwd(cwd, sizeof(cwd));       // get current working directory again
   
    DIR *pdir = NULL;
    pdir = opendir(cwd);            // getting current directory
    int i = 1;
    if(pdir ==  NULL){printf("\nFile Directory Inaccessible!!\n");exit(1);}	


    struct dirent *pent = NULL;
    
    while( (pent = readdir(pdir)) )   // TRAVERSE EACH CLIENT FILE AND MATCH AGAINST EACH SERVER FILE, LOOK FOR FILES NOT HAVE HAD BY THE SERVER
    {
    int found_flag = 0;
    if(pent == NULL){printf("\nCheck Your files / you may not have permission to access this folder. \n"); exit(1);}
//     filnam = pent->d_name;
    bzero(filnam,512);
    strcpy(filnam,pent->d_name);
    if(filnam[0] == '.') { continue; }  // skip hidden files (skips the .4220_file_list)
    if(strcmp("file_sync.c",filnam) == 0){continue;} // skip the metadata (filename)
    if(strcmp(exe,filnam) == 0){continue;}    // skip the executable (metadata)
    if(pent->d_type != DT_REG){ continue; } // skip anything not a regular files
    
    FILE *fp2 = NULL;
    char d[] = ".4220_file_list.txt";
    fp2 = fopen(d,"r");
    
        // USING THIS CLIENT FILE ... ... 
        while(1)
        {
        // ... TRAVERSE ALL THE SERVER FILES and look for anything that the server does not have at all, if so, give it to the server
        char hash[33],filnamE[512],time_s[30];
        int index;
        
        if(!read_file_list(fp2, hash,&index, filnamE, time_s))      // reading all the server files for each client file
        {
            break;
        }
                        
        if(strcmp(filnam,filnamE) == 0)                                 // checking each server file against this particular client file
        {
            found_flag = 1;
            break;
        }
        
        }
        
        fclose(fp2);
        
        if(found_flag == 0)     // client file not found in the server directory
        {   
//    Sending file to the server which it does not have
            int paktype = 3;
            client_process_pak(paktype,filnam, connfd,getMD5(filnam));
       
            // process pak
            
        }
            
   i++;
    }
    // shutting things down!!
    closedir(pdir);
    client_process_pak(5,filnam, connfd,getMD5(filnam));   // finished sync telling server to close connection goodbye
    

    
}

int main(int argc, char * argv[])   // main
{

if(argc != 3)       // No wrong arguments
{
    printf("\n ARGC::: %d \n",argc);
    printf ("\nThis is an invalid input \nPLEASE ENTER::\n ./file server [listening port #] for creating a server instance OR ./file client [comm port #] for creating a client instance \n");
    exit(1);
}

int port_no = is_valid(argv[2]);    /// Parse input

if( (strcasecmp(argv[1],"server") == 0) && (port_no != 0) )
{
    run_server(port_no);    // cannot allow port number to be zero, also runs server at port number given by port_no
}
else if( (strcasecmp(argv[1],"client") == 0) && (port_no != 0) )
{
    char * exe;
    exe = argv[0];
    char exec[100];
    snprintf(exec,100,"%s",exe+2);      // need to remove the "./" from the executable file for eg. ./ser_c needs to be parsed to only obtain ser_c
    run_client(port_no,exec);    // runs the client and directs it to port number running at port_no
}
else
{
    printf ("\nThis is an invalid input \nPLEASE ENTER::\n ./file server [listening port #] for creating a server instance OR ./file client [comm port #] for creating a client instance. Please run the server before the client and make sure the port numbers are not 0 and are equal. Thank you, have a nice day! \n");
    exit(1);
}
    
return 1;
}
