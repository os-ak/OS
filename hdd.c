#include "bootpack.h"

int dev_type[2];
int active_device;
int max_logical_sector_num;
short dev_identify_inf[2][256];
struct DEV_INF dev_inf;
int is_interrupt;

void inthandler2E(int *esp){
	io_out8(PIC1_OCW2, 0x66);	/* IRQ-12受付完�?をPIC1に通知 */
	io_out8(PIC0_OCW2, 0x62);	/* IRQ-02受付完�?をPIC0に通知 */
	is_interrupt++;
	return;
}

void ide_initialize_device(int device,int sec_cnt){
	ide_initialize_common_check();
	ide_initialize_transfer_mode(device);
	ide_set_multimode(device,sec_cnt);
}

int ide_get_is_interrupt(){
	return is_interrupt;
}

/*
Wait until Busy and DRQ bit are cleared.
Busy:8bit
DRQ:4bit
@return 0 if wait is success.
 */
int ide_wait_BSYclr_DRQclr(){
	int st,cnt=0;
	do{
		st = io_in8(ATA_ASR);
		if((st & ATA_BIT_BSY) == 0 && (st & ATA_BIT_DRQ) == 0)
			return 0;
		cnt++;
	}while(cnt<ATA_WAIT_LIMIT);

	return -1;
}

/*
Wait until BSY bit is cleared.
 */
int ide_wait_BSYclr(){
	int st,cnt=0;
	do{
		st = io_in8(ATA_ASR);
		if((st&ATA_BIT_BSY) == 0)
			return 0;
		cnt++;
	}while(cnt < ATA_WAIT_LIMIT);

	return -1;
}

/*
Wait until DRDY bit is 1.
 */
int ide_wait_DRDY(){
	int st;//,cnt=0;
	do{
		st = io_in8(ATA_ASR);
		if((st&ATA_BIT_DRDY) != 0)
			return 0;
		//cnt++;
	}while(1);

	return -1;
}

/*
Set DEV bit to device num on DHR
DEV bit:5bit
@param device 0 or 1
*/
void ide_set_device_number(int device){
	io_out8(ATA_DHR,device<<4);
	ide_wait_10milsec(1);/*wait 400ns*/
	return;
}

/*
Device Selection Protocol.
This protocol must be called before command protocols.
@param device 0 or 1
@return 0 if process is success. -1 if wait_BSYclr_DRQclr() fails.
 */
int ide_device_selection_protocol(int device){
	int st;
	st = ide_wait_BSYclr_DRQclr();
	if(st != 0) return st;
	ide_set_device_number(device);
	st = ide_wait_BSYclr_DRQclr();
	return st;
}

/*
First of all, this func must be called.
 */
int ide_initialize_common_check(){
	int i,st;
	int d0,cl0,ch0,d1,cl1,ch1;
	io_out8(ATA_DCR,0x06);/*software reset*/
	ide_wait_10milsec(1);/*wait 5ms*/
	io_out8(ATA_DCR,0x02);/*unlock reset, forbid interrupts*/
	ide_wait_10milsec(1);/*wait 5ms*/
	is_interrupt = 0;
	//active_device = 10000;
	dev_type[0] = DEV_UNKNOWN;
	dev_type[1] = DEV_UNKNOWN;

	/*device0 check*/
	for(i=0;i<ATA_RETRY_LIMIT;i++){
		cl0 = ch0 = 0xff;/*flag of not connected*/
		ide_set_device_number(0);
		// write_command(0x08);
		ide_wait_10milsec(1);
		//	break;
		d0 = io_in8(ATA_STR);
		if(d0 == 0xff) break;/*if read-back value is 0xff, a device is not connected*/
		st = ide_wait_BSYclr();
		if(st == -1) break;/*a device is not connected*/
		d0 = io_in8(ATA_ERR);
		if((d0&0x7f)!=1) break;/*device 0 is bad.*/
		d0 = io_in8(ATA_DHR);
		if((d0&ATA_BIT_DEV)==0){/*Confirm whether DEV bit is 0*/
			ch0 = io_in8(ATA_CHR);
			cl0 = io_in8(ATA_CLR);
		}
	}

	/*device1 check*/
	// for(i=0;i<RETRY_MAX;i++){
	// 	set_device_number(1);
	// 	d1 = io_in8(STATUS_REG);
	// 	if(d1 == 0xff) break;/*リードバ�?クした値�?0xff�?と未接�?*/
	// 	wait_bsyclr();
	// 	d1 = io_in8(ERR_REG);
	// 	if(d1!=1) break;/*不良*/
	// 	d1 = io_in8(DEVICE_HEAD_REG);
	// 	if((d1&0x10)!=0){/*DEVビットに0が立って�?るか確�?*/
	// 		c_high1 = io_in8(CYLINDER_HIGH);
	// 		c_low1 = io_in8(CYLINDER_LOW);
	// 		break;
	// 	}
	// }
	//ide_initialize_check_by_type(0,c_low0,c_high0,buf);
	ide_initialize_check_by_type(0,cl0,ch0);
	return dev_identify_inf[0][12];
}

