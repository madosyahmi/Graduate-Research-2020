#include <stdio.h>        /*printf()*/
#include <stdlib.h>       /*exit()*/
#include <string.h>       /*strlen(),strcmp()*/
#include <unistd.h>       /*close()*/
#include <sys/socket.h>   /*socket(),connect()*/
#include <sys/types.h>    /*setsockopt()*/
#include <arpa/inet.h>    /*struct sockaddr_in*/
#include <netinet/in.h>   /*inet_addr()*/
#include <openssl/sha.h>  /*SHA256*/
#include <fcntl.h>        /*open()*/
#include <time.h>         /*clock_t,time_t*/
#include <sys/time.h>     /*struct timeval*/

#define DEV_RANDOM "/dev/urandom"
#define AUTHDATA_BYTE 32

void get_random(char *const buf, const int bufLen, const int len){
  //初期エラー
  if(len > bufLen){
    perror("buffer size is small\n");
    exit(1);
  }
  
  int fd = open(DEV_RANDOM, O_RDONLY);
  if(fd == -1){
    printf("can not open %s\n", DEV_RANDOM);
    exit(1);
  }
  
  int r = read(fd,buf,len);
  if(r < 0){
    perror("can not read\n");
    exit(1);
  }
  if(r != len){
    perror("can not read\n");
    exit(1);
  }
  
  close(fd);
}

