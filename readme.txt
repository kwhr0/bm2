日立ベーシックマスターJr.(MB-6885)エミュレータ bm2 マニュアル


* 概要 *
 日立ベーシックマスターJr.(MB-6885)のエミュレータです.
 モニタサブルーチンの一部をエミュレートしているため, ROMイメージがなくてもマシ
ン語のプログラムを実行できます.
 実機のROMイメージは300baudのもののみで動作を確認しています.

 次の機能が使用できます.
 ・CPU
 ・メモリ
 ・キーボード
 ・タイマ
 ・テキスト表示
 ・グラフィック表示
 ・サウンド出力
 ・テープ入力
 ・テープ出力
 ・カラーアダプタ MP-1710
 ・モニタサブルーチンの一部(ROMイメージがない場合)

 次の機能は使用できません.
 ・プリンタ出力


* インストール *
 1.SDLの入手
   Linux, MacOSXでソースからmakeする場合はSDL2.0(Simple Directmedia Layer)の
   Development Libraryが必要となる.

   win32バイナリのSDL Runtime Libraryはアーカイブに含まれているので別途入手する
   必要はない.

 2.ベーシックマスターJr.のROMをコピーする.

   アドレス  | ファイル名
   -----------------------
   B000-E7FF | b000-e7ff.s
   F000-FFFF | f000-ffff.s

   詳細はbmtape2.zipのドキュメントを参照すること.

 2'. 追記
   ROMイメージと同じディレクトリにBMP画像をコピーするとフォントを変えることができる.
   添付のchr.bmpを参照すること.

   ROMイメージは よ氏 のeMB-689Xのもの(bas.rom, prt.rom, mon.rom, font.rom)を使うこともできる.

 3.設定ファイルbm2configを
   o Linux, MacOSXの場合は名前を .bm2config としてホームディレクトリにコピーす
     る.
   o win32の場合は \Documents and Settings\<ユーザ名>, またはbm2.exeと同じフォ
     ルダにコピーする.


* 設定 *
 bm2configファイルを編集すると設定を変更できる.
 <...>は適当な文字列, |は1つを選択することを表す.

 rom_dir    <ROM directory>
     ROMイメージのあるディレクトリ(フォルダ)を指定する.
     ホームディレクトリは ~ と書くことができる.
     空白にするとモニタサブルーチンをエミュレートする.

 clock    <CPUクロック周波数>
     CPUのクロック周波数を設定する. 単位はkHz.
     デフォルトは750.

 ram_size    16k|64k
     RAM容量を指定する.

 mp1710    y|n
     yのときカラーアダプタMP-1710をエミュレートする.

 display   c|g
     cのときカラーディスプレイ, gのときグリーンディスプレイとする.

 zoom    <倍率>
     画面の倍率を設定する. (1〜4)

 full_line   y|n
     全ての行を表示する.

 sound   y|n
     yのとき音を出力する.

 keyboard    jp|en|de
     キーボードを設定する. (jp:日本語, en:英語, de:ドイツ語)


* 起動 *
 o bm2の実行ファイルをダブルクリックする.
       ROMイメージがある場合はBASICを起動する.
       (テープイメージファイル名はnoname.bin)
       ない場合は擬似モニタを起動する.

 o bm2のアイコンにテープイメージファイル(bmtapeで作成される*.binファイル)をドロ
   ップする.
       ドロップしたテープイメージを使用する.
       ROMイメージがない場合は無効.

 o bm2のアイコンにモトローラSレコード形式のプログラムをドロップする.
       プログラムをロードする.

 o コマンドラインで
   bm2 <モトローラSレコード形式> <アドレス(16進数)>
   とする.
       プログラムをロードし指定したアドレスから実行する.
       ROMイメージがある場合は無効.

 また
 bm2 -[option]=[value]   (optionは設定ファイルの項目名, valueは値)
 とすると設定を変更できる.

 例: bm2 -sound=n
 音を出力しない.


