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

## ボード

* 68008 + RAM + Teraterm/minicom (端末の代用)
* スイッチパチパチ＋紙テープリーダパンチャの再現
* PCからTeraTerm/minicomでシリアル接続、手で打ち込む。
* 紙テープリーダ、ライター: ホスト側ダンプファイルをボード上RAMに書き込み、とボード上RAMの内容をホストファイルにダウンロード。TeraTermマクロで作成、マクロ実行(メニューから実行、Drag&Dropはできない様子)

## 機能

* データを手で打ち込む...モニタのSコマンド(RAMに書き込む)
* RAMのデータを見る...モニタのダンプコマンド
* テープに書き出す...モニタのIntel HEX　DUMPコマンド＋TeraTermマクロ(メニュー呼び出し)
* テープからRAMにロード...モニタのIntel HEX Loadコマンド＋TeraTermマクロ
* フラッシュに書き込む...モニタのSVコマンド(開始番地、長さ、セクタ番号を指定)
* フラッシュから読み出す...モニタのLDコマンド(開始番地、長さ、セクタ番号を指定)

## 回路

* 68008, 128kRAM, PIC18F47Q43の3チップ構成  
  最初は68008, PIC18F47Q43の2チップ構成
* 128kRAM: A0-A17(3チップ構成)  
  6kBRAM: A0-A12(2チップ構成)
* PIC: A0-A13 + A19(CE/) + CE2
* 電源はUSBシリアルから取る。
* クロックはPICから提供。

||3チップ構成|2チップ構成|
|--|--|--|
|RAM容量|128kB|6kB|
|アドレスバス|A0-A17|A0-A12|
|空間|00000-3FFFF|00000-017FF|
|実体|128kRAM|PIC内蔵RAM|

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
> 
> Halt (HALT)  
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

* ハード変更、アドレス線を1本割り込み入力(IPL1)に割り当てる。
* 割り込み後のアクノレッジサイクル対応: 
  + ベクタリード(D0-D7)するのでデータバスにベクタを供給する
  + PC保存、他。
* 割り込みアクノレッジ以下全てPICに対応させるとすると、この順番で対応。
* DTACK/HOLDで止めて、割り込み線をLにする。次のサイクルでベクタ載せ
  以後順にメモリアクセスが来る。各サイクルの順序を覚えておいて
  間違いないように命令・データを置いてゆく。

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
  + レジスタ退避は割り込み処理内部に任せる。
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

## シングルステップ

DTACKをアサートしない限り、アドレスバスとデータバスに有効データを出したままメモリアクセスは継続する。よって、各サイクルごとにそれらを読み込んでダンプすればシングルステップは可能である。

## DTACK制御

RAM搭載するときに、RAMアクセスのときだけDTACK no wait にしたい、他はソフトでONにするまで動かない。としたい。

* 通常動作の時、DTACK <-- A19 とする。CLCを使う。このとき、RAMアクセス時はDTACKは常時アサートされている。CPU のノーウェイトアクセスで動く。I/Oアクセス時は常時停止する。
* シングルステップ動作・命令置き動作の時、DTACKは常時Hとする。CPUはWAIT状態(メモリサイクル完了待ち)に入る。処理が終わると、PIC側でDTACKをアサート(Lに)する。DSがHになるまで待ってDTACKをネゲート(Hに)する。

素直に読むと、CLCの入力は、GPIO端子か周辺デバイス出力のいずれかであり、CLCの入力を直接ソフトで触る方法は見つかっていない。ならば、使っていなくて1/0出力をソフトで制御できる周辺デバイスをCLCの入力とすればよいと気づく。

CWF (Complementary WaveForm Generator)がよさそう。常時1/0にするビットがあり、出力をCLCの入力とすることもできる。

そこまでしなくても、通常動作とシングルステップ動作でCLCのコンフィギュレーションを変えるか。

敢えて言えば、DTACKのネゲートをCLCのフリップフロップにやらせる手もありそうだ。やり方はまだよくわからんが。

## 命令置き

「命令置き」は私が作った術語である。CPUはROM/RAMから命令フェッチして動くが、その命令フェッチ時にROM/RAMをディスエーブルしてPICがデータバスに代わりの命令をセットする。このセット動作を「置く」と表現している。何も知らずに進んでくるCPUに食わせる命令を黙ってすり替えるイメージである。

「命令置き」で、Readサイクルのみ命令を置き、WriteサイクルではRAMにそのままスルーすることで、任意のアドレスへのデータ書き込みが可能となる。RAMに供給されるアドレバスすべてが接続されていなくても、CPU が発行するアドレスに載せてCPU自身に書き込ませる。任意のアドレスにデータを書き込むことができる。

