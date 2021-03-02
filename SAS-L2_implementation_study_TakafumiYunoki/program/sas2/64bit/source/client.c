#include <stdio.h>        /*printf()*/
#include <stdlib.h>       /*exit()*/
#include <string.h>       /*strlen(),strcmp()*/
#include <unistd.h>       /*close()*/
#include <sys/socket.h>   /*socket(),connect()*/
#include <sys/types.h>    /*setsockopt()*/
#include <arpa/inet.h>    /*struct sockaddr_in*/
#include <netinet/in.h>   /*inet_addr()*/
#include <fcntl.h>        /*open()*/
#include <time.h>         /*clock_t,time_t*/
#include <sys/time.h>     /*struct timeval*/

#define DEV_RANDOM "/dev/urandom"  //乱数生成用のデバイスファイル名
#define AUTHDATA_BYTE 8            //認証情報のビット数

static const unsigned long crc64_tab[256] = {
    0x0000000000000000, 0x7ad870c830358979,
    0xf5b0e190606b12f2, 0x8f689158505e9b8b,
    0xc038e5739841b68f, 0xbae095bba8743ff6,
    0x358804e3f82aa47d, 0x4f50742bc81f2d04,
    0xab28ecb46814fe75, 0xd1f09c7c5821770c,
    0x5e980d24087fec87, 0x24407dec384a65fe,
    0x6b1009c7f05548fa, 0x11c8790fc060c183,
    0x9ea0e857903e5a08, 0xe478989fa00bd371,
    0x7d08ff3b88be6f81, 0x07d08ff3b88be6f8,
    0x88b81eabe8d57d73, 0xf2606e63d8e0f40a,
    0xbd301a4810ffd90e, 0xc7e86a8020ca5077,
    0x4880fbd87094cbfc, 0x32588b1040a14285,
    0xd620138fe0aa91f4, 0xacf86347d09f188d,
    0x2390f21f80c18306, 0x594882d7b0f40a7f,
    0x1618f6fc78eb277b, 0x6cc0863448deae02,
    0xe3a8176c18803589, 0x997067a428b5bcf0,
    0xfa11fe77117cdf02, 0x80c98ebf2149567b,
    0x0fa11fe77117cdf0, 0x75796f2f41224489,
    0x3a291b04893d698d, 0x40f16bccb908e0f4,
    0xcf99fa94e9567b7f, 0xb5418a5cd963f206,
    0x513912c379682177, 0x2be1620b495da80e,
    0xa489f35319033385, 0xde51839b2936bafc,
    0x9101f7b0e12997f8, 0xebd98778d11c1e81,
    0x64b116208142850a, 0x1e6966e8b1770c73,
    0x8719014c99c2b083, 0xfdc17184a9f739fa,
    0x72a9e0dcf9a9a271, 0x08719014c99c2b08,
    0x4721e43f0183060c, 0x3df994f731b68f75,
    0xb29105af61e814fe, 0xc849756751dd9d87,
    0x2c31edf8f1d64ef6, 0x56e99d30c1e3c78f,
    0xd9810c6891bd5c04, 0xa3597ca0a188d57d,
    0xec09088b6997f879, 0x96d1784359a27100,
    0x19b9e91b09fcea8b, 0x636199d339c963f2,
    0xdf7adabd7a6e2d6f, 0xa5a2aa754a5ba416,
    0x2aca3b2d1a053f9d, 0x50124be52a30b6e4,
    0x1f423fcee22f9be0, 0x659a4f06d21a1299,
    0xeaf2de5e82448912, 0x902aae96b271006b,
    0x74523609127ad31a, 0x0e8a46c1224f5a63,
    0x81e2d7997211c1e8, 0xfb3aa75142244891,
    0xb46ad37a8a3b6595, 0xceb2a3b2ba0eecec,
    0x41da32eaea507767, 0x3b024222da65fe1e,
    0xa2722586f2d042ee, 0xd8aa554ec2e5cb97,
    0x57c2c41692bb501c, 0x2d1ab4dea28ed965,
    0x624ac0f56a91f461, 0x1892b03d5aa47d18,
    0x97fa21650afae693, 0xed2251ad3acf6fea,
    0x095ac9329ac4bc9b, 0x7382b9faaaf135e2,
    0xfcea28a2faafae69, 0x8632586aca9a2710,
    0xc9622c4102850a14, 0xb3ba5c8932b0836d,
    0x3cd2cdd162ee18e6, 0x460abd1952db919f,
    0x256b24ca6b12f26d, 0x5fb354025b277b14,
    0xd0dbc55a0b79e09f, 0xaa03b5923b4c69e6,
    0xe553c1b9f35344e2, 0x9f8bb171c366cd9b,
    0x10e3202993385610, 0x6a3b50e1a30ddf69,
    0x8e43c87e03060c18, 0xf49bb8b633338561,
    0x7bf329ee636d1eea, 0x012b592653589793,
    0x4e7b2d0d9b47ba97, 0x34a35dc5ab7233ee,
    0xbbcbcc9dfb2ca865, 0xc113bc55cb19211c,
    0x5863dbf1e3ac9dec, 0x22bbab39d3991495,
    0xadd33a6183c78f1e, 0xd70b4aa9b3f20667,
    0x985b3e827bed2b63, 0xe2834e4a4bd8a21a,
    0x6debdf121b863991, 0x1733afda2bb3b0e8,
    0xf34b37458bb86399, 0x8993478dbb8deae0,
    0x06fbd6d5ebd3716b, 0x7c23a61ddbe6f812,
    0x3373d23613f9d516, 0x49aba2fe23cc5c6f,
    0xc6c333a67392c7e4, 0xbc1b436e43a74e9d,
    0x95ac9329ac4bc9b5, 0xef74e3e19c7e40cc,
    0x601c72b9cc20db47, 0x1ac40271fc15523e,
    0x5594765a340a7f3a, 0x2f4c0692043ff643,
    0xa02497ca54616dc8, 0xdafce7026454e4b1,
    0x3e847f9dc45f37c0, 0x445c0f55f46abeb9,
    0xcb349e0da4342532, 0xb1eceec59401ac4b,
    0xfebc9aee5c1e814f, 0x8464ea266c2b0836,
    0x0b0c7b7e3c7593bd, 0x71d40bb60c401ac4,
    0xe8a46c1224f5a634, 0x927c1cda14c02f4d,
    0x1d148d82449eb4c6, 0x67ccfd4a74ab3dbf,
    0x289c8961bcb410bb, 0x5244f9a98c8199c2,
    0xdd2c68f1dcdf0249, 0xa7f41839ecea8b30,
    0x438c80a64ce15841, 0x3954f06e7cd4d138,
    0xb63c61362c8a4ab3, 0xcce411fe1cbfc3ca,
    0x83b465d5d4a0eece, 0xf96c151de49567b7,
    0x76048445b4cbfc3c, 0x0cdcf48d84fe7545,
    0x6fbd6d5ebd3716b7, 0x15651d968d029fce,
    0x9a0d8ccedd5c0445, 0xe0d5fc06ed698d3c,
    0xaf85882d2576a038, 0xd55df8e515432941,
    0x5a3569bd451db2ca, 0x20ed197575283bb3,
    0xc49581ead523e8c2, 0xbe4df122e51661bb,
    0x3125607ab548fa30, 0x4bfd10b2857d7349,
    0x04ad64994d625e4d, 0x7e7514517d57d734,
    0xf11d85092d094cbf, 0x8bc5f5c11d3cc5c6,
    0x12b5926535897936, 0x686de2ad05bcf04f,
    0xe70573f555e26bc4, 0x9ddd033d65d7e2bd,
    0xd28d7716adc8cfb9, 0xa85507de9dfd46c0,
    0x273d9686cda3dd4b, 0x5de5e64efd965432,
    0xb99d7ed15d9d8743, 0xc3450e196da80e3a,
    0x4c2d9f413df695b1, 0x36f5ef890dc31cc8,
    0x79a59ba2c5dc31cc, 0x037deb6af5e9b8b5,
    0x8c157a32a5b7233e, 0xf6cd0afa9582aa47,
    0x4ad64994d625e4da, 0x300e395ce6106da3,
    0xbf66a804b64ef628, 0xc5bed8cc867b7f51,
    0x8aeeace74e645255, 0xf036dc2f7e51db2c,
    0x7f5e4d772e0f40a7, 0x05863dbf1e3ac9de,
    0xe1fea520be311aaf, 0x9b26d5e88e0493d6,
    0x144e44b0de5a085d, 0x6e963478ee6f8124,
    0x21c640532670ac20, 0x5b1e309b16452559,
    0xd476a1c3461bbed2, 0xaeaed10b762e37ab,
    0x37deb6af5e9b8b5b, 0x4d06c6676eae0222,
    0xc26e573f3ef099a9, 0xb8b627f70ec510d0,
    0xf7e653dcc6da3dd4, 0x8d3e2314f6efb4ad,
    0x0256b24ca6b12f26, 0x788ec2849684a65f,
    0x9cf65a1b368f752e, 0xe62e2ad306bafc57,
    0x6946bb8b56e467dc, 0x139ecb4366d1eea5,
    0x5ccebf68aecec3a1, 0x2616cfa09efb4ad8,
    0xa97e5ef8cea5d153, 0xd3a62e30fe90582a,
    0xb0c7b7e3c7593bd8, 0xca1fc72bf76cb2a1,
    0x45775673a732292a, 0x3faf26bb9707a053,
    0x70ff52905f188d57, 0x0a2722586f2d042e,
    0x854fb3003f739fa5, 0xff97c3c80f4616dc,
    0x1bef5b57af4dc5ad, 0x61372b9f9f784cd4,
    0xee5fbac7cf26d75f, 0x9487ca0fff135e26,
    0xdbd7be24370c7322, 0xa10fceec0739fa5b,
    0x2e675fb4576761d0, 0x54bf2f7c6752e8a9,
    0xcdcf48d84fe75459, 0xb71738107fd2dd20,
    0x387fa9482f8c46ab, 0x42a7d9801fb9cfd2,
    0x0df7adabd7a6e2d6, 0x772fdd63e7936baf,
    0xf8474c3bb7cdf024, 0x829f3cf387f8795d,
    0x66e7a46c27f3aa2c, 0x1c3fd4a417c62355,
    0x935745fc4798b8de, 0xe98f353477ad31a7,
    0xa6df411fbfb21ca3, 0xdc0731d78f8795da,
    0x536fa08fdfd90e51, 0x29b7d047efec8728,
};

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

