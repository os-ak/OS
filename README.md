# OS
disk controller
21日目から

# 参考URL
* [０から作るOS開発](http://homepage3.nifty.com/kap/soft/identify.htm)
* [Wiki OS Project "ATA/ATAPI"](http://www.wiki.os-project.jp/?ATA%EF%BC%8FATAPI)
* [Wiki OS Dev "PCI IDE Controller"](http://wiki.osdev.org/PCI_IDE_Controller)
* [SEAGATE ATA reference](ftp://ftp.seagate.com/acrobat/reference/111-1c.pdf)
* [カーネルをブートするまで](http://softwaretechnique.jp/OS_Development/index.html)
* [How to write on hard disk with bios interrupt 13h](http://stackoverflow.com/questions/9057670/how-to-write-on-hard-disk-with-bios-interrupt-13h)
* [programming_ata_atapi](https://code.google.com/p/kenos/wiki/programming_ata_atapi)
* [HDDの話](http://caspar.hazymoon.jp/OpenBSD/misc/hdd.html)
* [How do I configure an ATA hard disk to start generating interrupts?](http://stackoverflow.com/questions/858657/how-do-i-configure-an-ata-hard-disk-to-start-generating-interrupts)
* [ATA/ATAPI Identify](http://homepage3.nifty.com/kap/soft/identify.htm)
* [参考github](https://github.com/masami256/miko)
# Read/Write Sectors(PIO)
## 注意事項
* 512byteの倍数分メモリが確保されている前提で動く。確保してない場合の動作は保証しない
* hariboteではintサイズ**4byte**,shortサイズ**2byte**
* device番号は **0** のみ想定
* ATAPIだった時の動作は保証しない
* Read/WriteでINTRQの発生タイミングは異なる [SEAGATE ATA reference](ftp://ftp.seagate.com/acrobat/reference/111-1c.pdf)
* INTRQの総数と読み書きしたセクタ数は同じ値 (MULTIMODEは別)

## Read/Write Sectors
セクタ数が増えるにつれ、INTRQ増加。
* ide_ata_read_sector_pio(device, LBA, 読み込むセクタの数, メモリの先頭アドレス)
* ide_ata_write_sector_pio(device, LBA, 書き込むセクタの数, メモリの先頭アドレス)

## Read/Write Sectors(Multiple)
Set Multipleコマンドで何セクタごとにINTRQを発生させるか設定したのち、  
読み書き実行。ただし、設定したセクタ数とちょうどのセクタを読み書きした場合は  
コマンド実行完了時の割り込みのみが発生する。(設定したセクタ数以下の読み書きでは特別な割り込みなし)
* int ide_set_multimode(int device, int sec_cnt)
* ide_ata_read_multiple_sector_pio(device, LBA, 読み込むセクタの数, メモリの先頭アドレス)
* ide_ata_write_multiple_sector_pio(device, LBA, 書き込むセクタの数, メモリの先頭アドレス)

# 動作確認
* writeに関しては、vdiファイルをhexdumpで確認できる。
* readとかのテストはbootpack.cに記述してる。
