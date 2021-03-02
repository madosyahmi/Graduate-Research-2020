#include <stdio.h>        /*printf()*/
#include <stdlib.h>       /*exit()*/
#include <string.h>       /*strcmp(),strcpy()*/
#include <unistd.h>       /*close()*/
#include <sys/types.h>    /*setsockopt()*/
#include <sys/socket.h>   /*socket(),bind(),connect(),accept()*/
#include <arpa/inet.h>    /*sockaddr_in,inet_ntoa()*/
#include <netinet/in.h>
#include <sys/time.h>     /*struct timeval*/
#include <openssl/sha.h>
#include <sys/stat.h>

#define MAXPENDING 5
#define AUTHDATA_BYTE 32


char str1[] = "Auth_file.bin";

FILE *fp;
SHA256_CTX sha_ctx;

//関数プロトタイプ宣言
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr);
void InitRegistration(int clntSock,unsigned char *clntIpAddr);
int Authentication(int clntSock,char *id);
int VernamCipher(int clntSock,char *id);
void AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);

//クライアントが登録済みのユーザかをチェックする関数
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr){
  unsigned char dataLine[50]={'\0'}; //userdataの1行分を保持
  unsigned char search[20]={'\0'};   //探索するIPアドレスを保持
  char id[33]={'\0'};                //識別子を保持
  char *idp;                         //識別子用のポインタ
  char *ipp;                         //ipアドレス用のポインタ
  unsigned short authent_flag=0;
  
  if((fp=fopen("./../server_data/UserData.txt","r"))==NULL){
    perror("User FILE OPEN ERROR\n");
    exit(1);
  }
  else{
    strcpy(search,inet_ntoa(clnt_addr->sin_addr));
    printf("探索データ:%s\n",search);
	  
    while(fgets(dataLine,50,fp)){
      printf("探索中のデータ:%s\n",dataLine);
	  
      //探索対象を識別子に設定
      strtok(dataLine,",");
      ipp=strtok(NULL,"\n");
      printf("読み込んだデータ:%s\n",ipp);
      puts("---------");
      if(strcmp(ipp, search)==0){
	idp = strtok(dataLine,",");
	puts("----------");
	printf("識別子は%s\n", idp);
	authent_flag = 1;
	break;
      }
      dataLine[0]='\0';
    }
  }    
  fclose(fp);

  //flagを送信
  write(clntSock, &authent_flag, sizeof(unsigned short));
  
  if(authent_flag == 0){
    puts("識別子は見つかりませんでした");
    InitRegistration(clntSock,search);
  }
  else if(authent_flag == 1){
    printf("識別子が見つかりました\n");
    strcpy(id,idp);

    if(Authentication(clntSock,id)==0){
      VernamCipher(clntSock,id);
    }
  }
  else{
    perror("flag error\n");
    exit(1);
  }
}

//初期登録陽関数
void InitRegistration(int clntSock,unsigned char *clntIpAddr){
  char id[33]={'\0'};
  unsigned char addUserData[50]={'\0'};/*id,ip*/
  unsigned char A[AUTHDATA_BYTE]={'\0'};
  
  char filename1[50]={'\0'};
  char foldername[]={'\0'};

  register int i;
  unsigned short length;

  //IDのデータ長を記録
  read(clntSock, &length, sizeof(unsigned short)); 
  printf("%d\n",length);
  
  //IDを受け取る
  read(clntSock,id,length);

  //UserDataに新たなユーザ情報を記録
  if((fp=fopen("./../server_data/UserData.txt","a")) == NULL){
    perror("User FILE OPEN ERROR\n");
    exit(1);
  } 
  sprintf(addUserData,"%s,%s\n",id,clntIpAddr);
  fprintf(fp,"%s",addUserData);
  fclose(fp);
  addUserData[0]='\0';

  //新しいユーザの認証情報、メッセージを保存するフォルダを作成
  sprintf(foldername,"./../server_data/%s",id);  
  if(mkdir(foldername,0777)==0){
    printf("フォルダ作成に成功しました。\n");
  }else{
    printf("フォルダ作成に失敗しました。\n");
  }
  
  //Aを受け取る
  read(clntSock,A,AUTHDATA_BYTE);
  ////////////////////////
  //
  printf("A->");
  for(i=0; i<sizeof(A); i++){
    printf("%02x",A[i]);
  }
  printf("\n");
  //
  ////////////////////////

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1);
  
  //Aを保存する
  printf("ファイル名=%s\n",filename1);
  if((fp=fopen(filename1,"wb")) == NULL){
    perror("Auth FILE OPEN ERROR\n");
    exit(1);
  }
  fwrite(A,sizeof(A[0]),AUTHDATA_BYTE,fp);
  fclose(fp);
   
  printf("Aをファイルに保存\n\n");	
}

