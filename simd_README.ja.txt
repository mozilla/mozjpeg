Independent JPEG Group's JPEG software release 6b
  with x86 SIMD extension for IJG JPEG library version 1.02
    == README ==
-----------------------------------------------------------

    ** Note **
The accompanying documents related to x86 SIMD extension are written in
Japanese. The English version of these documents is currently unavailable.
I apologize for this inconvenience to international programmers.

Most of the source code of the extension part is written in assembly
language. To compile the source, you need NASM (netwide assembler).
NASM is available from http://nasm.sourceforge.net/ or
http://sourceforge.net/project/showfiles.php?group_id=6208 .

At present, the x86 SIMD extension doesn't support 64-bit mode of
AMD64 (x86_64).

The x86 SIMD extension is an unofficial extension to the IJG JPEG
software. Please do not send any questions about this distribution
to the Independent JPEG Group.

For conditions of distribution and use, see the IJG's README file.
The same conditions apply to this SIMD-extended JPEG software.



■このソフトは

  JPEG のサポートライブラリとして広く使われている Independent JPEG Group's
  JPEG library (libjpeg ライブラリ) に、Intel x86 系 CPU の持つ SIMD 命令を
  利用したコード(ルーチン)を新たに追加し、高速化改造したものです。
  MMX や SSE などの SIMD 演算機能を装備しているプロセッサ上で動作させると、
  オリジナル版の libjpeg ライブラリと比較して 2〜3 倍程度の速度で動作します。
  また、SIMD 化に依らない高速化改造もいくつか施されており、SIMD 演算の使え
  ない旧型CPUにおいても、オリジナル版と比較して十数％程度高速に動作します。

  JPEG 圧縮／展開処理の高速化を目的としていますが、動作速度最優先ではなく、
  オリジナル版と同等以上の計算精度を持つことを最優先に考えたコードを採用
  しています。実際、DCT演算に浮動小数点DCTを使った場合、および、やや特殊な
  サンプリング比(h1v2)を持つJPEGファイルを展開する場合を除いては、
  オリジナル版と１ビットも違わない結果を出します。上記の２つの例外の場合も
  オリジナル版よりは高画質化(高精度化)されています。

  SIMD 対応化に際しては、可能な限り、オリジナル版の libjpeg ライブラリとの
  互換性が失われないように考慮されていますので、ほとんどの場合、オリジナル
  版をそのまま置き換えることが可能です。特に、共有ライブラリに関して言えば、
  一部の例外(cygwin の場合)を除き、それはオリジナル版とバイナリレベルでの
  上位互換性がありますので、そのままオリジナル版を置き換えることができます。

  SIMD 対応化されている部分は、以下のとおり：

  圧縮処理：
    色空間変換(RGB->YCbCr)  : MMX or SSE2
    ダウンサンプリング      : MMX or SSE2
    DCT順変換(高精度整数)   : MMX or SSE2
    DCT順変換(高速整数)     : MMX or SSE2
    DCT順変換(浮動小数)     : 3DNow! or SSE (整数演算部: MMX or SSE2)
    DCT係数量子化(整数)     : MMX or SSE2
    DCT係数量子化(浮動小数) : 3DNow! or SSE (整数演算部: MMX or SSE2)

  展開処理：
    色空間変換(YCbCr->RGB)  : MMX or SSE2
    アップサンプリング      : MMX or SSE2
    DCT逆変換(高精度整数)   : MMX or SSE2
    DCT逆変換(高速整数)     : MMX or SSE2
    DCT逆変換(浮動小数)     : 3DNow! or SSE (整数演算部: MMX or SSE2)
    DCT逆変換(縮小展開)     : MMX or SSE2

  注）SSE2 については、SIMD 整数演算のみを利用しています。SIMD 倍精度
      浮動小数点演算は利用していません。また、SSE3 は使用されていません。
      この JPEG ライブラリにおいては、SSE3 を使用しても動作速度向上の
      見込みがほとんどないためで、SSE3 をサポートする予定はありません。

  このほかに、アセンブリ言語版DCTルーチン(非SIMD; 順変換３種／逆変換４種)
  により、SIMD命令の使えない旧型CPUでも十数％程度の高速化が期待できます。
  さらに、展開処理でのハフマンデコードルーチンは、SIMD 化に依らない方法で
  高速化改造されています。