/*
Get type from Cylinder low and high bits.
@param cl:Cylinder low
@param ch:Cylinder high
@return definition
 */
int ide_get_dev_type(int cl,int ch){
	if((ch == 0x00) && (cl == 0x00)){
		return DEV_ATA;
	}else if((ch== 0xEB) && (cl == 0x14)){
		return DEV_ATAPI;
	}else{
		return DEV_UNKNOWN;
	}
}

/*
Read from DTR for length times
@param length:maybe BLOCK_LENGTH
@param buf:buffer for reading data
 */
void ide_ata_read_data(int length,void* const buf){
	short* tmp_buf = (short*)buf;
	int i,j;
	for(i=0;i<length;i++){
		*tmp_buf = io_in16(ATA_DTR);
		tmp_buf++;
	}
}

/*
PIO DATA IN PROTOCOL.
@param device:0 or 1
@param ata:reg config structure
@param sec_cnt:reading bytes are "BLOCK_LENGTH*2*sec_cnt byte"
@param buf:buffer for read
@return
	 0:success
	-1:device selection protocol failed
	-2:waiting BSY failed
	-3:error in executing command -> the device is not connected
	-4:data not existing(cannot read or prepare data)
	-5:error in finishing command with error
 */
int ide_pio_datain_protocol(int device,struct ATA_CMD_STR* ata,int sec_cnt,void* const buf){
	int i,j,st;
	short* tmp_buf = (short*)buf;
	if(ide_device_selection_protocol(device) != 0) return -1;
	ide_protocol_exec_command(ata);

	io_in8(ATA_ASR);
	for(i=0;i<sec_cnt;i++){
		if(ide_wait_BSYclr() == -1)
			return -2;
		st = io_in8(ATA_STR);
		if((st&ATA_BIT_ERR) != 0)
			return -3;

		if((st&ATA_BIT_DRQ) == 0){
			return -4;
		}else{
			ide_ata_read_data(BLOCK_LENGTH,tmp_buf);
		}
		tmp_buf += BLOCK_LENGTH;
	}
	io_in8(ATA_ASR);
	st = io_in8(ATA_STR);
	if((st&ATA_BIT_ERR) != 0)
		return -5;
	else
		return 0;
}

/*
NON DATA CMD PROTOCOL.
@param device: 0 or 1
@param ata:reg config structure
@return
	 0:success
	-1:device selection protocol failed
	-2:waiting BSY failed
	-3:error in executing command -> the device is not connected
 */
int ide_nondata_protocol(int device,struct ATA_CMD_STR* ata){
	int st;
	if(ide_device_selection_protocol(device) != 0) return -1;
	ide_protocol_exec_command(ata);

	if(ide_wait_BSYclr() == -1)
		return -2;
	st = io_in8(ATA_STR);
	if((st & ATA_BIT_ERR) != 0)
		return -3;

	return 0;
}

/*
Execute Command accordin to ATA_CMD_STR.
This func must be called by *_protocol.
@param ata:command info
 */
void ide_protocol_exec_command(struct ATA_CMD_STR* ata){
	// io_out8(ATA_DCR,0x02);
	io_out8(ATA_DCR,0x02);
	io_out8(ATA_FTR,ata->feature);
	io_out8(ATA_SCR,ata->sec_cnt);
	io_out8(ATA_SNR,ata->sec_num);
	io_out8(ATA_CLR,ata->cl);
	io_out8(ATA_CHR,ata->ch);
	io_out8(ATA_DHR,ata->dev_head);
	if(ata->DRDY_chk == 1) ide_wait_DRDY();
	io_out8(ATA_CMDR,ata->command);
	ide_wait_10milsec(1);/* wait 400ns. */
}

/*
Identify device!
@param device: 0 or 1
@param buf: buffer to read data.
@return -5..0: see ide_pio_datain_protocol
 */
int ide_identify_device(int device,void* const buf){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0;
	ata.sec_cnt =0;
	ata.sec_num =0;
	ata.ch=0;
	ata.cl=0;
	ata.dev_head = (device<<4);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_IDENTIFY;

	st = ide_pio_datain_protocol(device,&ata,1,buf);

	return st;
}

/*
Initialize devices with vary processes for each type(ATA or ATAPI)
Now, ONLY ATA!
@param device:0 or 1
@param cl:cylinder low
@param ch:cylinder high
@return:0 or -1
 */
