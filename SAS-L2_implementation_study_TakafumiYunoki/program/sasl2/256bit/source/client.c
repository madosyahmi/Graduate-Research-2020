#include <stdio.h>       /*printf()*/
#include <stdlib.h>      /*exit()*/
#include <string.h>      /*strlen(),strcmp()*/
#include <unistd.h>      /*close()*/
#include <sys/types.h>   /*setsockopt()*/
#include <arpa/inet.h>    /*struct sockaddr_in*/
#include <sys/socket.h>  /*socket(),connect(),read(),write()*/
#include <netinet/in.h>  /*inet_addr()*/
#include <time.h>        /*clock_t,time_t*/
#include <sys/time.h>    /*struct timeval*/

#define AUTHDATA_BYTE 32

int main(int argc, char **argv){

  //ソケット通信用
  register int sock;
  struct sockaddr_in server_addr;
  unsigned short servPort;
  char *servIP;

  //通信データ用バッファサイズ
  int rcvBufferSize=100;
  int sendBufferSize=100;

  //認証用フラグ
  unsigned short authent_flag=0;
  
  FILE *fp;
  
  //ループ用変数
  register int i;
  register int n;

  //認証情報
  unsigned char B[AUTHDATA_BYTE]={'\0'};
  unsigned char C[AUTHDATA_BYTE]={'\0'};
  unsigned char D[AUTHDATA_BYTE]={'\0'};
  unsigned char alpha[AUTHDATA_BYTE]={'\0'};
  unsigned char gamma[AUTHDATA_BYTE]={'\0'};
  unsigned char authData[2][AUTHDATA_BYTE]={'\0'};

  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};
  int length;


  //実行時の入力エラー
  if((argc<2) || (argc>3)){
    fprintf(stderr,"Usage: %s <Server IP> <Server Port>\n",argv[0]);
    exit(1);
  }
  
  servIP = argv[1]; //server IP設定
  //ポート番号設定
  if(argc==3){
    servPort=atoi(argv[2]);
  }
  else{
    servPort=7777;
  }

  //サーバ接続用ソケットを作成
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client: socket\n");
    exit(1);
  }
  
  //サーバアドレス用構造体の初期設定
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = inet_addr(servIP);
  server_addr.sin_port = htons(servPort);

  //サーバと接続
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("client: connect\n");
    exit(1);
  }

  //バッファサイズを設定
  if(setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&rcvBufferSize,sizeof(rcvBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }
  if(setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendBufferSize,sizeof(sendBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }

  //初回登録フェーズ
  
  //フラグを受け取る
  read(sock, &authent_flag, sizeof(unsigned short));

  if(authent_flag==0){
    printf("このユーザは未登録のため、初回登録を行います\n\n");
	
    //A,Mを受け取る
    read(sock, &authData[0], AUTHDATA_BYTE);
    read(sock, &authData[1], AUTHDATA_BYTE);
    /////////////////////////////
    //
    printf("A->");
    for(i=0; i<sizeof(authData[0]); i++){
      printf("%02x",authData[0][i]);
    }
    printf("\n");
    printf("M->");
    for(i=0; i<sizeof(authData[1]); i++){
      printf("%02x",authData[1][i]);
    }
    printf("\n");
    //
    ////////////////////////////

    if((fp=fopen("./../client_data/Auth_file.bin","wb"))==NULL){
      perror("Auth FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);  
    fclose(fp);

    close(sock);   
    
    printf("AとMはファイルに保存\n");
    
    exit(0);
  }
  
  else if(authent_flag==1){
    printf("このクライアントは登録済みです\n\n");
    authent_flag=0;
  }
  
  else{
    perror("flag error!\n");
    close(sock);
    exit(1);
  }
 

  //認証フェーズ
    
  //alphaを受け取る
  read(sock, alpha, AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("alpha->");
  for(i=0; i<sizeof(alpha); i++){
    printf("%02x",alpha[i]);
  }
  printf("\n");
  //
  ////////////////////////////////
    
  //ファイルからAとMを読み取る
  if((fp=fopen("./../client_data/Auth_file.bin","rb"))==NULL){
    perror("Auth FILE OPEN ERROR\n");
    close(sock);
    exit(1);
  }
  fread(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);
  fclose(fp);
    
  ///////////////////////////////
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
  ///////////////////////////////
    
  //Bを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    B[i]=alpha[i]^authData[0][i]^authData[1][i];
    i++;
  }
    
  //Cを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    C[i]=authData[0][i]+B[i];
    i++;
  }
    
  ///////////////////////////
  //
  printf("B->");
  for(i=0; i<sizeof(B); i++){
    printf("%02x",B[i]);
  }
  printf("\n");
    
  printf("C->");
  for(i=0; i<sizeof(C); i++){
    printf("%02x",C[i]);
  }
  printf("\n");
  //
  ///////////////////////////
    
  //Cを送信する
  write(sock, C, AUTHDATA_BYTE);
    
  //gammaを受け取る
  read(sock, gamma, AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("gamma->");
  for(i=0; i<sizeof(gamma); i++){
    printf("%02x",gamma[i]);
  }
  printf("\n");
  //
  ///////////////////////////////
    
  //Mi+1を作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    authData[1][i]=authData[0][i]+authData[1][i];
    i++;
  }
    
  //////////////////////////
  //
  printf("Mi+1->");
  for(i=0; i<sizeof(authData[1]); i++){
    printf("%02x",authData[1][i]);
  }
  printf("\n");
  //
  //////////////////////////
    
  //Dを作成する
  i=0;
  while(i<AUTHDATA_BYTE){
    D[i]=authData[0][i]^authData[1][i];
    i++;
  }
    
  ///////////////////////////
  //
  printf("D->");
  for(i=0; i<sizeof(D); i++){
    printf("%02x",D[i]);
  }
  printf("\n");
  //
  //////////////////////////
    
  for(i=0; i<AUTHDATA_BYTE; i++){
    if(gamma[i]!=D[i]){
      authent_flag=1;
    }
  }
    
  if(authent_flag==1){
    perror("gamma and D are not equal\n");
    perror("authentication error!\n");
    close(sock);
    exit(1);
  }
  else{
    //printf("gammma and D equal\n");
    memcpy(authData,B,sizeof(B));
      
    //次の認証情報をファイルに保存
    if((fp=fopen("./../client_data/Auth_file.bin","wb"))==NULL){
      perror("Auth FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(authData,sizeof(authData[0][0])*AUTHDATA_BYTE,2,fp);
    fclose(fp);
  }
  
  //暗号通信フェーズ
  puts("-----------------------");
  
  ///////////////////////////////////
  //
  printf("vernam_key:");
  for(i=0; i<AUTHDATA_BYTE; i++){
    printf("%02x",B[i]);
  }
  printf("\n");
  //
  //////////////////////////////////
  
  
  //バーナム暗号用メッセージを読み取り表示
  if((fp=fopen("./../client_data/msg.txt","r"))==NULL){
    perror("msg_file open error!\n");
    close(sock);
    exit(1);
  }
  fgets(vernamData,AUTHDATA_BYTE+1,fp);
  /////////////////////////////////
  //
  printf("平文(上限32文字)：%s\n",vernamData);
  //
  //////////////////////////////////
  
  //暗号化
  i=0;
  length=strlen(vernamData);
  while(i<length){
    vernamData[i]^=B[i];
    i++;
  }
  vernamData[i]='\0';
  
  ////////////////////////////////////
  //
  printf("暗号文(16進数):");
  for(i=0; i<length; i++){
    printf("%02x",vernamData[i]);
  }
  printf("\n");
  //
  ///////////////////////////////////
  
  //送信するmsgの長さの情報を送信する
  write(sock, &length, sizeof(unsigned short));
  
  //msgを送信する
  write(sock, vernamData,length);
  close(sock);
  
  exit(0);
}
