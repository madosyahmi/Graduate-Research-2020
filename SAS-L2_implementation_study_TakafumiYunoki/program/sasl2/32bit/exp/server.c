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
#include <sys/stat.h>

#define MAXPENDING 5
#define AUTHDATA_BYTE 4            //認証情報のビット数
#define DATAMAX 4294967295         //32bitの上限値
#define DEV_RANDOM "/dev/urandom"

char str1[] = "Auth_file.bin";

unsigned char hash_buf[20]={'\0'};
unsigned char rand_buf[AUTHDATA_BYTE];
FILE *fp;

//関数プロトタイプ宣言
unsigned long crc32(const unsigned char *s,int len);
unsigned long get_random(unsigned char *const buf, const int bufLen, const int len);
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr);
void InitRegistration(int clntSock,unsigned char *clntIpAddr);
int Authentication(int clntSock,char *username);
int VernamCipher(int clntSock,char *username);
void AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);

static const unsigned int crc32tab[256] = { 
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

unsigned long crc32(const unsigned char *s,int len){
  unsigned int crc_init = 0;
  unsigned int crc =0;
  
  crc = crc_init ^ 0xFFFFFFFF;
  for(;len--;s++){
    crc = ((crc >> 8)&0x00FFFFFF) ^ crc32tab[(crc ^ (*s)) & 0xFF];
  }
  return crc ^ 0xFFFFFFFF;
}

unsigned long get_random(unsigned char *const buf, const int bufLen, const int len){
  //初期エラー
  if(len > bufLen){
    perror("buffer size is small\n");
    return -1;
  }
  
  int fd = open(DEV_RANDOM, O_RDONLY);
  unsigned long result=0b0;
  register int i;
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
  
  for(i=0; i<AUTHDATA_BYTE; i++){
    result=(result<<8)+buf[i];
  }
  return result;
}

//クライアントが登録済みのユーザかをチェックする関数
void UserDataSearch(int clntSock,struct sockaddr_in *clnt_addr){
  unsigned char dataLine[50]={'\0'}; //userdataの1行分を保持
  unsigned char search[20]={'\0'};   //探索するIPアドレスを保持
  char id[AUTHDATA_BYTE+1]={'\0'};   //識別子を保持
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
  unsigned long id_num,pre_A,Ni;
  unsigned long authData[2];
  unsigned short overflow_flag=1;

  char filename1[50]={'\0'};
  char foldername[]={'\0'};

  register int i;
  unsigned short length;
  
  puts("識別子を設定してください");
  //標準入力(fgets)を受け取る
  if(fgets(id,AUTHDATA_BYTE+2,stdin)==NULL || id[0]=='\0' || id[0]=='\n'){
    perror("識別子:1-4char\n");
    exit(1);
  }
  for(i=1; i<AUTHDATA_BYTE+2; i++){
    if(id[i]=='\n'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("識別子:1-4char\n");
    exit(1); 
  }
  overflow_flag=1;
  
  //改行を削除
  length=strlen(id);
  if(id[length-1]=='\n'){
    id[--length]='\0';
  }

  //識別子を数値に変換
  id_num=0;
  for(i=0; i<AUTHDATA_BYTE; i++){
    id_num=(id_num<<8)+id[i];
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
  Ni=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("Ni->%lu\n",Ni);
  //
  ////////////////////////////////

  //Mi作成
  authData[1]=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
  ////////////////////////////////
  //
  printf("Mi->%lu\n",authData[1]);
  //
  ///////////////////////////////

  //Aを作成
  pre_A=id_num^Ni;
  sprintf(hash_buf,"%lu",pre_A);
  authData[0]=crc32(hash_buf,strlen(hash_buf));

  //////////////////////////////
  //デバッグ用
  printf("Ni^S->%lu\n",pre_A);
  printf("A->%lu\n",authData[0]);
  //
  //////////////////////////////////

  //A,Mを送信
  write(clntSock,&authData[0],sizeof(unsigned long));
  write(clntSock,&authData[1],sizeof(unsigned long));

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1); //User_Auth_file
  
  //A,Mを保存
  printf("ファイル名=%s\n",filename1);
  if((fp=fopen(filename1,"wb")) == NULL){
    perror("Auth FILE OPEN ERROR\n");
    exit(1);
  }
  fwrite(authData,sizeof(unsigned long),2,fp);
  fclose(fp);
   
  printf("AとMをファイルに保存\n\n");	
  
}

//認証用関数
int Authentication(int clntSock,char *id){
  unsigned long id_num,pre_A2,A2,C,Nii,alpha,beta,gamma;
  unsigned long authData[2];

  char filename1[50]={'\0'};

  register int i;

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1);

  //識別子を数値に変換
  id_num=0;
  for(i=0; i<AUTHDATA_BYTE; i++){
    id_num=(id_num<<8)+id[i];
  }
  
  while(1){
    //Ni+1の作成
    Nii=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
    
    ////////////////////////
    //
    //printf("Ni+1->%lu\n",Nii);
    //
    ////////////////////////
    
    //Bの作成
    pre_A2=id_num^Nii;
    sprintf(hash_buf,"%lu",pre_A2);
    A2=crc32(hash_buf,strlen(hash_buf));
    
    /////////////////////////////////
    //
    //printf("Ni+1^S->%lu\n",pre_A2);
    //printf("Ai+1->%lu\n",A2);
    //
    /////////////////////////////////
    
    //ファイルからAとMを読み取る
    if((fp=fopen(filename1,"rb")) == NULL){
      printf("Auth FILE OPEN ERROR\n");
      return -1;
    }
    fread(authData,sizeof(unsigned long),2,fp);
    fclose(fp);
    
    ///////////////////////////
    //
    //printf("read_A->%lu\n",authData[0]);
    //printf("read_M->%lu\n",authData[1]);
    //
    ///////////////////////////
    
    //alphaを作成する
    alpha=authData[0]^A2^authData[1];
    
    //betaを作成する
    if(authData[0]+A2>DATAMAX){
      beta=authData[0]+A2-DATAMAX;
    }
    else{
      beta=authData[0]+A2;
    }
    
    ////////////////////////
    //
    //printf("alpha->%lu\n",alpha);
    //printf("beta->%lu\n",beta);
    //
    ////////////////////////
    
    //Mi+1を作成する
    if(authData[0]+authData[1]>DATAMAX){
      authData[1]=authData[0]+authData[1]-DATAMAX;
    }
    else{
      authData[1]=authData[0]+authData[1];
    }
    
    ////////////////////////
    //
    //printf("Mi+1->%lu\n",authData[1]);
    //
    ////////////////////////
    
    //alphaを送信する
    write(clntSock,&alpha,sizeof(unsigned long));
    
    //Cを受け取る
    read(clntSock,&C,sizeof(unsigned long));
    
    //////////////////////////
    //
    //printf("C->%lu\n",C);
    //
    /////////////////////////
    
    
    if(beta-C != 0){
      puts("beta and C are not equal");
      perror("authentication error!\n");
      return -1;
    }
    else{
      //puts("beta and C are equal");
      
      //gammaを作成する
      gamma=authData[0]^authData[1];
      
      ////////////////////////
      //
      //printf("gamma->%lu\n",gamma);
      //
      ////////////////////////

      authData[0]=A2;

      //B(Ai+1)とMi+1を保存する
      if((fp=fopen(filename1,"wb")) == NULL){
	perror("Auth FILE OPEN ERROR\n");
	return -1;
      }
      fwrite(authData,sizeof(unsigned long),2,fp);
      fclose(fp);
      
      //gammaを送信する
      write(clntSock,&gamma,sizeof(unsigned long));

    }
  }
  return 0;
}

int VernamCipher(int clntSock,char *id){
  char filename1[50]={'\0'};
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'};
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};
  unsigned long authData;
  register int i;
  unsigned short length;
  
  puts("-------------------");

  sprintf(filename1,"./../server_data/%s/%s%s",id,id,str1);
  
  //認証情報読み取り
  if((fp=fopen(filename1,"rb")) == NULL){
    printf("Auth FILE OPEN ERROR\n");
    exit(1);
  }
  fread(&authData,sizeof(unsigned long),1,fp);
  fclose(fp);

  for(i=0; i<AUTHDATA_BYTE; i++){
    vernamKey[i]=(authData & ((unsigned long)0xFF<<24-8*i))>>24-8*i;
  }

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
  //
  //////////////////////////////////
  return 0;
}

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
