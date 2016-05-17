========================================================================
    スタティック ライブラリ: UHInput プロジェクトの概要
========================================================================
Unlimited HandをWin32から扱う為のクラスです。UH側はexamplesのSerial_4Unity_4Processing(ver.0.0033)を利用します。
Win32のデスクトップアプリケーションで使えます。
取り敢えずの検証用に作った物なので、実装は適当です。

--------------
使い方
--------------
※あらかじめUHにはSerial_4Unity_4Processingを書き込んで置いて下さい。

#include "UHInput.h"

// 初期化
UHInput* unlimitedHand = new UHInput();
unlimitedHand->initialize("COM7");


// 更新/取得
while (mainloop) {
	unlimitedHand->update();
	printf("X%dY%dZ%d\n", unlimitedHand->getAngleX(), unlimitedHand->getAngleY(), unlimitedHand->getAngleZ());
}


// 終了処理
unlimitedHand->terminate();
delete unlimitedHand;
