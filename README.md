# EMU68k改デザインメモ書き

* FORTHが生まれた当時のプログラミングを体験する。FORTH立ち上げを経て体験する。
* Mooreさんいわく、1968-70当時は16kワード、パネルスイッチとランプだけでプログラミングを始たとのこと。
* 最初のコードはシリアルI/Oとメモリダンプを動かすところから始める。
* 8bit CPUでは面白くない、ちょっとしっかりした16bit CPUがよい。
* 手元にMC68008がある。機械語にも詳しくないので、この機会に勉強できて良いかな。
* CPU/RAM/シリアルIO/外部ストレージ(RAMを保存できる)
* 最初の目標が、シリアルI/Oとメモリダンプ、それとRAM保存。
* EMU68k8, 2チップ構成をベースとする。
* RAMが不足するので、128kB(1Mbit)SRAMを追加する。これで64kワードまでサポートできる。当時のコンピュータの雰囲気を出すことができる。
* RAMを保存する仕掛け: SDカード/QSPI FLASHあたりが妥当だが、例によってピンが足りない。クロックのみ専用線として、あとは68008を止めた状態でバスを解放させて行う。
* 最初は外部記憶を使わない。メモリのセット・メモリダンプ・シリアルへのダンプとリストアをPICに組み込む。

## 当時のコンピュータの雰囲気

* 16bitマシン、
* 16kワード程度、これでも大きい方。
* パネルのスイッチでアドレス・データをセットしてスイッチを押してRAM(コア)に書き込む。
* ディスクはある。数MB程度のハードディスクと推察。
* ファイルシステムは存在しない、ブロックごとにデータを保存する。ブロック番号に何が入っているかをノートにメモして管理する感じ。
* セクタリードライトだけを使う。

## 回路

* 68008, 128kRAM, PIC18F47Q43の３チップ構成
* 128kRAM: A0-A17, 
* PIC: A0-A16 + A19(CE/) + CE2
* 電源はUSBシリアルから取る。
* クロックはPICから提供。

## PIC機能

* CPUコントローラ: 仮想I/Oデバイス(シリアルI/O)、メモリローダ、
* 仮想I/Oデバイス: A19==1でI/O空間、A19==0でRAM有効
* シリアルI/O: A19==1のどこかのアドレス($80000, $80001)に仮想UARTのステータスレジスタとデータレジスタを置く(==PICのUARTレジスタをそのまま返す)
* メモリローダ: とりあえずHALTでCPU止めてSRAMに直接書き込む。
* シングルステップ: DTACKを返さずにメモリアクセス待ちで止める。Teraterm画面に実行中の命令をダンプする。
* バス乗っ取り: CE2==0にしてSRAMをdisableにできる。この状態でRESET解除してCPUのメモリアクセスをすべて押さえる。
* モニタ: シリアルI/O, メモリセットとメモリダンプ(視認)、ホストマシンへのメモリダンプとリストア。

## メモリマップ

|開始|終了|説明|
|--|--|--|
|00000|03FFF|RAM(ブートコード展開可能/ダンプ可能)|
|04000|1FFFF|RAM|
|80000|803FF|ディスクバッファ（PIC内部)|
|80400|...|シリアルI/O(データレジスタ(R/W))|
|80401|...|シリアルI/O(コマンド(W)/ステータス(R))|
|80402|...|ページ番号(H)|
|80403|...|ページ番号(L)|
|80404|...|コマンドワード(W)/ステータス(R)|

## ミニモニタ

* ESC2回連打で実行を停止、モニタモードに入る。  
  実行はHALTで停止させる。
* S: メモリへのセット(シリアル接続時)
* D: メモリダンプ(シリアル接続時)
* G: ターゲットプログラム実行、同時にターゲットモードに入る。
* R: メモリのセーブ(Linuxコマンドに戻ってダウンロードを実行)
* W: メモリのリストア(Linuxコマンドに戻ってアップロードを実行)

## アセンブラ

* Linuxで動くアセンブラ。ハンドアセンブルはしんどい。
* crownassemberというのを見つけた。bison/flexでm68kアセンブラが記述されている。これでアセンブリリスト出力し手で打ち込むイメージ
* HEXを目視して手でSで打ち込むイメージ

## 電源投入時の挙動

* PIC起動前
  + RESET == L(プルダウン), クロック提供無し  
    これで出力が3stateを期待。
  + CE/ == H(プルアップ)