■対応しているプラットフォーム

  Intel x86 CPU に固有の機能を利用していますので、オリジナル版とは異なり、
  Intel x86 CPU およびその互換 CPU を採用しているシステムに限られます。
  PowerPC などの Intel x86 系以外のシステムには対応していません。
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  具体的には、80386 以降の Intel x86 CPU およびその互換 CPU を採用している
  ハードウェアで、かつ、32bitフラットアドレスモード(保護モード)を使用して
  いるプラットフォーム(OS)が対象です。これには、Win32 (Windows 9x系/NT系)
  や各種 PC-UNIX (linux や xBSD ファミリなど) などが該当します。なお、
  AMD64 (EM64T) の64bitモード環境には対応していません。ご注意ください。
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

■この SIMD 拡張版 IJG JPEG library 固有の制限

  オリジナル版の IJG JPEG library では、コンパイル時のオプションで、
  8bit精度JPEG と 12bit精度JPEG の両方に対応しますが、この SIMD 拡張版は
  8bit精度JPEG のみの対応で、12bit精度JPEG には対応しません。とはいえ、
  12bit精度JPEG は医療用などの特殊分野を除いて殆ど使われていないので、
  問題は少ないと思います。


■使い方

  マニュアルは、以下のファイルに分かれていますので、実際の使い方などに
  ついては、そちらを参照してください。

    simd_README.ja.txt   - このファイル
    simd_filelist.ja.txt - 収録ファイルのファイルリスト
    simd_install.ja.txt  - コンパイルの仕方
    simd_internal.ja.txt - SIMD 拡張部分の詳細
    simd_cdjpeg.ja.txt   - SIMD 版 cjpeg/djpeg に固有の機能の解説
    simd_changes.ja.txt  - SIMD 拡張部分の改版履歴


■使用条件・サポート

  この SIMD 拡張版 IJG JPEG software の使用条件については、オリジナル版の
  IJG JPEG software の使用条件が適用されます。詳しくは、同梱の README
  ファイル(英文)の LEGAL ISSUES の項を参照してください。

  上記の使用条件の内容の繰り返しになりますが、このソフトウェアは「現状の
  ままで」提供されているもので、商業的な使用可能性、および特定の目的に
  対する適合性なども含め、いかなる保証もありません。
  また、原作者(The Independent JPEG Group)も改造者(MIYASAKA Masaru)も、
  事由のいかんを問わず、本ソフトウェアの使用によって発生した如何なる損害に
  ついても、一切その責任を負わないものとします。

  この SIMD 拡張版 IJG JPEG software は、オリジナル開発元の IJG とは関係
  なく、独自に拡張を行なったものです。ですので、この SIMD 拡張版 IJG JPEG
  software に関する質問を、オリジナル開発元 (The Independent JPEG Group)
  に送らないでください。

  この SIMD 拡張版 IJG JPEG software に関しては、原則としてノーサポートと
  させていただきます。メールなどでご質問などをいただきましても、常に何らか
  の返答ができるわけではありませんので、ご承知おきください。
  特に、（オリジナルの英文マニュアルを含め）同梱のマニュアル類に回答が
  書いてある質問や、使用者のソフトウェア技術者としての技量不足・経験不足に
  関わる質問、質問の要領を得ない質問などについては、回答をいたしませんので、
  あしからずご了承ください。



           E-Mail Address : alkaid@coral.ocn.ne.jp (宮坂 賢/MIYASAKA Masaru)
[EOF]