//認証用関数
int Authentication(int clntSock,char *id){
  unsigned char A[AUTHDATA_BYTE]={'\0'};
  unsigned char C[AUTHDATA_BYTE]={'\0'};
  unsigned char D[AUTHDATA_BYTE]={'\0'};
  unsigned char E[AUTHDATA_BYTE]={'\0'};
  unsigned char alpha[AUTHDATA_BYTE]={'\0'};
  unsigned char beta[AUTHDATA_BYTE]={'\0'};
  unsigned char gamma[AUTHDATA_BYTE]={'\0'};

  char filename1[50]={'\0'};
  unsigned short authent_flag=0;

  register int i;

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1);
  
  //Aを読み取る
  if((fp=fopen(filename1,"rb")) == NULL){
    printf("Ai FILE OPEN ERROR\n");
    return -1;
  }
  fread(A,sizeof(A[0]),AUTHDATA_BYTE,fp);
  fclose(fp);
    
  //////////////////////////////
  //
    printf("read_A->");
    for(i=0; i<sizeof(A); i++){
    printf("%02x",A[i]);
    }
    printf("\n");
  //
  /////////////////////////////
    
  //alpha を受け取る
  read(clntSock,alpha,AUTHDATA_BYTE);
  ////////////////////////////
  //
    printf("alpha->");
    for(i=0; i<sizeof(alpha); i++){
    printf("%02x",alpha[i]);
    }
    printf("\n");
  //
  ////////////////////////////
    
  //beta を受け取る
  read(clntSock,beta,AUTHDATA_BYTE);
  ////////////////////////////
  //
    printf("beta->");
    for(i=0; i<sizeof(beta); i++){
    printf("%02x",beta[i]);
    }
    printf("\n");
  //
  ////////////////////////////
    
  //Cを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    C[i]=beta[i]^A[i];
    i++;
  }
    
  ////////////////////////
  //
    printf("C->");
    for(i=0; i<sizeof(C); i++){
    printf("%02x",C[i]);
    }
    printf("\n");
  //
  ////////////////////////
    
  //Dを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    D[i]=alpha[i]^(C[i]+A[i]);
    i++;
  }
    
  ////////////////////////
  //
    printf("D->");
    for(i=0; i<sizeof(D); i++){
    printf("%02x",D[i]);
    }
    printf("\n");
  //
  ////////////////////////
    
  //Eを作成する
  SHA256_Init(&sha_ctx);
  SHA256_Update(&sha_ctx, D, sizeof(D));
  SHA256_Final(E, &sha_ctx);
    
  /////////////////////////////////
  //
    printf("E->");
    for (i = 0; i<sizeof(E); ++i) {
    printf("%02x", E[i]);
    }
    printf("\n");
  //
  /////////////////////////////////
    
  //CとEを比較
  for(i=0; i<AUTHDATA_BYTE; i++){
    if(E[i]!=C[i]){
      authent_flag=1;
    }
  }
    
  if(authent_flag==1){
    puts("C and E are not equal");
    perror("authentication error!\n");
    return -1;
  }
  else if(authent_flag==0){
    //puts("C and E are equal");
      
    //D(Ai+1を保存する)
    if((fp=fopen(filename1,"wb")) == NULL){
      perror("Auth FILE OPEN ERROR\n");
      return -1;
    }	  
    fwrite(D,sizeof(D[0]),AUTHDATA_BYTE,fp);
    fclose(fp);
      
    //gammaを作成
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, C, sizeof(C));
    SHA256_Final(gamma, &sha_ctx);
      
    ////////////////////////////////
    //
      printf("gamma->");
      for (i = 0; i<sizeof(gamma); ++i) {
      printf("%02x", gamma[i]);
      }
      printf("\n");
    //
    /////////////////////////////////

    //gammaを送信
    write(clntSock,gamma,AUTHDATA_BYTE);
  }
  return 0;
}

