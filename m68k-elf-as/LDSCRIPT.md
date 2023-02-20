# EMU68k改: アセンブラ実行環境を整える

`m68k-elf-as -c trip.o trip.s`で作ったオブジェクトファイルからアップロード用16進ダンプファイルを作っていると、ラベルをミススペルしていてもアセンブルエラーが出てこないことに気づいた。最初は目でオペランド0000の箇所をチェックしていたが、これではやっていられない。改善しないと使い物にならない。

考えてみれば、オブジェクトファイルを作る時点で未解決のシンボルが出てくるのは当たり前であり、エラーチェックさせるためにはリンクまで済ませないといけない。ということで、リンカも通すことにした。

リンカスクリプトも整えて得たものが以下の通り。

アセンブラ起動用スクリプト`as.sh`:
```
#! /bin/sh
b=`basename -s .s "$1"`
m68k-elf-as -o ${b}.o "$1" &&
m68k-elf-ld -T trip.ldscript ${b}.o &&
( m68k-elf-objdump -D a.out | tee ${b}.list )
```
使用時には
```
$ sh as.sh trip.s
```
と書く。ソースファイル(例えば`trip.s`)引数を1個取る。するとアセンブルして`trip.o`を生成し、
そのあとリンカ`m68k-elf-ld`を実行する。最後で生成されたオブジェクトファイル`a.out`の逆アセンブルリストを出力して終わる。

`a.out`はELF形式で、ターゲットマシンにアップロードするには更に処理が必要である。既に`dump.sh`スクリプトを作ってそこでやらせているので、通常と異なり、`a.out`は単なる中間ファイルである。同時に複数のアセンブラを走らせることもないので、重ならない一時ファイル名を生成することもしない。

各コマンドの間に`&&`を挟んでいる。これは、シェルスクリプトにおいて、前のコマンドが成功したときのみ次のコマンドを実行するという意味で、命令やラベルのスペルミスがあると、アセンブラやリンカが実行失敗となり、そこで停止するようになっている。

ldの-Tオプションに注意、これを-cオプションに変えると、
```
$ m68k-elf-ld -c trip.ldscript trip.o
m68k-elf-ld: unrecognised keyword in MRI style script 'MEMORY'
```
と言っておこられます。ググって見つかるスクリプトには`-T`オプションだ。

最後のリストファイル生成時に画面出力させるのは趣味です、まぁ、実機でデバッグ実行時に必要なので画面に出すようにしているだけです。ふつうはこんなことはしないで黙って`trip.list`ファイルだけを生成するものです。

リンカスクリプト`trip.ldscript`:
```
MEMORY {
    RAM     (rw)    : ORIGIN = 0x0, LENGTH = 6k
    STACK   (rw)    : ORIGIN = 0x1700, LENGTH = 256
}

SECTIONS {
    .text : {} > RAM
    .data : {} > RAM
    .bss  : {} > RAM
    .stack : {} > STACK
}
```
ターゲットマシンであるEMU68k改では、オールRAMなのでこのように簡単な構成になっている。スタックのところもなくてもかまわないぐらいである。セクションも .text のみでもかまわないだが、いちおう恰好を付けて入れておいた。

これでちゃんとリンクしてくれたので、セクションとか気張らなければリンカスクリプトも意外に簡単なものだ。`m68k-elf-gcc`のビルドを通しており68008用のCコンパイラも動作するので、ライブラリ無しでよければ(startupさえ書けば)Cコンパイラで生成したバイナリも実行できる。これも暇をみてやってみたい。その前にRAM 128kB化が先だな。

## 参考: アップロード用ファイル生成 `dump.sh`

リストファイルからアップロード用の16進ファイルを生成する。
例えば、実行開始後即無限ループにハマるアセンブラソース`test.s`

test.s:
```
/*
 *  test.s ... simple infinite loop.
 */
/*  definitions */
 .equ ram,  0
 .equ start,  0x80

   .org     ram
    dc.l    0x1000
    dc.l    start
   .org     start
main:
    bra.b   main
```
をアセンブルすると、以下のリストファイルを得る。
```
a.out:     file format elf32-m68k


Disassembly of section .text:

00000000 <main-0x80>:
   0:   0000 1000       orib #0,%d0
   4:   0000 0080       orib #-128,%d0
        ...

00000080 <main>:
  80:   60fe            bras 80 <main>
```
これを
```
=0 0000 1000
=4 0000 0080
=80 60fe
```
に変換するのが、シェルスクリプト`dump.sh`である。

中身はsedスクリプト、`  80: `を`=80 `に変換して、うしろのニーモニック部分を削っているだけです。
```
#! /bin/sh
m68k-elf-objdump -D "$@" |sed -n '
/^ *[0-9A-Fa-f][0-9A-Fa-f]*:/{
    s/^\([^     ]*\)    \([^    ]*\)    .*$/\1 \2/
    s/^ *\([0-9A-Fa-f][0-9A-Fa-f]*\): */=\1 /
    s/  *$//
    s/  */ /g
    p
}
'
```