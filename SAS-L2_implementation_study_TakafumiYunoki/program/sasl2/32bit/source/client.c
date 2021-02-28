#include <stdio.h>       /*printf()*/
#include <stdlib.h>      /*exit()*/
#include <string.h>      /*strlen(),strcmp()*/
#include <unistd.h>      /*close()*/
#include <sys/types.h>   /*setsockopt()*/
#include <arpa/inet.h>    /*struct sockaddr_in*/
#include <sys/socket.h>  /*socket(),connect(),read(),write()*/
#include <netinet/in.h>  /*inet_addr()*/

#define AUTHDATA_BYTE 4            //認証情報のビット数
#define DATAMAX 4294967295         //32bitの上限値

int main(int argc, char **argv){
  
  //ソケット通信用
  register int sock;
  struct sockaddr_in server_addr;
  unsigned short servPort;
  char *servIP;
  
  //通信データ用バッファサイズ
  int rcvBufferSize=100;
  int sendBufferSize=100;

  //データ長
  unsigned short length;
  
  //各種フラグ
  unsigned short authent_flag=0;
  unsigned short overflow_flag=1;
  
  FILE *fp;

  //ループ用変数
  register int i;
  register int n;

  //認証情報
  unsigned long B,C,D,alpha,gamma;
  unsigned long authData[2];

  //暗号通信用
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'};
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};
  
  //実行時の入力エラー
  if((argc<2) || (argc>3)){
    fprintf(stderr,"Usage: %s <Server IP> <Server Port>\n",argv[0]);
    exit(1);
  }
  
  servIP = argv[1]; //server IP設定
  //ポート番号設定
  if(argc==3){
    servPort=atoi(argv[2]);    //ポート番号設定
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
  
  //フラグを受け取る
  read(sock, &authent_flag, sizeof(unsigned short));

  if(authent_flag==0){
    
    //初回登録フェーズ
    printf("このユーザは未登録のため、初回登録を行います\n\n");
	
    //A,Mを受け取る
    read(sock, &authData[0], sizeof(unsigned long));
    read(sock, &authData[1], sizeof(unsigned long));

    /////////////////////////////
    //
    printf("A->%lu\n",authData[0]);
    printf("M->%lu\n",authData[1]);
    //
    ////////////////////////////

    if((fp=fopen("./../client_data/Auth_file.bin","wb"))==NULL){
      perror("Auth FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(authData,sizeof(unsigned long),2,fp);
    fclose(fp);
    
    close(sock);

    printf("AとMはファイルに保存\n");
     
    exit(0);
  }
  
  else if(authent_flag==1){
    printf("このクライアントは登録済みです\n\n");
  }
  
  else{
    perror("flag error!\n");
    close(sock);
    exit(1);
  }
 

  //認証フェーズ
    
  //alphaを受け取る
  read(sock, &alpha, sizeof(unsigned long));
  ////////////////////////////////
  //
  printf("alpha->%lu\n",alpha);
  //
  ////////////////////////////////
    
  //ファイルからAとMを読み取る
  if((fp=fopen("./../client_data/Auth_file.bin","rb"))==NULL){
    perror("Auth FILE OPEN ERROR\n");
    close(sock);
    exit(1);
  }
  fread(authData,sizeof(unsigned long),2,fp);
  fclose(fp);
    
  ///////////////////////////////
  //
  printf("read_A->%lu\n",authData[0]);
  printf("read_M->%lu\n",authData[1]);
  //
  ///////////////////////////////
    
  //Bを作成する
  B=alpha^authData[0]^authData[1];
    
  //Cを作成する
  if(authData[0]+B>DATAMAX){
    C=authData[0]+B-DATAMAX;
  }
  else{
    C=authData[0]+B;
  }
    
  ///////////////////////////
  //
  printf("read_B->%lu\n",B);
  printf("read_C->%lu\n",C);
  //
  ///////////////////////////
    
  //Cを送信する
  write(sock, &C, sizeof(unsigned long));
    
  //gammaを受け取る
  read(sock, &gamma, sizeof(unsigned long));
  ////////////////////////////////
  //
  printf("gamma->%lu\n",gamma);
  //
  ///////////////////////////////
    
  //Mi+1を作成する
  if(authData[0]+authData[1]>DATAMAX){
    authData[1]=authData[0]+authData[1]-DATAMAX;
  }
  else{
    authData[1]=authData[0]+authData[1];
  }

  //////////////////////////
  //
  printf("Mi+1->%lu\n",authData[1]);
  //
  //////////////////////////

  //Dを作成する
  D=authData[0]^authData[1];

  ///////////////////////////
  //
  printf("D->%lu\n",D);
  //
  //////////////////////////
    
  if(gamma-D != 0){
    perror("gamma and D are not equal\n");
    perror("authentication error!\n");
    close(sock);
    exit(1);
  }
  else{
    printf("gammma and D equal\n");
    authData[0]=B;
      
    //次の認証情報をファイルに保存
    if((fp=fopen("./../client_data/Auth_file.bin","wb"))==NULL){
      printf("Auth file open error!\n");
      close(sock);
      exit(1);
    }
    fwrite(authData,sizeof(unsigned long),2,fp);
    fclose(fp);
  }
  
  //暗号通信フェーズ
  puts("-----------------------");

  for(i=0; i<AUTHDATA_BYTE; i++){
    vernamKey[i]=(B & ((unsigned long)0xFF<<24-8*i))>>24-8*i;
  }
  
  ///////////////////////////////////
  //
  printf("vernam_key:");
  for(i=0; i<AUTHDATA_BYTE; i++){
    printf("%02x",vernamKey[i]);
  }
  printf("\n");
  //
  //////////////////////////////////
  
  
  //バーナム暗号用メッセージを読み取り表示
  if((fp=fopen("./../client_data/msg.txt","r"))==NULL){
    perror("Msg_file open error!\n");
    close(sock);
    exit(1);
  }
  fgets(vernamData,AUTHDATA_BYTE+1,fp);

  /////////////////////////////////
  //
  printf("平文(上限4文字)：%s\n",vernamData);
  //
  //////////////////////////////////
  
  //暗号化
  i=0;
  length=strlen(vernamData);
  while(i<length){
    vernamData[i]^=vernamKey[i];
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
  
  return 0;
}
