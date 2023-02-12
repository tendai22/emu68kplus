# m68k-elf-gccのインストール

https://github.com/ddraig68k/m68k-elf-toolchain の手順に従い入れてゆく。

> 失敗しないようにするためには、
> 1. Makefileの書き換え: `binutils-gdb`のリポジトリ先を更新する  
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
