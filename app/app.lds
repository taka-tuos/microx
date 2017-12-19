OUTPUT_FORMAT("binary");

SECTIONS
{
    .head 0x0 : {
        LONG(256 * 1024+((SIZEOF(.data)+0xfff)&0xfffff000))	/*  0 : stack+.data+heap の大きさ（4KBの倍数） */
        LONG(0x5863694D)									/*  4 : シグネチャ "MicX" */
        LONG(512 * 1024)									/*  8 : mmarea の大きさ（4KBの倍数） */
        LONG(512 * 1024)									/* 12 : スタック初期値＆.data転送先 */
        LONG(SIZEOF(.data))									/* 16 : .dataサイズ */
        LONG(LOADADDR(.data))								/* 20 : .dataの初期値列のファイル位置 */
        LONG(0xE9000000)									/* 24 : 0xE9000000 */
        LONG(MicroMain - 0x20)								/* 28 : エントリアドレス - 0x20 */
        LONG(LOADADDR(.data)+SIZEOF(.data))					/* 32 : heap領域（malloc領域）開始アドレス */
    }

    .text : { *(.text) }

    .data 512 * 1024 : AT ( ADDR(.text) + SIZEOF(.text) ) {
        *(.data)
        *(.rodata*)
        *(.bss)
    }

    /DISCARD/ : { *(.eh_frame) }

}