* 起動
  + PIC起動、
  + HALT == Lにする。
  + クロック提供開始
  + HALT解除、RESET解除(できれば同時に行う)

> Reset (RESET)
The external assertion of this bidirectional signal along with the assertion of HALT starts
a system initialization sequence by resetting the processor. The processor assertion of
RESET (from executing a RESET instruction) resets all external devices of a system
without affecting the internal state of the processor. To reset both the processor and the
external devices, the RESET and HALT input signals must be asserted at the same
time.
Halt (HALT)
An input to this bidirectional signal causes the processor to stop bus activity at the
completion of the current bus cycle. This operation places all control signals in the
inactive state and places all three-state lines in the high-impedance state (refer to Table
3-4).
When the processor has stopped executing instructions (in the case of a double bus
fault condition, for example), the HALT line is driven by the processor to indicate the
condition to external devices.
Mode (MODE) (MC68HC001/68EC0

RESET:  
* 双方向、HALTと同時にLにするとシステム初期化を開始する。
* RESET 命令実行によりL出力される。このときCPU内部はリセットされない。
* AS,DS,R/WはHigh-Zにならない。リードモードになる。

> * PICが直接RAMアクセスするには、BRでバスを空けてもらうしかない。現在はBRはH固定なので、それはできない。ということで、PICは命令置きをするしかない。
> * または、PICがAS,DS,R/Wを仲介する手もあるが、配線切り替えが必要。

HALT
* 入力、実行停止、制御信号をinacitive, 全3-state信号をフローティングにする。
* とはいえAS/DS/RWは3-stateにならない。非アクティブ(High)になるだけ。
* SPI信号(SCLK以外)はアドレスバスまたはデータバスから割り当てできる。  
  SCLKは専用に線を割り当てる(CPU動作中にクロックを振ってはいけない)

### リードサイクル

|STATE|説明|
|--|--|
|0|FC0-FC2, R/W|
|1|Address/Data bus|
|2|AS, DS asserted|
|3|(none)|
|4|wait for DTACK or BERR or VPA,|
|5|(none)|
|6|デバイスがデータをD0-D7に載せる|
|7|falling edgeでCPUがデータバスをラッチする。AS,DSをHに戻す。デバイスはDTACK or BERRをLにする。<br>riging edgeでアドレスバスをHigh-Zにする<br>ここでデバイスはDTACK or BERRをHに戻す|
|||

### ライトサイクル

|STATE|説明|
|--|--|
|0|FC0-FC2, R/WをHに|
|1|Address/Data bus|
|2|rising edge, AS == L, R/W == L|
|3|データバスをHigh-Zから脱しデータを載せる|
|4|DS ==L, wait for DTACK or BERR or VPA,|
|5|(none)|
|6|(none)|
|7|falling edgeでAS,DSをHに戻す。デバイスはDTACK or BERRをLにする。<br>riging edgeでアドレスバスをHigh-Zにする<br>ここでデバイスはDTACK or BERRをHに戻す|
|||

### CPU space cycle

* A16-19 == H, 割り込みアクノレッジ, このときA1-3に割り込みレベルが出る。
* 割り込み信号のサンプリング、各命令実行のS4 fallingでサンプリングされる。
* 次のサイクルが割り込みアクノレッジ、FC0-2 all Hとなる。
  このときDSもLになるがCPUは何もしない(データバスサンプリングもしない)

### クロック下限

下限は 2.0MHz(パルス幅max 250ns, サイクル幅max 500ns)である。クロックストレッチはできません。DTACKがあるのでアクセス時間延ばしは問題なく実現可能だが。

## CPUのRAMアクセス

現在は、
* 00000-7FFFFまで(A19 == L)ならばCPUはRAM直接アクセスする。
* ただし、DTACKだけはPICが制御するので完全フルスピードというわけではない。

CTCでA19 == L && DS == Lの時にDTACK == Lとするようにすればノーウェイトでアクセスできるが、実現可能性の検証が必要。


## PICからRAMにデータアクセスする方法

RESET, HALTともバスを空けないことが分かった。となるとアプローチは3つしかない。

1. ハード改造、PICにBR,BGをつなぎPICがバスを専有する。この場合、アドレス線からさらに2本をBR,BGに回す。実行中のプログラムコンテキスト(レジスタ内容、PCがさす場所)を変えずにRAMにアクセスできる。
2. ハード改造、SRAM-CPU間のWE,OEを外し、PIC-SRAM間とする。全アクセスをPICのタイミングで行う。現在でもDTACKはPICがあげるので、タイミング的にフルスピードで動くことはない。さらに遅くなってもかまわないという考え方。
3. 現ハードのまま、命令置きを行う。クロック置きしなくても、DTACKでCPUへの返しを待たせており、RAMのCE2もコントロールできるので、READサイクルだけPICが置いてWRITEサイクルだけRAMに書き込ませることは可能。アドレス線がIOアドレスのでコード以外は不要となり解放できる。

要件は
* メモリピークポーク: CPU実行コンテキストを保持したうえで。
* ブートローダ：起動時にRAM上にデータを展開する。

全RAM分のアドレスバス信号をPICに与えることはできない(PIC のピン数が足りない)ので、命令置きを使うしかない。メモリfetch/depositなら、

* PC保存、アドレスレジスタ1本保存(call命令＋push命令)
* アドレスレジスタに値をセット(直値で)
* アドレスレジスタ関節でメモリR or W
* アドレスレジスタとPCを戻す(スタックからPCをPopして値を巻き戻してPCにセット)。
* フラグ保存に注意する必要がある。

と思ったが、「次のアクセスがインストラクションフェッチ」を知る方法がない。アクセス状態をトレースするには割り込みかリセットが必要。リセットでは現在のPCを得られないので、割り込みを掛けて載せるしかないか。

## RAMデータアクセスの方法(割り込み編)

上3つはいずれも不完全なので忘れて、以下の方針で進める。、

* ハード変更、アドレス線を1本IPL0/2(or INT1)に割り当てる。
* 割り込み後の空くノレッジサイクル対応: 
  + ベクタリード(D0-D7)するのでデータバスにベクタを供給する
  + PC保存、他。
* 割り込み開くノレッジ以下全てPICに対応させるとすると、この順番で対応。
* DTACK/HOLDで止めて、割り込み線をLにする。次のサイクルでベクタ載せ
  以後順にメモリアクセスが来る。

## 例外処理

* 割り込みベクタ(周辺がデータバスに載せる)に基づき割り込みベクタを決める。0-3FFまで1kB
* PIC内アドレスとしては、0-3FFはRAM配列を割り当てず、ソフトで発生させる。
* RAM(プログラム置き場)は0400Hからとする。
* 例外スタックフレーム: 割り込み処理の最初でレジスタを保存する。

## 改訂版

* 命令置き: Read CycleはPICから、Write CycleはSRAMへ。  
  全アクセスPICがチェック、Read CycleはCE2==L, Write CycleはCE2==Hにする。
* 命令置きは割り込み処理で開始する。
  + IPL1をLにして開始。
  + レジスタ対比は割り込み処理内部に任せる。
  + 割り込み処理中の命令はPICが置く。
* 命令置きで行う処理は、ブートローダ、RAM fetch/depositとする。
* CE2に1ピン割り当てる必要がある(RB6を割り当てる)。
* SDCard(SPI)のSCLKに1ピン割り当てる必要がある(RB5を割り当てる)

## ピン割り当て(改訂)

|ピン名|割り当て|
|--|--|
|RE0|RESET/|
|RE1|IPL1/|
|RE2|A19 (MEM/IOとして扱う)|
|RD7|TEST|
|RD6|A14 / CE2 on SRAM (ジャンパ切替え)|
|RD5|A13 (SDカード搭載時にSCLKに切替え)|

## メモリマップ(改訂)

上記を受けて改訂。
* PIC内蔵メモリと外部SRAMの区分を明確化
* PIC接続のアドレスバス線は13本(A0-A12)を想定

### 内蔵版 4kB

|開始|終了|size|実体|説明|
|--|--|--|--|--|
|00000|003FF|1kB|ソフト生成|ベクタ|
|00400|013FF|4kB|内蔵RAM|コード/データ|

### SRAM版 128kB

|開始|終了|size|実体|説明|
|--|--|--|--|--|
|00000|003FF|1kB|外部RAM|ベクタ|
|00400|013FF|4kB|外部RAM|コード/データ|
|01400|1FFFF|124kB|外部SRAM|コード/データ|
|80000|803FF|1kB|ディスクバッファ（PIC内部)|
|80400|...|1b|シリアルI/O(データレジスタ(R/W))|
|80401|...|1b|シリアルI/O(コマンド(W)/ステータス(R))|
|80402|...|1b|ページ番号(H)|
|80403|...|1b|ページ番号(L)|
|80404|...|1b|コマンドワード(W)/ステータス(R)|
