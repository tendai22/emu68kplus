# EMU68k改: アップロードの処理余裕
EMU68k改では、TeratermからのテキストアップロードによりRAM上にコードを展開している。この手の機能では、処理が間に合わず取りこぼしが生じるとよく言われる。

取りこぼしは、読み込んだ文字をエコーバックする際に、文字送信完了を待っている時間が無駄になり、正味の処理時間がほとんど取れないことによると考えている。UARTから文字データを送信している間、内部処理・正味の処理はできない。内部ループは送信完了を待っている。

1文字受信してから次の文字を受信するまでの間と、1文字送信する時間はほぼ同じなので、その差で内部処理するのなら正味の時間はほとんど取れるはずがない。割り込みと送信バッファで内部処理を待たないようにしても、送信バッファがふんづまったらそこで終わりです。なので、TeraTermが送信する側で1文字送信するごとに待ち時間を設けて、内部処理の時間を稼ぐ方法が普通に用いられている。

考えてみれば、文字送信を止めてしまえば、1文字読み終えて処理を開始してから次の文字を読み終わるまでの間、115200bps が11520byte/sec → 逆数が86.8us。これが1文字受信してから次の文字を受信し終わるまでの時間、内部処理に充てられることになる。

ということでロジアナで波形を見てみました。#1がUART TX(アップロードデータ), #2が内部処理、Lが受信データの処理に相当し、Hがデータ到着待ちに相当します。

<img width=500 src ="img/02-1-upload-performance-anntated.png">

1文字86usのうち、内部処理は4～5us、内部でASCII -> 16進変換して、区切りが来たら内部RAMに書き込むだけの処理なので数usで済むというのも納得です。ということで、Teratermの待ち時間を入れなくても余裕は十分あることがわかりました。