int ide_initialize_check_by_type(int device,int cl,int ch){
	int type,flg,st,i;
	int a,b;
	type = ide_get_dev_type(cl,ch);
	if(type != DEV_ATA)
		return -1;

	st = ide_wait_BSYclr();
	if(st == -1) return -1;

	for(i=0;i<ATA_RETRY_LIMIT;i++){
		a = ide_identify_device(device,(void*)dev_identify_inf[0]);
		ide_wait_10milsec(1);
		b = ide_identify_device(device,(void*)dev_identify_inf[0]);
		if(a==b){
			if(a==0){
				dev_type[0] = DEV_ATA;
				break;
			}else if(a == -3||a==-5){
				dev_type[0] = DEV_NIL;
				break;
			}
		}
		ide_wait_10milsec(1);/*wait 5ms*/
	}
	return 0;
}

void ide_finish_sector_rw(){
	io_in8(ATA_ASR);
	io_in8(ATA_STR);
}

/*
Set Features!
@param device:0 or 1
@param subcmd:Subcommand of 'SET FEATURES' command. It's located in Features REG.
@param sec_cnt~:depending on subcommands.
@return -3..0:see ide_nondata_protocol
 */
int ide_set_features(int device,int subcmd,int sec_cnt,int sec_num,int cl,int ch){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =subcmd;
	ata.sec_cnt =sec_cnt;
	ata.sec_num =sec_num;
	ata.ch=cl;
	ata.cl=ch;
	ata.dev_head = (device<<4);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_SET_FEATURES;

	st = ide_nondata_protocol(device,&ata);

	return st;
}

/*
Get PIO mode 0,3,4.
No method to know wheather PIO support 1 or 2.
@param device:0 or 1
@return -1,0,3,4: -1 caused by device_selection error.
 */
int ide_get_pio_mode(int device){
	if(dev_type[device] != DEV_ATA) return -1;
	if((dev_identify_inf[device][53] & 0x02) != 0){
		if(dev_identify_inf[device][64] & 0x02) return 4;
		if(dev_identify_inf[device][64] & 0x01) return 3;
	}
	return 0;
}

/*
Set Transfer Mode.
Set PIO mode version to highest supported version.
@param device:0 or 1
@return:-3..0
 */
int ide_initialize_transfer_mode(int device){
	int st;
	int pio_mode = ide_get_pio_mode(device);
	if(pio_mode == -1) return -1;

	//st = ide_initialize_dev_params(0,dev_identify_inf[device][3]-1,dev_identify_inf[device][6]);
	//if(st != 0) return st;
	if(dev_identify_inf[device][49] & 0x800){/* supporting iordy? if mode more than 3,must support*/
		st = ide_set_features(device,ATA_SUBCMD_SET_TRANFER,0x08|(pio_mode&0x07),0,0,0);
	}else{
		st = ide_set_features(device,ATA_SUBCMD_SET_TRANFER,0,0,0,0);
	}

	return st;
}


int ide_ata_read_sector_pio(int device,int lba,int sec_cnt, void* const buf){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0;
	ata.sec_cnt =sec_cnt;
	ata.sec_num =lba & 0xff;
	ata.cl=(lba >> 8) & 0xff;
	ata.ch=(lba >> 16) & 0xff;
	ata.dev_head = ATA_DEFAULT_DHR|(device<<4)|((lba>>24)&0x0f);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_READ_SECTOR;
	//return dev_identify_inf[device][88];
	st = ide_pio_datain_protocol(device,&ata,sec_cnt,buf);

	return st;
}


/*
PIO DATA OUT PROTOCOL.
@param device:0 or 1
@param ata:reg config structure
@param sec_cnt:reading bytes are "BLOCK_LENGTH*2*sec_cnt byte"
@param buf:buffer for write
@return
	 0:success
	-1:device selection protocol failed
	-2:waiting BSY failed
	-3:error in executing command -> the device is not connected
	-4:data not existing(cannot write or prepare data)
	-5:error in finishing command with error
 */
int ide_pio_dataout_protocol(int device,struct ATA_CMD_STR* ata,int sec_cnt,void* const buf){
	int i,j,st;
	short* tmp_buf = (short*)buf;
	if(ide_device_selection_protocol(device) != 0) return -1;
	ide_protocol_exec_command(ata);

	io_in8(ATA_ASR);
	for(i=0;i<sec_cnt;i++){
		if(ide_wait_BSYclr() == -1)
			return -2;
		st = io_in8(ATA_STR);
		if((st&ATA_BIT_ERR) != 0)
			return -3;

		if((st&ATA_BIT_DRQ) == 0){
			return -4;
		}else{
			ide_ata_write_data(BLOCK_LENGTH,tmp_buf);
		}
		tmp_buf += BLOCK_LENGTH;
	}
	ide_wait_BSYclr_DRQclr();
	io_in8(ATA_ASR);
	st = io_in8(ATA_STR);

	if((st&ATA_BIT_ERR) != 0)
		return -5;
	else
		return 0;
}

