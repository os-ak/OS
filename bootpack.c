/* bootpack?���??��?��C?��?�� */

#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED		0xed
struct TASK *task_b[3];
struct SHEET *sht_win_b[3];
struct MEMMAN *memman;
struct SHTCTL *shtctl;
void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, cursor_x, cursor_c;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons,*buf_win_b;
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	struct TASK *task_a, *task_cons;
	struct TIMER *timer;
	/* for HDD TEST */
	int flg,flg2,buf[384];
	buf[0] = 2015;
	buf[1] = 12;
	buf[2] = 5;
	/* for HDD TEST finish*/

	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PIC?��̏�?��?��?��?��?��?��?��I?��?��?��?��?��?��?��̂�CPU?��̊�?��荞�݋֎~?��?��?��?��?��?�� */
	fifo32_init(&fifo, 128, fifobuf, 0);
	init_pit();
	init_keyboard(&fifo, 256);
	ide_init_waiter();
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); /* PIT?��?��PIC1?��ƃL?��[?��{?��[?��h?��?��?��?��?��?��(11111000) */
	// io_out8(PIC1_IMR, 0xef); /* ?��}?��E?��X?��?��?��?��?��?��(11101111) */
	io_out8(PIC1_IMR, 0xaf); /* ?��}?��E?��X?��?��HDD?��?��?��?��?��?��(10101111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	ide_initialize_device(0,2);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);

	/* sht_back */
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* ?��?��?��?��?��F?��Ȃ� */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* sht_cons */
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 357);
	sheet_setbuf(sht_cons, buf_cons, 256, 357, -1); /* ?��?��?��?��?��F?��Ȃ� */
	make_window8(buf_cons, 256, 357, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 320, COL8_000000);
	task_cons = task_alloc();
	task_cons->pid = 2;
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
	*((int *) (task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 2, 2); /* level=2, priority=2 */

	for (i = 0; i < 3; i++) {
		sht_win_b[i] = sheet_alloc(shtctl);
		buf_win_b = (unsigned char *) memman_alloc_4k(memman, 144 * 52);
		sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1); /* �����F�Ȃ� */
		sprintf(s, "run%d pid:%d", i,i+3);
		make_window8(buf_win_b, 144, 52, s, 0);
		task_b[i] = task_alloc();
		task_b[i]->pid = i+3;
		task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
		task_b[i]->tss.eip = (int) &task_b_main;
		task_b[i]->tss.es = 1 * 8;
		task_b[i]->tss.cs = 2 * 8;
		task_b[i]->tss.ss = 1 * 8;
		task_b[i]->tss.ds = 1 * 8;
		task_b[i]->tss.fs = 1 * 8;
		task_b[i]->tss.gs = 1 * 8;
		*((int *) (task_b[i]->tss.esp + 4)) = (int) sht_win_b[i];
		*((int *) (task_b[i]->tss.esp + 8)) = i;
	}

	/* sht_win */
	sht_win   = sheet_alloc(shtctl);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1); /* ?��?��?��?��?��F?��Ȃ� */
	make_window8(buf_win, 144, 52, "task_a", 1);
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* ?��?��?��ʒ�?��?��?��ɂȂ�?��悤?��ɍ�?��W?��v?��Z */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back,  0,  0);
	sheet_slide(sht_cons, 32,  4);
	sheet_slide(sht_win,  328+100, 16);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_cons,  1);
	sheet_updown(sht_win,   2);
	sheet_updown(sht_mouse, 3);

	sheet_slide(sht_win_b[0], 328+100, 116);
	sheet_slide(sht_win_b[1], 328+100, 116+100);
	sheet_slide(sht_win_b[2], 328+100, 116+200);

	/* ?��ŏ�?��ɃL?��[?��{?��[?��h?��?��?��ԂƂ̐H?��?��?��Ⴂ?��?��?��Ȃ�?��悤?��ɁA?��ݒ肵?��Ă�?��?��?��?��?��Ƃɂ�?��?�� */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/* HDD DEBUG ---------------------------------------------*/
	putfonts8_asc_sht(sht_back, 0, 370, COL8_FFFFFF, COL8_008484, "---------SIZE-------", 100);
	sprintf(s, "int: %d",sizeof(int));
	putfonts8_asc_sht(sht_back, 0, 390, COL8_FFFFFF, COL8_008484, s, 40);
	sprintf(s, "short int: %d",sizeof(short));
	putfonts8_asc_sht(sht_back, 0, 410, COL8_FFFFFF, COL8_008484, s, 40);
	sprintf(s, "long int: %d",sizeof(long int));
	putfonts8_asc_sht(sht_back, 0, 430, COL8_FFFFFF, COL8_008484, s, 40);
	sprintf(s, "char: %d",sizeof(char));
	putfonts8_asc_sht(sht_back, 0, 450, COL8_FFFFFF, COL8_008484, s, 40);
	putfonts8_asc_sht(sht_back, 0, 470, COL8_FFFFFF, COL8_008484, "------HDD READ & WRITE---------", 100);
	sprintf(s, "INITIALIZE TRSF MODE(): %d",flg);
	putfonts8_asc_sht(sht_back, 0, 490, COL8_FFFFFF, COL8_008484, s, 40);
	sprintf(s, "PIO_MODE: %d",ide_get_pio_mode(0));
	putfonts8_asc_sht(sht_back, 0, 510, COL8_FFFFFF, COL8_008484, s, 40);
	// flg2 = ide_ata_read_sector_pio(0,18,2,buf);
	// flg = ide_ata_write_multiple_sector_pio(0,20,3,buf);
	flg = ide_ata_write_sector_pio(0,20,3,buf);
	flg2 = ide_ata_read_sector_pio(0,20,3,buf);
	// flg = ide_ata_write_sector_pio(0,18,1,buf);
	// flg = ide_ata_write_sector_pio(0,19,1,buf);
	// flg2 = ide_ata_read_sector_pio(0,19,1,rbuf);
	sprintf(s, "WRTIE:%d READ:%d SEC_CNT:%d READ DATA: %d, %d, %d",0,0,(flg2<<16|flg),buf[0],buf[1],buf[2]);
	putfonts8_asc_sht(sht_back, 0, 530, COL8_FFFFFF, COL8_008484, s, 200);
	sprintf(s, "INTERRUPTS: %d",ide_get_is_interrupt());
	putfonts8_asc_sht(sht_back, 0, 550, COL8_FFFFFF, COL8_008484, s, 200);
	/* HDD DEBUG FINISH ----------------------------------------- */

	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* ?��L?��[?��{?��[?��h?��R?��?��?��g?��?��?��[?��?��?��ɑ�?��?��?��f?��[?��^?��?��?��?��?��?��?��΁A?��?��?��?�� */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			task_sleep(task_a,TASK_SLEEPING);
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511) { /* ?��L?��[?��{?��[?��h?��f?��[?��^ */
				if (i < 0x80 + 256) { /* ?��L?��[?��R?��[?��h?��?���?��R?��[?��h?��ɕϊ� */
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	/* ?��?��?��͕�?��?��?��?��?��A?��?��?��t?��@?��x?��b?��g */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* ?��啶?��?��?��?��?��?��?��?��?��?��?��ɕϊ� */
					}
				}
				if (s[0] != 0) { /* ?��ʏ핶?��?�� */
					if (key_to == 0) {	/* ?��^?��X?��NA?��?�� */
						if (cursor_x < 128) {
							/* ?��ꕶ?��?��?��\?��?��?��?��?��Ă�?��?��?��A?��J?��[?��\?��?��?��?��1?��i?��߂� */
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					} else {	/* ?��R?��?��?��\?��[?��?��?��?�� */
						fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				if (i == 256 + 0x0e) {	/* ?��o?��b?��N?��X?��y?��[?��X */
					if (key_to == 0) {	/* ?��^?��X?��NA?��?�� */
						if (cursor_x > 8) {
							/* ?��J?��[?��\?��?��?��?��?��X?��y?��[?��X?��ŏ�?��?��?��Ă�?��?��?��A?��J?��[?��\?��?��?��?��1?��߂� */
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {	/* ?��R?��?��?��\?��[?��?��?��?�� */
						fifo32_put(&task_cons->fifo, 8 + 256);
					}
				}
				if (i == 256 + 0x1c) {	/* Enter */
					if (key_to != 0) {	/* ?��R?��?��?��\?��[?��?��?��?�� */
						fifo32_put(&task_cons->fifo, 10 + 256);
					}
				}
				if (i == 256 + 0x0f) {	/* Tab */
					if (key_to == 0) {
						key_to = 1;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1; /* ?��J?��[?��\?��?��?��?��?��?��?��?�� */
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo, 2); /* ?��R?��?��?��\?��[?��?��?��̃J?��[?��\?��?��ON */
					} else {
						key_to = 0;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = COL8_000000; /* ?��J?��[?��\?��?��?��?��?��o?��?�� */
						fifo32_put(&task_cons->fifo, 3); /* ?��R?��?��?��\?��[?��?��?��̃J?��[?��\?��?��OFF */
					}
					sheet_refresh(sht_win,  0, 0, sht_win->bxsize,  21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				if (i == 256 + 0x2a) {	/* ?��?��?��V?��t?��g ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* ?��E?��V?��t?��g ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* ?��?��?��V?��t?��g OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* ?��E?��V?��t?��g OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0xfa) {	/* ?��L?��[?��{?��[?��h?��?��?��f?��[?��^?��?���?��Ɏ󂯎�?��?��?��?�� */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* ?��L?��[?��{?��[?��h?��?��?��f?��[?��^?��?���?��Ɏ󂯎�?��?��?��Ȃ�?��?��?��?�� */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				/* ?��J?��[?��\?��?��?��̍ĕ\?��?�� */
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= i && i <= 767) { /* ?��}?��E?��X?��f?��[?��^ */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* ?��}?��E?��X?��J?��[?��\?��?��?��̈ړ� */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sheet_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0) {
						/* ?��?��?��{?��^?��?��?��?��?��?��?��?��?��Ă�?��?��?��?��?��Asht_win?��?���?��?�� */
						sheet_slide(sht_win, mx - 80, my - 8);
					}
				}
			} else if (i <= 1) { /* ?��J?��[?��\?��?��?��p?��^?��C?��} */
				if (i != 0) {
					timer_init(timer, &fifo, 0); /* ?��?��?��?��0?��?�� */
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1); /* ?��?��?��?��1?��?�� */
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50);
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}
void task_b_make(){
/*	struct TASK *task_b;
	struct SEET *sht_win_b = sheet_alloc(shtctl);
	unsigned char buf_win_b = (unsigned char *) memman_alloc_4k(memman, 144 * 52);
	sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1);
	sprintf(s, "run%d pid:%d", 4,4);
	make_window8(buf_win_b, 144, 52, s, 0);
	task_b = task_alloc();
	task_b->pid = 4;
	task_b->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_b->tss.eip = (int) &task_b_main;
	task_b->tss.es = 1 * 8;
	task_b->tss.cs = 2 * 8;
	task_b->tss.ss = 1 * 8;
	task_b->tss.ds = 1 * 8;
	task_b->tss.fs = 1 * 8;
	task_b->tss.gs = 1 * 8;
	*((int *) (task_b->tss.esp + 4)) = (int) sht_win_b;
	*((int *) (task_b->tss.esp + 8))=i;
	sheet_slide(sht_win_b, 488+100, 116+150);
	sheet_updown(sht_win_b, 3);
	task_run(task_b,2,2);*/
	return ;
}

void start_hdd(int flg, int in1,int in2,void* in3){
	struct TASK *task_hdd;
	task_hdd = task_alloc();
	task_hdd->pid = 50;
	task_hdd->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024-20;
	task_hdd->tss.eip = (int) &task_hdd_main;
	task_hdd->tss.es = 1 * 8;
	task_hdd->tss.cs = 2 * 8;
	task_hdd->tss.ss = 1 * 8;
	task_hdd->tss.ds = 1 * 8;
	task_hdd->tss.fs = 1 * 8;
	task_hdd->tss.gs = 1 * 8;
	*((int *) (task_hdd->tss.esp + 4))=flg;
	*((int *) (task_hdd->tss.esp + 8))=in1;
	*((int *) (task_hdd->tss.esp + 12))=in2;
	*((int *) (task_hdd->tss.esp + 16))=(int)in3;
	task_run(task_hdd, 2, 2);
	return;
}

void task_hdd_main(int flg, int in1,int in2,void* in3)
{
	struct FIFO32 fifo;
	struct TIMER *timer;
	int fifobuf[128],i;
	if(flg==0){
		ide_read(in1,in2,in3);
	}else if(flg==1){
		ide_write(in1,in2,in3);
	}
	fifo32_init(&fifo, 128, fifobuf, 0);
	timer = timer_alloc();
	timer_init(timer, &fifo, 55500);
	timer_settime(timer, 200);
	for (;;) {
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (i == 55500) {
				stop_hdd();
				task_exit();
			}
		}
	}
}

void task_b_main(struct SHEET *sht_win_b,int num)
{
	struct FIFO32 fifo;
	struct TIMER *timer_1s,*timer_2s;
	int i, fifobuf[128], cnt = 5;
	int in3[128];
	char s[12];
	int in1,in2;
	int flg;
	int call = 0;
	fifo32_init(&fifo, 128, fifobuf, 0);
	timer_1s = timer_alloc();
	timer_init(timer_1s, &fifo, 10000);
	timer_settime(timer_1s, 100);
	timer_2s = timer_alloc();
	timer_init(timer_2s, &fifo, 11100);
	timer_settime(timer_2s, 500);

	for (;;) {
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (i == 10000) {
				cnt--;
				if(call==0){
					sprintf(s, "PRE WRITE %d", cnt);
				}else if(call==1){
					sprintf(s, "PRE READ  %d", cnt);
				}else{
					sprintf(s, "ALL END  %d", cnt);
				}
				putfonts8_asc_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, 11);
				if(cnt==-10){
					sheet_updown(sht_win_b, -1);
					break;
				}
				timer_settime(timer_1s, 100);
			}else if(i==11100){
				if(call==0){
					//�������݃e�X�g
					call++;
					flg=1;
					in1=20+(i*10);
					in2=3;
					in3[0]=123+(i*100);
					in3[1]=456+(i*100);
					in3[2]=789+(i*100);
					test_hdd(flg,in1,in2,in3);
					cnt=6;
					timer_settime(timer_2s, 400);
				}else{
					//�ǂݍ��݃e�X�g
					call++;
					flg=0;
					in1=20+(i*10);
					in2=3;
					in3[0]=123+(i*100);
					in3[1]=456+(i*100);
					in3[2]=789+(i*100);
					/*test_hdd(flg,in1,in2,in3);
					sprintf(s, , );
					putfonts8_asc_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, 11);*/
				}

			}
		}
	}
	task_exit();
}
