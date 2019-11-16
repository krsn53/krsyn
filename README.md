# Krsyn 

Krsyn はC言語で書かれた、YM2612のような、FMシンセです。  
FM音源の勉強のために作ったもので、最低限の機能しか（すらも）実装していないという、お粗末な状況です。  
4オペレータのFM音源の他に、エセ三角波、エセ鋸波、エセ矩形波をならせます。


## Build

（エディタの）ビルドには、CMakeにOpenAL、Gtk+-3.0が必要です。
```
mkdir build
cd build
cmake ..
```

## License

Public domainとします。ただし、editor/内のものはGPLライセンスとします。

## TODO
 - Midiメッセージに対応させる。（ピッチベンドなど）
 - Lv2プラグインを実装する。
 - SSEやAVXなどでVectorizeする。
 - マニュアルを作成する。
