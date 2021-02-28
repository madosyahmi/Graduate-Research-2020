#include <stdio.h>      /*printf()*/
#include <stdlib.h>     /*exit()*/
#include <string.h>     /*strcmp(),strcpy()*/
#include <unistd.h>     /*close()*/
#include <sys/types.h>  /*setsockopt()*/
#include <sys/socket.h> /*socket(),bind(),connect(),accept()*/
#include <arpa/inet.h>  /*sockaddr_in,inet_ntoa()*/
#include <netinet/in.h>
#include <sys/time.h>   /*struct timeval*/
#include <fcntl.h>      /*open()*/
#include <openssl/sha.h>
#include <sys/stat.h>

#define MAXPENDING 5
#define AUTHDATA_BYTE 32
#define DEV_RANDOM "/dev/urandom"

char str1[] = "Auth_file.bin";

FILE *fp;
SHA256_CTX sha_ctx;

//関数プロトタイプ宣言
int get_random (char * const buf, const int buflen, const int len);
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr);
void InitRegistration(int clntSock,unsigned char *clntIpAddr);
int Authentication(int clntSock,char *username);
int VernamCipher(int clntSock,char *username);
void AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);

int get_random(char * const buf, const int bufLen, const int len){
  //初期エラー
  if(len > bufLen){
    perror("buffer size is small\n");
    return -1;
  }
  
  int fd = open(DEV_RANDOM, O_RDONLY);
  if(fd == -1){
    printf("can not open %s\n", DEV_RANDOM);
    return -1;
  }
  
  int r = read(fd,buf,len);
  if(r < 0){
    perror("can not read\n");
    return -1;
  }
  if(r != len){
    perror("can not read\n");
    return -1;
  }
  
  close(fd);
  return 0;
}