double getETime(){
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

int main(int argc, char **argv){

  //ソケット通信用
  register int sock;
  struct sockaddr_in server_addr;
  unsigned short servPort;
  char *servIP;

  //通信データ用バッファサイズ
  int rcvBufferSize=100*1024;
  int sendBufferSize=100*1024;

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
  unsigned char id[34]={'\0'};
  unsigned char pass[AUTHDATA_BYTE+2]={'\0'};
  unsigned char A[AUTHDATA_BYTE]={'\0'};
  unsigned char pre_A[AUTHDATA_BYTE]={'\0'};  
  unsigned char A2[AUTHDATA_BYTE]={'\0'};
  unsigned char pre_A2[AUTHDATA_BYTE]={'\0'};
  unsigned char B[AUTHDATA_BYTE]={'\0'};
  unsigned char F[AUTHDATA_BYTE]={'\0'};
  unsigned char alpha[AUTHDATA_BYTE]={'\0'};
  unsigned char beta[AUTHDATA_BYTE]={'\0'};
  unsigned char gamma[AUTHDATA_BYTE]={'\0'};
  unsigned char Ni[AUTHDATA_BYTE]={'\0'};
  unsigned char Nii[AUTHDATA_BYTE]={'\0'};

  //暗号通信用
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};

  SHA256_CTX sha_ctx;

  /*時間計測*/
  clock_t start,end;
  double st,en;
  int calc_times;
  register int z;

  if((argc<3) || (argc>4)){
    fprintf(stderr,"Usage: %s <Server IP> <Server Port> <calc times>\n",argv[0]);
    exit(1);
  }

  printf("%d\n",getpid());
  
  servIP = argv[1]; //server IP設定
  //ポート番号設定
  if(argc==4){
    servPort=atoi(argv[2]);    //ポート番号設定
    calc_times=atoi(argv[3]);  //測定回数設定
  }
  else{
    servPort=7777;
    calc_times=atoi(argv[2]);
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

    //Niを作成
    get_random(Ni,AUTHDATA_BYTE,AUTHDATA_BYTE);
    /////////////////////////////
    //
    printf("Ni->");
    for(i=0; i<sizeof(Ni); i++){
      printf("%02x",Ni[i]);
    }
    printf("\n");
    //
    /////////////////////////////
    
    //Niを保存
    if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(Ni,sizeof(Ni[0])*AUTHDATA_BYTE,1,fp);
    fclose(fp);
    
    printf("ID:");
    //最大32文字の入力を受け入れる
    if (fgets(id, 34, stdin)==NULL || id[0]=='\0' || id[0] == '\n'){
      perror("id:1-32char\n");
      close(sock);
      exit(1);
    }
    for(i=1; i<34; i++){
      if(id[i]=='\n'){
	overflow_flag=0;
      }
    }
    if(overflow_flag==1){
      perror("id:1-32char\n");
      close(sock);
      exit(1); 
    }
    overflow_flag=1;
    
    //改行を削除
    length=strlen(id);
    if(id[length-1]=='\n'){
      id[--length]='\0';
    }

    //送信するidの長さの情報を送信する
    write(sock, &length, sizeof(unsigned short));

    //IDを送信する
    write(sock, id, length);
    
    printf("PASS:");
    //最大32文字の入力を受け入れる
    if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
      perror("PASS:1-32char\n");
      close(sock);
      exit(1);
    }
    for(i=1; i<AUTHDATA_BYTE+2; i++){
      if(pass[i]=='\n'){
	overflow_flag=0;
      }
    }
    if(overflow_flag==1){
      perror("pass:1-32char\n");
      close(sock);
      exit(1); 
    }
    overflow_flag=1;
    
    //改行を削除
    length=strlen(pass);
    if(pass[length-1]=='\n'){
      pass[--length]='\0';
    }

    //Aを作成
    i=0;
    while(i<AUTHDATA_BYTE){
      pre_A[i]=pass[i]^Ni[i];
      i++;
    }

    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, pre_A, sizeof(pre_A));
    SHA256_Final(A, &sha_ctx);

    //////////////////////////////////////
    //
    printf("Ni^S->");
    for(i=0; i<sizeof(pre_A); i++){
      printf("%02x",pre_A[i]);
    }
    printf("\n");
    
    printf("A->");
    for(i=0; i<sizeof(A); i++){
      printf("%02x",A[i]);
    }
    printf("\n");
    //
    //////////////////////////////////////

    //Aを送信する
    write(sock, A, AUTHDATA_BYTE);

    close(sock);
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

  printf("PASS:");
  if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
    perror("PASS:1-32char\n");
    close(sock);
    exit(1);
  }
  for(i=1; i<AUTHDATA_BYTE+2; i++){
    if(pass[i]=='\n'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("pass:1-32char\n");
    close(sock);
    exit(1); 
  }
  overflow_flag=1;
  
  //改行を削除
  length=strlen(pass);
  if(pass[length-1]=='\n'){
    pass[--length]='\0';
  }
  
  //認証フェーズ

  /*計測開始*/
  puts("計測開始:");
  st = getETime();
  start = clock();
  for(z=calc_times; z>0; z--){
    
    if((fp=fopen("./../client_data/Ni_file.bin","rb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fread(Ni,sizeof(Ni[0])*AUTHDATA_BYTE,1,fp);
    fclose(fp);
    
    //Aの作成
    i=0;
    while(i<AUTHDATA_BYTE){
      pre_A[i]=pass[i]^Ni[i];
      i++;
    }
    
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, pre_A, sizeof(pre_A));
    SHA256_Final(A, &sha_ctx); 
    
    /////////////////////////////////
    //
    /*
    printf("Ni^S->");
    for (i=0; i<sizeof(pre_A); i++) {
      printf("%02x", pre_A[i]);
    }
    printf("\n");
    
    printf("A->");
    for (i=0; i<sizeof(A); ++i) {
      printf("%02x", A[i]);
    }
    printf("\n");
    */
    //
    /////////////////////////////////
    
    //Ni+1を作成
    get_random(Nii,AUTHDATA_BYTE,AUTHDATA_BYTE);
    ///////////////////////////////
    //
    /*
    printf("Ni+1->");
    for(i=0; i<sizeof(Nii); i++){
      printf("%02x",Nii[i]);
    }
    printf("\n");
    */
    //
    //////////////////////////////
    
    //Ai+1を作成
    i=0;
    while(i<AUTHDATA_BYTE){
      pre_A2[i]=pass[i]^Nii[i];
      i++;
    }
    
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, pre_A2, sizeof(pre_A2));
    SHA256_Final(A2, &sha_ctx);
    
    /////////////////////////////////
    //
    /*
    printf("Ni+1^S->");
    for (i = 0; i<sizeof(pre_A2); i++) {
      printf("%02x", pre_A2[i]);
    }
    printf("\n");
    
    printf("A2->");
    for (i = 0; i<sizeof(A2); ++i) {
      printf("%02x", A2[i]);
    }
    printf("\n");
    */
    //
    /////////////////////////////////
    
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, A2, sizeof(A2));
    SHA256_Final(B, &sha_ctx);
    
    ////////////////////////////////
    //
    /*
    printf("B->");
    for (i = 0; i<sizeof(B); ++i) {
      printf("%02x", B[i]);
    }
    printf("\n");
    */
    //
    ////////////////////////////////
    
    //alphaを作成
    i=0;
    while(i<AUTHDATA_BYTE){
      alpha[i]=A2[i]^(B[i]+A[i]);
      i++;
    }
    ///////////////////////////////
    //
    /*
    printf("alpha->");
    for(i=0; i<sizeof(alpha); i++){
      printf("%02x",alpha[i]);
    }
    printf("\n");
    */
    //
    //////////////////////////////
    
    //betaを作成
    i=0;
    while(i<AUTHDATA_BYTE){
      beta[i]=B[i]^A[i];
      i++;
    }
    /////////////////////////////
    //
    /*
    printf("beta->");
    for(i=0; i<sizeof(beta); i++){
      printf("%02x",beta[i]);
    }
    printf("\n");
    */
    //
    ////////////////////////////
    
    //alphaを送信する
    write(sock, alpha, AUTHDATA_BYTE);
    
    //betaを送信する
    write(sock, beta, AUTHDATA_BYTE);
    
    //gammaを受け取る
    read(sock, gamma, AUTHDATA_BYTE);
    ////////////////////////////
    //
    /*
    printf("gamma->");
    for(i=0; i<sizeof(gamma); i++){
      printf("%02x",gamma[i]);
    }
    printf("\n");
    */
    //
    ////////////////////////////
    
    //F=H(B)を作成する
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, B, sizeof(B));
    SHA256_Final(F, &sha_ctx);
    
    ////////////////////////////////
    //
    /*
    printf("F->");
    for (i = 0; i<sizeof(F); ++i) {
      printf("%02x", F[i]);
    }
    printf("\n");
    */
    //
    ////////////////////////////////
    
    for(i=0; i<AUTHDATA_BYTE; i++){
      if(gamma[i]!=F[i]){
	authent_flag=1;
	printf("%d    ---\n",i);
      }
    }
    
    if(authent_flag==1){
      perror("gamma and F are not equal\n");
      perror("authentication error!\n");
      close(sock);
      exit(1);
    }
    else{
      //printf("gamma and F equal\n");

      //次の認証情報をファイルに保存
      if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
	perror("Ni FILE OPEN ERROR\n");
	close(sock);
	exit(1);
      }
      fwrite(Nii,sizeof(Nii[0])*AUTHDATA_BYTE,1,fp);
      fclose(fp);
    }
  }

  /*計測終了*/
  end = clock();
  en = getETime();
      
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
    for(i=1; i<AUTHDATA_BYTE+1; i++){
    if(vernamData[i]=='\0'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("vernamData:1-32char\n");
    close(sock);
    exit(1);
  }
  /////////////////////////////////
  //
  printf("平文(上限32文字)：%s\n",vernamData);
  //
  //////////////////////////////////
  
  //暗号化
  i=0;
  length=strlen(vernamData);
  while(i<length){
    vernamData[i]^=A2[i];
    i++;
  }
  vernamData[i]='\0';
  
  ////////////////////////////////////
  //
  printf("暗号文(16進数):");
  for(i=0; i<length; i++){
    printf("%02x",vernamData[i]);
  }
  vernamData[i]='\0';
  printf("\n");
  //
  ///////////////////////////////////
  
  //送信するmsgの長さの情報を送信する
  write(sock, &length, sizeof(unsigned short));
  
  //msgを送信する
  write(sock, vernamData,length);
  close(sock);

  //計測結果出力
  printf("CPU時間 :%.6f s\n",(double)(end-start)/(CLOCKS_PER_SEC));
  printf("経過時間:%.6f s\n",en-st);

  //測定用
  puts("-----------------------");
  printf("process number -> %d\nPlease input Ctrl+C...\n",getpid());
  while(1){
  }
  
  exit(0);
}