unsigned long crc64(const unsigned char *s, unsigned long len) {
  unsigned long crc=0;
  unsigned long j;
  
  for (j = 0; j < len; j++) {
    unsigned int byte = s[j];
    crc = crc64_tab[(crc ^ byte)&0xFF] ^ (crc >> 8);
  }
  return crc;
}

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

  //認証情報用
  unsigned char id[34]={'\0'};
  unsigned char pass[AUTHDATA_BYTE+2]={'\0'};
  unsigned long pass_num,A,pre_A,A2,pre_A2,B,F,alpha,beta,gamma,Ni,Nii;

  //各関数用バッファ
  unsigned char hash_buf[20]={'\0'};
  unsigned char rand_buf[AUTHDATA_BYTE]={'\0'};

  //暗号通信用
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'};
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};

  //実行時の入力エラー
  if((argc<3) || (argc>4)){
    fprintf(stderr,"Usage: %s <Server IP> <Server Port>\n",argv[0]);
    exit(1);
  }
  
  servIP = argv[1]; //server IP設定
  //ポート番号設定
  if(argc==4){
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
  
  //初回登録フェーズ
  
  //フラグを受け取る
  read(sock, &authent_flag, sizeof(unsigned short));
  
  if(authent_flag==0){
    printf("このユーザは未登録のため、初回登録を行います\n\n");

    //Niを作成
    Ni=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
    /////////////////////////////
    //
    printf("Ni->%lu\n",Ni);
    //
    /////////////////////////////
    
    //Niを保存
    if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(&Ni,sizeof(unsigned long),1,fp);
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
    //最大8文字の入力を受け入れる
    if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
      perror("PASS:1-8char\n");
      close(sock);
      exit(1);
    }
    for(i=1; i<AUTHDATA_BYTE+2; i++){
      if(pass[i]=='\n'){
	overflow_flag=0;
      }
    }
    if(overflow_flag==1){
      perror("pass:1-8char\n");
      close(sock);
      exit(1); 
    }
    overflow_flag=1;

    //改行を削除
    length=strlen(pass);
    if(pass[length-1]=='\n'){
      pass[--length]='\0';
    }

    //パスワードを数値に変換
    pass_num=0;
    for(i=0; i<strlen(pass); i++){
      pass_num=(pass_num<<8)+pass[i];
    }
    
    //Aを作成
    pre_A=pass_num^Ni;
    sprintf(hash_buf,"%lu",pre_A);
    A=crc64(hash_buf,strlen(hash_buf));

    //////////////////////////////
    //デバッグ用
    printf("Ni^S->%lu\n",pre_A);
    printf("A->%lu\n",A);
    //
    //////////////////////////////////

    //Aを送信する
    write(sock, &A, sizeof(unsigned long));

    close(sock);
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

  printf("PASS:");
  if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
    perror("PASS:1-8char\n");
    close(sock);
    exit(1);
  }
  for(i=1; i<AUTHDATA_BYTE+2; i++){
    if(pass[i]=='\n'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("pass:1-8char\n");
    close(sock);
    exit(1); 
  }
  overflow_flag=1;
  
  //改行を削除
  length=strlen(pass);
  if(pass[length-1]=='\n'){
    pass[--length]='\0';
  }

  //パスワードを数値に変換
  pass_num=0;
  for(i=0; i<AUTHDATA_BYTE; i++){
    pass_num=(pass_num<<8)+pass[i];
  }
  //パスワードを数値に変換
  pass_num=0;
  for(i=0; i<strlen(pass); i++){
    pass_num=(pass_num<<8)+pass[i];
  }
  
  //認証フェーズ
  if((fp=fopen("./../client_data/Ni_file.bin","rb"))==NULL){
    perror("Ni FILE OPEN ERROR\n");
    close(sock);
    exit(1);
  }
  fread(&Ni,sizeof(unsigned long),1,fp);
  fclose(fp);
  
  //Aの作成
  pre_A=pass_num^Ni;
  sprintf(hash_buf,"%lu",pre_A);
  A=crc64(hash_buf,strlen(hash_buf));
  
  //////////////////////////////
  //デバッグ用
  printf("Ni^S->%lu\n",pre_A);
  printf("A->%lu\n",A);
  //
  //////////////////////////////////
  
  //Ni+1を作成
  Nii=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
  ///////////////////////////////
  //
  printf("Ni+1->%lu\n",Nii);
  //
  //////////////////////////////
  
  //Ai+1を作成
  pre_A2=pass_num^Nii;
  sprintf(hash_buf,"%lu",pre_A2);
  A2=crc64(hash_buf,strlen(hash_buf));
  
  /////////////////////////////////
  //
  printf("Ni+1^S->%lu\n",pre_A2);
  printf("Ai+1->%lu\n",A2);
  //
  /////////////////////////////////

  //B=H(Ai+1)を作成
  sprintf(hash_buf,"%lu",A2);
  B=crc64(hash_buf,strlen(hash_buf));
  
  ////////////////////////////////
  //
  printf("B->%lu\n",B);
  //
  ////////////////////////////////
  
  //alphaを作成
  alpha=A2^(B+A);
  ///////////////////////////////
  //
  printf("alpha->%lu\n",alpha);
  //
  //////////////////////////////
  
  //betaを作成
  beta=B^A;
  /////////////////////////////
  //
  printf("beta->%lu\n",beta);
  //
  ////////////////////////////
  
  //alphaを送信する
  write(sock, &alpha, sizeof(unsigned long));
  
  //betaを送信する
  write(sock, &beta, sizeof(unsigned long));
  
  //gammaを受け取る
  read(sock, &gamma, sizeof(unsigned long));
  ////////////////////////////
  //
  printf("gamma->%lu\n",gamma);
  //
  ////////////////////////////
  
  //F=H(B)を作成する
  sprintf(hash_buf,"%lu",B);
  F=crc64(hash_buf,strlen(hash_buf));
  
  ////////////////////////////////
  //
  printf("F->%lu\n",F);
  //
  ////////////////////////////////
  
  if(gamma-F != 0){
    perror("gamma and F are not equal\n");
    perror("authentication error!\n");
    close(sock);
    exit(1);
  }
  else{
    printf("gamma and F equal\n");
    
    //次の認証情報をファイルに保存
    if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(&Nii,sizeof(unsigned long),1,fp);
    fclose(fp);
  }
      
  //暗号通信フェーズ
  puts("-----------------------");

  for(i=0; i<AUTHDATA_BYTE; i++){
    vernamKey[i]=(A2 & ((unsigned long)0xFF<<56-8*i))>>56-8*i;
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
  printf("平文(上限8文字)：%s\n",vernamData);
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
  vernamData[i]='\0';
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