//クライアントが登録済みのユーザかをチェックする関数
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr){
  unsigned char dataLine[50]={'\0'}; //userdataの1行分を保持
  unsigned char search[20]={'\0'};   //探索するIPアドレスを保持
  char id[AUTHDATA_BYTE+1]={'\0'};   //識別子を保持
  char *idp;                         //識別子用のポインタ
  char *ipp;                         //ipアドレス用のポインタ
  unsigned short authent_flag=0;     //処理分岐用フラグ
  
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
	puts("---------");
	printf("識別子は%s\n", idp);
	authent_flag = 1;
	break;
      }
      dataLine[0]='\0';
    }
  }    
  fclose(fp);

  //flagを送信
  write(clntSock,&authent_flag, sizeof(unsigned short));
  
  if(authent_flag==0){
    puts("識別子は見つかりませんでした");
    InitRegistration(clntSock,search);
  }
  else if(authent_flag == 1){
    puts("識別子が見つかりました");
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
  char id[AUTHDATA_BYTE+2]={'\0'};
  unsigned char addUserData[50]={'\0'};/*(id),(ip address)*/
  unsigned char pre_A[AUTHDATA_BYTE]={'\0'};
  unsigned char Ni[AUTHDATA_BYTE]={'\0'};
  unsigned char authData[2][AUTHDATA_BYTE]={'\0'};
  unsigned short overflow_flag=1;

  char filename[50]={'\0'};
  char foldername[]={'\0'};

  register int i;
  unsigned short length;
  
  puts("識別子を設定してください");
  //標準入力を受け取る
  if(fgets(id,AUTHDATA_BYTE+2,stdin)==NULL || id[0]=='\0' || id[0]=='\n'){
    perror("識別子:1-32char\n");
    exit(1);
  }
  for(i=1; i<AUTHDATA_BYTE+2; i++){
    if(id[i]=='\n'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("識別子:1-32char\n");
    exit(1); 
  }
  overflow_flag=1;
  
  //改行を削除
  length=strlen(id);
  if(id[length-1]=='\n'){
    id[--length]='\0';
  }
  
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

  //Niを作成
  get_random(Ni,AUTHDATA_BYTE,AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("Ni->");
  for(i=0; i<sizeof(Ni); i++){
    printf("%02x",Ni[i]);
  }
  printf("\n");
  //
  ////////////////////////////////

  //Mi作成
  get_random(authData[1],AUTHDATA_BYTE,AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("Mi->");
  for(i=0; i<sizeof(authData[1]); i++){
    printf("%02x",authData[1][i]);
  }
  printf("\n");
  //
  ///////////////////////////////

  //Aを作成
  i=0;
  while(i<AUTHDATA_BYTE){
    pre_A[i]=id[i]^Ni[i];
    i++;
  }
  
  SHA256_Init(&sha_ctx);
  SHA256_Update(&sha_ctx, pre_A, sizeof(pre_A));
  SHA256_Final(authData[0], &sha_ctx); 

  //////////////////////////////
  //デバッグ用
  printf("Ni^S->");
  for (i = 0; i<sizeof(pre_A); i++) {
    printf("%02x", pre_A[i]);
  }
  printf("\n");

  printf("A->");
  for (i = 0; i < sizeof(authData[0]); i++) {
    printf("%02x", authData[0][i]);
  }
  printf("\n");
  //
  //////////////////////////////////

  //A,Mを送信
  write(clntSock,&authData[0],AUTHDATA_BYTE);
  write(clntSock,&authData[1],AUTHDATA_BYTE);

  sprintf(filename,"./../server_data/%s/%s%s",id,id,str1);
  
  //A,Mを保存
  printf("ファイル名=%s\n",filename);
  if((fp=fopen(filename,"wb")) == NULL){
    perror("Auth FILE OPEN ERROR\n");
    exit(1);
  }
  fwrite(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);
  fclose(fp);
   
  printf("AとMをファイルに保存\n\n");	
  
}

//認証用関数
int Authentication(int clntSock,char *id){
  
  //認証情報
  unsigned char pre_A2[AUTHDATA_BYTE]={'\0'};
  unsigned char A2[AUTHDATA_BYTE]={'\0'};
  unsigned char C[AUTHDATA_BYTE]={'\0'};
  unsigned char Nii[AUTHDATA_BYTE]={'\0'};
  unsigned char alpha[AUTHDATA_BYTE]={'\0'};
  unsigned char beta[AUTHDATA_BYTE]={'\0'};
  unsigned char gamma[AUTHDATA_BYTE]={'\0'};
  unsigned char authData[2][AUTHDATA_BYTE]={'\0'}; //[0]:Ai,[1]:Mi

  //ユーザごとに用意された認証情報ファイルの名前を格納する配列
  char filename[50]={'\0'};

  //認証用フラグ
  unsigned short authent_flag=0;

  //ループ用変数
  register int i;

  sprintf(filename,"./../server_data/%s/%s%s",id,id,str1);

  //Ni+1の作成
  get_random(Nii,AUTHDATA_BYTE,AUTHDATA_BYTE);
    
  ////////////////////////
  //
  printf("Ni+1->");
  for(i=0; i<sizeof(Nii); i++){
    printf("%02x",Nii[i]);
  }
  printf("\n");
  //
  ////////////////////////
    
  //Ai+1の作成
  i=0;
  while(i<AUTHDATA_BYTE){
    pre_A2[i]=id[i]^Nii[i];
    i++;
  }   
  SHA256_Init(&sha_ctx);
  SHA256_Update(&sha_ctx, pre_A2, sizeof(pre_A2));
  SHA256_Final(A2, &sha_ctx);
    
  /////////////////////////////////
  //
  printf("Ni+1^S->");
  for (i = 0; i<sizeof(pre_A2); i++) {
    printf("%02x", pre_A2[i]);
  }
  printf("\n");
    
  printf("Ai+1->");
  for (i = 0; i<sizeof(A2); ++i) {
    printf("%02x", A2[i]);
  }
  printf("\n");
  //
  /////////////////////////////////
    
  //ファイルからAとMを読み取る
  if((fp=fopen(filename,"rb")) == NULL){
    printf("Auth FILE OPEN ERROR\n");
    return -1;
  }
  fread(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);
  fclose(fp);

  ///////////////////////////
  //
  printf("read_A->");
  for(i=0; i<sizeof(authData[0]); i++){
    printf("%02x",authData[0][i]);
  }
  printf("\n");
    
  printf("read_M->");
  for(i=0; i<sizeof(authData[1]); i++){
    printf("%02x",authData[1][i]);
  }
  printf("\n");
  //
  ///////////////////////////
    
  //alphaを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    alpha[i]=authData[0][i]^A2[i]^authData[1][i];
    i++;
  }
    
  //betaを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    beta[i]=authData[0][i]+A2[i];
    i++;
  }
    
  ////////////////////////
  //
  printf("alpha->");
  for(i=0; i<sizeof(alpha); i++){
    printf("%02x",alpha[i]);
  }
  printf("\n");
    
  printf("beta->");
  for(i=0; i<sizeof(beta); i++){
    printf("%02x",beta[i]);
  }
  printf("\n");
  //
  ////////////////////////
    
  //Mi+1を作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    authData[1][i]=authData[0][i]+authData[1][i];
    i++;
  }
    
  ////////////////////////
  //
  printf("Mi+1->");
  for(i=0; i<sizeof(authData[1]); i++){
    printf("%02x",authData[1][i]);
  }
  printf("\n");
  //
  ////////////////////////
    
  //alphaを送信する
  write(clntSock, alpha, AUTHDATA_BYTE);
    
  //Cを受け取る
  read(clntSock,C,AUTHDATA_BYTE);
    
  //////////////////////////
  //
  printf("C->");
  for(i=0; i<sizeof(C); i++){
    printf("%02x",C[i]);
  }
  printf("\n");
  //
  /////////////////////////
    
  for(i=0; i<AUTHDATA_BYTE; i++){
    if(beta[i]!=C[i]){
      authent_flag=1;
    }
  }
    
  if(authent_flag==1){
    puts("beta and C are not equal");
    perror("authentication error!\n");
    return -1;
  }
  else{
    puts("beta and C are equal");

    //gammaを作成する
    i=0;
    while(i<AUTHDATA_BYTE){
      gamma[i]=authData[0][i]^authData[1][i];
      i++;
    }
      
    ////////////////////////
    //
    printf("gamma->");
    for(i=0; i<sizeof(gamma); i++){
      printf("%02x",gamma[i]);
    }
    printf("\n");
    //
    ////////////////////////
      
      //A2(Ai+1)とMi+1を保存する
      memcpy(authData,A2,sizeof(A2));
      if((fp=fopen(filename,"wb")) == NULL){
	perror("Auth FILE OPEN ERROR\n");
	return -1;
      }
      fwrite(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);
      fclose(fp);
      
      //gammaを送信する
      write(clntSock, gamma, AUTHDATA_BYTE);
  }
  return 0;
}

int VernamCipher(int clntSock,char *id){

  //バーナム暗号情報
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'}; //秘密鍵
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};

  //ユーザごとに用意された認証情報ファイルの名前を格納する配列
  char filename[50]={'\0'};

  //ループ用変数
  register int i;

  //暗号文のデータ長
  unsigned short length;
  
  puts("-------------------");

  sprintf(filename,"./../server_data/%s/%s%s",id,id,str1);
  
  //認証情報読み取り
  if((fp=fopen(filename,"rb")) == NULL){
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

  sprintf(filename,"./../server_data/%s/%s_msg.txt",id,id);
  
  if((fp=fopen(filename,"w")) == NULL){
    printf("Msg FILE OPEN ERROR\n");
    exit(1);
  }
  fwrite(vernamData,sizeof(unsigned char),strlen(vernamData),fp);
  fclose(fp);
 
  ///////////////////////////////////
  //
  printf("復号文：%s\n",vernamData);
  //
  //////////////////////////////////
  return 0;
}

void AcceptTCPConnection(int servSock){

  //ソケット通信用
  int clntSock;
  unsigned int clntLen;
  struct sockaddr_in clnt_addr;

  //通信データ用バッファサイズ
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