[Z80-MBC2](https://hackaday.io/project/159973-z80-mbc2-a-4-ics-homebrew-z80-computer)が命令置きを使っている。コントローラであるATmega32Aに接続されているアドレスバスはわずか2本だけと徹底しています。Just4Fun氏は [68k-MBC](https://hackaday.io/project/177988-68k-mbc-a-3-ics-68008-homebrew-computer)も作成しており、これも命令置きを使っている。

私自身もこれまでZ80とF8で命令置きを行った経験がある。68008でも似たようなものだろうと簡単に考えてきたが、そこまで世の中は甘くない。「次のReadサイクルが命令フェッチかオペランドデータ読み出しかを信号を見て判別することができない」のである。信号線を見てわかるのは、このメモリサイクルがReadかWriteかということだけで、信号線を見ているだけでは、このサイクルでCPUに食わせる命令を置いて大丈夫かが判断できない。

このメモリサイクルは間違いなく命令フェッチである、というサイクルを押さえておいて、以後、途切れることなく命令置きを続けなければならない。68008の場合は、リセット直後か、割り込み・例外発生直後からとぎれず追いかけることになる。

今回は、割り込みを発生させアクノレッジサイクルを検知する。アクノレッジサイクルはA0-A19 all 1(A1-3除く)なので、見える範囲のアドレスバスが全て1であるｋとおをもってアクノレッジサイクルとみなす。

アクノレッジを検知すると、PICはデータバスにベクトルを載せ、DTACKをアサートする。読み書きとも、アドレスレジスタに目標アドレスをセットし、データレジスタに書き込みたいデータをセットし、ポストインクリメントのデータレジスタ書き込み命令を置いてゆく。WriteアクセスのみRAMに向かわせるようにする(CE2==Hにする)。

* モニタ機能のバイトフェッチ・デポジット: プログラム実行中なら割り込み後に2バイト読み書きプログラムの命令を置く。1バイト読み書きだと基数番地にアクセスした直後にバスエラーが発生してしまうので。プログラム停止中ならリセット後の命令置き。
* ブートローダ機能: リセット後の命令置き。

### 参考: 紙テープリーダ

* パラレルポート接続
* STA信号をアサートするとテープ走行開始する
* テープには送り歯車がはまる穴がある。これがデータ読み込み友好の時にスプロケット信号になる。
* スプロケット信号がLになるときにデータが有効
* STAアサート、スプロケット信号Lを待ってデータを読み込み
* を繰り返し、終了するとSTAを止める
* 紙テープの果ての検出は、スプロケット信号が一定時間の間ONOFFしない、になるのだろう。

### 過去の68000 Forth実装調査

[Easy68k](http://www.easy68k.com/paulrsm/mecb/forth.zip)
*	68000 Registers used in eForth model
*	A7 = SP = Data Stack Pointer, SP
*	A6 = Return Stack Pointer, RP
*	A5 = Interpreter Pointer, IP
*	Stack and all variables and tokens are 32-bits wide.

## m68k-elf-gccのインストール

https://github.com/ddraig68k/m68k-elf-toolchain の手順に従い入れてゆく。

> 失敗しないようにするためには、
> 1. Makefileの書き換え: `binutils-gdb`のリポジトリを更新する  
> `https://github.com/bminor/binutils-gdb.git`
> 2. bisonのインストール。
> 3. インストール先に自分が書き込めるようにしておく。  
>   `/opt/m68k-elf/` ディレクトリのパーミッションに`chmod o+w`しておく。
>
> が必要だった。

* 必要なツールを入れる。bisonが抜けていたので、それも足しておくこと。
```sh
$ sudo apt install make git gcc g++ lhasa libgmp-dev libmpfr-dev libmpc-dev flex gettext texinfo bison
```

* `make update` で失敗する。binutils-gdbのリポジトリが古いらしい。  
ググってそれらしいところを見つけた(https://github.com/bminor/binutils-gdb)ので
それと入れ替える。

```make
#GIT_BINUTILS         := git://sourceware.org/git/binutils-gdb.git
GIT_BINUTILS         := https://github.com/bminor/binutils-gdb.git
```

* これで`make update`成功した。
* `make all`中、configure/build中のメッセージが出てこないので静か。
* make allに失敗する。bisonがないと言っている。

```sh
$ sudo apt install bison
```
* `make binutils`に失敗する。/opt/m68k-elfがmkdirできないと言っている。
* `sudo make all`する。`/bin/bash: /projects/gcc/configure: No such file or directory`と言われる。ルートがおかしい?
* `sudo make all`すべきではなさそうなので、`/opt/m68k-elf` に自分が書き込み可能な状態にしておく。乱暴すぎて怖いが `sudo chmod o+w /opt/m68k-elf` した。
* これで make binutils, install binutilsは完了した。
* configure gccも完了し、現在 make gcc 中。
* なんか寂しいので、別窓シェルにて `tail -f 'log/make gcc.log'`を実施中(^^)
* 結局これでビルドが完了し/opt/m68k-elf/bin下にツールが格納された。

## m68k-elf-asの文法

> gun-assembler.pdf に記載がある。

* コメント: `;` -> `/* ... */`
* 文字定数: "a" -> 'a'
* 16進数: $1F5E -> 0x1F5E
* レジスタ: a7 -> %a7, d0 -> %d0
* EQU: start equ $80 -> .set start, 0x80
* ORG: org start -> .org  start

> Sun assembler準拠らしいので、ポストインクリメントは (a7)+ -> %a7@+ が正式だそうだが、(%a7)+ も受け入れてくれるらしい。