int VernamCipher(int clntSock,char *id){
  char filename1[50]={'\0'};
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'};
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};
  register int i;
  unsigned short length;
  
  puts("-------------------");

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1);
  
  //認証情報読み取り
  if((fp=fopen(filename1,"rb")) == NULL){
    printf("Auth FILE OPEN ERROR\n");
    exit(1);
  }
  fread(vernamKey,sizeof(vernamKey[0]),AUTHDATA_BYTE,fp);
  fclose(fp);

  ////////////////////////////////
  //
  printf("vernamKey->");
  for(i=0; i<AUTHDATA_BYTE; i++){
    printf("%02x",vernamKey[i]);
  }
  printf("\n");
  //
  ///////////////////////////////

  //受け取る暗号文のデータ長を記録
  read(clntSock, &length, sizeof(unsigned short));

  //暗号文を受け取る
  read(clntSock, vernamData, length);

  //////////////////////////////////
  //
  printf("暗号文:");
  for(i=0; i<length; i++){
    printf("%02x",vernamData[i]);
  }
  printf("\n");
  //
  ////////////////////////////////
  
  //復号
  i=0;
  while(i<length){
    vernamData[i]^=vernamKey[i];
    i++;
  }
  vernamData[i]='\0';

  sprintf(filename1,"./../server_data/%s/%s_msg.txt",id,id);

  if((fp=fopen(filename1,"w")) == NULL){
    printf("Msg FILE OPEN ERROR\n");
    exit(1);
  }
  fwrite(vernamData,sizeof(unsigned char),strlen(vernamData),fp);
  fclose(fp);

  ///////////////////////////////////
  //
  printf("復号文：%s\n",vernamData);
  puts("復号文をファイルに保存しました");
  //
  //////////////////////////////////
  return 0;
}


//クライアントからの接続を受入関数
void AcceptTCPConnection(int servSock){
  int clntSock;
  unsigned int clntLen;
  struct sockaddr_in clnt_addr;

  int rcvBufferSize=100*1024;
  int sendBufferSize=100*1024;
  
  clntLen=sizeof(clnt_addr);

  if((clntSock=accept(servSock,(struct sockaddr *)&clnt_addr,&clntLen))<0){
    perror("accept() failed\n");
    exit(1);
  }

  printf("Accepted connection from a client %s\n",inet_ntoa(clnt_addr.sin_addr));

  //バッファサイズを設定
  if(setsockopt(clntSock,SOL_SOCKET,SO_RCVBUF,&rcvBufferSize,sizeof(rcvBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }
  if(setsockopt(clntSock,SOL_SOCKET,SO_SNDBUF,&sendBufferSize,sizeof(sendBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }

  UserDataSearch(clntSock,&clnt_addr);
  close(clntSock);
}

int CreateTCPServerSocket(unsigned short port){
  int sock;
  struct sockaddr_in server_addr;

  if((sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
    perror("socket() failed\n");
    exit(1);
  }

  //構造体server_addrの設定
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  //サーバのIPアドレスとポート番号をバインド
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind() failed\n");
    exit(1);
  }

  //接続待機開始
  if(listen(sock, MAXPENDING)<0){
    perror("listen() failed\n");
    exit(1);
  }
  
  return sock;
}

int main(int argc, char **argv){
  int *servSock;
  int maxDescriptor;
  fd_set watchFds;
  long timeout;
  struct timeval selTimeout;
  int running=1;
  int noPorts;
  register int port;
  unsigned short portNo;

  if(argc<3){
    fprintf(stderr,"Usage: %s <Timeout (secs.)> <Port 1>...\n",argv[0]);
    exit(1);
  }
  
  timeout=atol(argv[1]);
  noPorts=argc-2;

  servSock=(int*)malloc(noPorts*sizeof(int));
  
  maxDescriptor=-1;
  
  for(port=0; port<noPorts; port++){
    portNo=atoi(argv[port+2]);
    servSock[port]=CreateTCPServerSocket(portNo);
    
    if(servSock[port]>maxDescriptor){
      maxDescriptor=servSock[port];
    }
  }

  printf("Starting server:Hit return to shutdown\n");

  while(running){
    FD_ZERO(&watchFds);
    FD_SET(STDIN_FILENO,&watchFds);
    for(port=0; port<noPorts; port++){
      FD_SET(servSock[port],&watchFds);
    }
    
    selTimeout.tv_sec=timeout;
    selTimeout.tv_usec=0;
    
    //監視対象のFDのいずれかが読み込み可能になるまで待機
    if(select(maxDescriptor+1, &watchFds, NULL, NULL, &selTimeout)==0){
      fprintf(stderr,"No authent requests for %ld secs...Server still alive\n",timeout);
      break;
    }
    else{
      if(FD_ISSET(STDIN_FILENO,&watchFds)){
	printf("Shutting down server\n");
	getchar();
	running=0;
      }
      for(port=0; port<noPorts; port++){
	if(FD_ISSET(servSock[port],&watchFds)){
	  printf("Request on port %d: ",atoi(argv[port+2]));
	  AcceptTCPConnection(servSock[port]);
	}
      }	
    }
  }

  for(port=0; port<noPorts; port++){
    printf("client Disconnected and closed port %d\n",atoi(argv[port+2]));
    close(servSock[port]);
  }
  free(servSock);
  return 0;
}
