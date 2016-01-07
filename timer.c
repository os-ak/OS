/* �^�C�}�֌W */

#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;
struct IDE_WAITER waiter;

#define TIMER_FLAGS_ALLOC		1	/* �m�ۂ������� */
#define TIMER_FLAGS_USING		2	/* �^�C�}�쓮�� */

void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; /* ���g�p */
	}
	t = timer_alloc(); /* ���������Ă��� */
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* ���Ԃ����� */
	timerctl.t0 = t; /* ���͔ԕ��������Ȃ��̂Ő擪�ł����� */
	timerctl.next = 0xffffffff; /* �ԕ��������Ȃ��̂Ŕԕ��̎��� */
	return;
}

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0; /* �������Ȃ����� */
}

void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /* ���g�p */
	return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/* �擪�ɓ������ꍇ */
		timerctl.t0 = timer;
		timer->next = t; /* ����t */
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/* �ǂ��ɓ������΂��������T�� */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			/* s��t�̊Ԃɓ������ꍇ */
			s->next = timer; /* s�̎���timer */
			timer->next = t; /* timer�̎���t */
			io_store_eflags(e);
			return;
		}
	}
}

void inthandler20(int *esp)
{
	struct TIMER *timer;
	char ts = 0;
	io_out8(PIC0_OCW2, 0x60);	/* IRQ-00���t������PIC�ɒʒm */
	timerctl.count++;
	if (timerctl.next > timerctl.count) {
		return;
	}
	timer = timerctl.t0; /* �Ƃ肠�����擪�̔Ԓn��timer�ɑ��� */
	for (;;) {
		/* timers�̃^�C�}�͑S�ē��쒆�̂��̂Ȃ̂ŁAflags���m�F���Ȃ� */
		if (timer->timeout > timerctl.count) {
			break;
		}
		/* �^�C���A�E�g */
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
		} else {
			ts = 1; /* task_timer���^�C���A�E�g���� */
		}
		timer = timer->next; /* ���̃^�C�}�̔Ԓn��timer�ɑ��� */
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	if (ts != 0) {
		task_switch();
	}
	return;
}


/* hdd */
void ide_init_waiter(){
	waiter.flag=-1;
	waiter.count=0;
	return;
}

void ide_set_wait_time(unsigned int timeout){
	for(;;){
		if(waiter.flag==-1)
			break;
	}
	waiter.timeout = timeout;
	waiter.flag=-1;
	return;
}
void ide_wait_10milsec(unsigned int timeout){
	ide_set_wait_time(timeout);
	for(;;){
		if(waiter.flag==-1)
			return;
	}
	return;
}