/*
Write to DTR for length times
@param length:maybe BLOCK_LENGTH
@param buf:buffer for reading data
 */
void ide_ata_write_data(int length,void* const buf){
	short* tmp_buf = (short*)buf;
	int i,j;
	for(i=0;i<length;i++){
		io_out16(ATA_DTR,*tmp_buf);
		tmp_buf++;
	}
}

/*

 */
int ide_ata_write_sector_pio(int device,int lba,int sec_cnt, void* const buf){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0;
	ata.sec_cnt =sec_cnt;
	ata.sec_num =lba & 0xff;
	ata.cl=(lba >> 8) & 0xff;
	ata.ch=(lba >> 16) & 0xff;
	ata.dev_head = ATA_DEFAULT_DHR|(device<<4)|((lba>>24)&0x0f);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_WRITE_SECTOR;

	st = ide_pio_dataout_protocol(device,&ata,sec_cnt,buf);

	return st;
}

int ide_initialize_dev_params(int device,int head,int sector){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0xff;
	ata.sec_cnt =sector;
	ata.sec_num =0;
	ata.ch=0;
	ata.cl=0;
	ata.dev_head = ATA_DEFAULT_DHR|(device<<4)|(head&0x0f);
	ata.DRDY_chk =0;
	ata.command = 0x91;

	st = ide_nondata_protocol(device,&ata);

	return st;
}


int ide_set_multimode(int device,int sec_cnt){
		struct ATA_CMD_STR ata;
		int st;
		ata.feature = 0;
		ata.sec_cnt = sec_cnt;
		ata.sec_num = 0;
		ata.ch=0;
		ata.cl=0;
		ata.dev_head = (device<<4);
		ata.DRDY_chk =1;
		ata.command = ATA_CMD_SET_MULTIMODE;

		st = ide_nondata_protocol(device,&ata);

		return st;
}

int ide_ata_write_multiple_sector_pio(int device,int lba,int sec_cnt, void* const buf){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0;
	ata.sec_cnt =sec_cnt;
	ata.sec_num =lba & 0xff;
	ata.cl=(lba >> 8) & 0xff;
	ata.ch=(lba >> 16) & 0xff;
	ata.dev_head = ATA_DEFAULT_DHR|(device<<4)|((lba>>24)&0x0f);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_WRITE_MULTIPLE_SECTOR;

	st = ide_pio_dataout_protocol(device,&ata,sec_cnt,buf);

	return st;
}

int ide_ata_read_multiple_stor_pio(int device,int lba,int sec_cnt,void* const buf){
	struct ATA_CMD_STR ata;
	int st;
	ata.feature =0;
	ata.sec_cnt =sec_cnt;
	ata.sec_num =lba & 0xff;
	ata.cl=(lba >> 8) & 0xff;
	ata.ch=(lba >> 16) & 0xff;
	ata.dev_head = ATA_DEFAULT_DHR|(device<<4)|((lba>>24)&0x0f);
	ata.DRDY_chk =1;
	ata.command = ATA_CMD_READ_MULTIPLE_SECTOR;
	//return dev_identify_inf[device][88];
	st = ide_pio_datain_protocol(device,&ata,sec_cnt,buf);

	return st;
}

int ide_get_dev_identify_inf(int num){
	return dev_identify_inf[0][num];
}

/**
 * num/2/1024/1024 -> GB size
 * @return [description]
 */
int ide_get_maximum_logical_sector(){
	int num = (dev_identify_inf[0][61] << 16)|dev_identify_inf[0][60];
	return num;
}

/**
 * [ide_read description]
 * wrapper
 * @param  lba     LBA
 * @param  sec_cnt number of sectors read
 * @param  buf     memory to store
 * @return         0 or error code
 */
int ide_read(int lba, int sec_cnt, void* const buf){
	int st = ide_ata_read_sector_pio(0,lba,sec_cnt,buf);
	return st;
}

/**
 * [ide_write description]
 * wrapper
 * @param  lba     LBA
 * @param  sec_cnt number of sectors read
 * @param  buf     memory to store
 * @return         0 or error code
 */
int ide_write(int lba, int sec_cnt, void* const buf){
	int st = ide_ata_write_sector_pio(0,lba,sec_cnt,buf);
	return st;
}