* 操作 *
 キーの配置は次のとおりである.
 左のShift: 英記号
 左のCtrl : 英数
 右のShift: カナ記号
 右のCtrl : カナ
 BackSpace: DEL
 ESC      : BREAK RESET
 PageUp   : テープの読み込み位置, 書き込み位置を先頭にする
 PageDown : テープの書き込み位置を末尾にする
 Alt + F  : テープイメージを選択する. またはバイナリ(モトローラSレコードファイ
            ル)をメモリにロードする.
 Alt + P または マウスの中ボタン: クリップボードのテキストをキーボードから入力する.


* 擬似モニタの操作 *
 ROMイメージがない場合は起動すると擬似モニタ実行される.

 L: ディスク上のモトローラSレコード形式のファイルをロードする.
 G: 入力したアドレスから実行する.
 決定はスペースキーである.


* 参考文献・webページ *
 次の文献とwebページを参考にしました. ありがとうございました. なお敬称は省略しています.

 ・MB-6885マニュアル, 日立製作所, 1981
 ・I/O別冊ベーシックマスター活用研究, 工学社, 1982
 ・m6800マイクロコンピュータマニュアル, モトローラ・セミコンダクターズ・ジャパン, エレクトリックダイジェスト, 1977
 ・マイクロコンピュータの基礎と6800, Ron Bishop著 モトローラ・セミコンダクターズ・ジャパン 日立製作所武蔵工場訳, CQ出版, 1980
 ・MinGW - Minimalist GNU for Windows, http://www.mingw.org/
 ・Simple Directmedia Layer, http://www.libsdl.org/
 ・Marat Fayzullin著 bero訳, コンピュータエミュレータの書き方, Console/Emulator Programming http://www.geocities.co.jp/Playtown/2004/
 ・Open Source Group Japan, http://www.opensource.jp/
 ・門真なむ, Little Limit, http://www.geocities.jp/littlimi/
 ・Enri, Enri's Home PAGE, http://www43.tok2.com/home/cmpslv/


* スペシャルサンクス *
 Hさん, ヒマジンガーＺさん, Karl-Heinz Verslさん, がんぴーさん 情報提供等ありがとうございました.


* ライセンス *

 bm2はBSDスタイルライセンスのもとで配布される.

 Copyright (c) 2008, 2015 maruhiro
 All rights reserved. 

 Redistribution and use in source and binary forms, 
 with or without modification, are permitted provided that 
 the following conditions are met: 

  1. Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer. 

  2. Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution. 

 THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
 THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 (日本語訳・参考)
 Copyright (c) 2008, 2015 maruhiro
 All rights reserved.

 ソースコード形式かバイナリ形式か、変更するかしないかを問わず、以下の条件を満たす
 場合に限り、再頒布および使用が許可されます。

  1. ソースコードを再頒布する場合、上記の著作権表示、本条件一覧、および下記免責条
     項を含めること。
  2. バイナリ形式で再頒布する場合、頒布物に付属のドキュメント等の資料に、上記の著
     作権表示、本条件一覧、および下記免責条項を含めること。

 本ソフトウェアは、著作権者およびコントリビューターによって「現状のまま」提供され
 ており、明示黙示を問わず、商業的な使用可能性、および特定の目的に対する適合性に関
 する暗黙の保証も含め、またそれに限定されない、いかなる保証もありません。著作権者
 もコントリビューターも、事由のいかんを問わず、損害発生の原因いかんを問わず、かつ
 責任の根拠が契約であるか厳格責任であるか（過失その他の）不法行為であるかを問わず
 、仮にそのような損害が発生する可能性を知らされていたとしても、本ソフトウェアの使
 用によって発生した（代替品または代用サービスの調達、使用の喪失、データの喪失、利
 益の喪失、業務の中断も含め、またそれに限定されない）直接損害、間接損害、偶発的な
 損害、特別損害、懲罰的損害、または結果損害について、一切責任を負わないものとしま
 す。
