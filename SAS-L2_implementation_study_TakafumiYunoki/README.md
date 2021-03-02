### コンパイラのバージョン
gcc 9.3.0
### 実行ファイルのサイズ
※program/source/にあるソースの実行ファイルのコードサイズを示したものである。  
  
**SAS-2**  
* 32bit  
  * server:22.3KB  
  * client:21.4KB  
* 64bit  
  * server:26.3KB  
  * client:21.4KB  
* 256bit  
  * server:22.4KB  
  * client:21.5KB  
  
**SAS-L2**  
* 32bit  
  * server:22.4KB  
  * client:17.2KB  
* 64bit  
  * server:26.4KB  
  * client:17.2KB  
* 256bit  
  * server:22.5KB  
  * client:17.2KB  
### 各フォルダの概要
認証情報のデータ長(32bit/64bit/256bit)ごとに以下のフォルダを設けている。  
* client_data(被認証側のデータ保存場所)  
* server_data(認証側のデータ保存場所)  
* exp(SAS-2の評価実験用ソース)  
* source(SAS-2のソース)  
### 実行方法
プログラムの実行方法はthesis.pdfの4.4節を参照されたし。  
source内のプログラムについては、「認証フェーズの繰り返し回数」を考慮しないため、  
client.c の実行には、thesis.pdfの4.4節に示すコマンドから「認証フェーズの繰り返し回数」を省略した形となる。
