/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Implementation of the CD-ROM drive with SCSI(-like)
 *		commands, for both ATAPI and SCSI usage.
 *
 * Version:	@(#)cdrom.h	1.0.9	2018/03/18
 *
 * Author:	Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2016,2017 Miran Grca.
 */
#ifndef EMU_CDROM_H
#define EMU_CDROM_H


#define CDROM_NUM		4

#define CD_STATUS_EMPTY		0
#define CD_STATUS_DATA_ONLY	1
#define CD_STATUS_PLAYING	2
#define CD_STATUS_PAUSED	3
#define CD_STATUS_STOPPED	4

#define CDROM_PHASE_IDLE            0
#define CDROM_PHASE_COMMAND         1
#define CDROM_PHASE_COMPLETE        2
#define CDROM_PHASE_DATA_IN         3
#define CDROM_PHASE_DATA_IN_DMA     4
#define CDROM_PHASE_DATA_OUT        5
#define CDROM_PHASE_DATA_OUT_DMA    6
#define CDROM_PHASE_ERROR           0x80

#define BUF_SIZE 32768

#define CDROM_IMAGE 200

#define IDE_TIME (5LL * 100LL * (1LL << TIMER_SHIFT))
#define CDROM_TIME (5LL * 100LL * (1LL << TIMER_SHIFT))


enum {
    CDROM_BUS_DISABLED = 0,
    CDROM_BUS_ATAPI_PIO_ONLY = 4,
    CDROM_BUS_ATAPI_PIO_AND_DMA,
    CDROM_BUS_SCSI,
    CDROM_BUS_USB = 8
};


typedef struct {
	int (*ready)(uint8_t id);
	int (*medium_changed)(uint8_t id);
	int (*media_type_id)(uint8_t id);

	void (*audio_callback)(uint8_t id, int16_t *output, int len);
	void (*audio_stop)(uint8_t id);
	int (*readtoc)(uint8_t id, uint8_t *b, uint8_t starttrack, int msf, int maxlen, int single);
	int (*readtoc_session)(uint8_t id, uint8_t *b, int msf, int maxlen);
	int (*readtoc_raw)(uint8_t id, uint8_t *b, int maxlen);
	uint8_t (*getcurrentsubchannel)(uint8_t id, uint8_t *b, int msf);
	int (*pass_through)(uint8_t id, uint8_t *in_cdb, uint8_t *b, uint32_t *len);
	int (*readsector_raw)(uint8_t id, uint8_t *buffer, int sector, int ismsf, int cdrom_sector_type, int cdrom_sector_flags, int *len);
	void (*playaudio)(uint8_t id, uint32_t pos, uint32_t len, int ismsf);
	void (*load)(uint8_t id);
	void (*eject)(uint8_t id);
	void (*pause)(uint8_t id);
	void (*resume)(uint8_t id);
	uint32_t (*size)(uint8_t id);
	int (*status)(uint8_t id);
	int (*is_track_audio)(uint8_t id, uint32_t pos, int ismsf);
	void (*stop)(uint8_t id);
	void (*exit)(uint8_t id);
} CDROM;

typedef struct {
	uint8_t previous_command;

	int toctimes;
	int media_status;

	int is_dma;

	int requested_blocks;		/* This will be set to something other than 1 when block reads are implemented. */

	uint64_t current_page_code;
	int current_page_len;

	int current_page_pos;

	int mode_select_phase;

	int total_length;
	int written_length;

	int do_page_save;

	uint8_t error;
	uint8_t features;
	uint16_t request_length;
	uint16_t max_transfer_len;
	uint8_t status;
	uint8_t phase;

	uint32_t sector_pos;
	uint32_t sector_len;

	uint32_t packet_len;
	int packet_status;

	uint8_t atapi_cdb[16];
	uint8_t current_cdb[16];

	uint32_t pos;

	int callback;

	int data_pos;
	uint32_t seek_diff;

	int cdb_len_setting;
	int cdb_len;

	int cd_status;
	int prev_status;

	int unit_attention;
	uint8_t sense[256];

	int request_pos;

	uint8_t *buffer;

	int times;

	uint32_t seek_pos;
	
	int total_read;

	int block_total;
	int all_blocks_total;
	
	int old_len;
	int block_descriptor_len;
	
	int init_length;

	int16_t cd_buffer[BUF_SIZE];

	uint8_t rcbuf[16];
	uint8_t sub_q_data_format[16];
	uint8_t sub_q_channel_data[256];
	int last_subchannel_pos;

	uint32_t cd_end;
	uint32_t cdrom_capacity;

	int cd_buflen;
	int cd_state;

	int handler_inited;
	int disc_changed;

	int cur_speed;
} cdrom_t;

typedef struct {
	int max_blocks_at_once;

	CDROM *handler;

	int host_drive;
	int prev_host_drive;

	unsigned int bus_type;		/* 0 = ATAPI, 1 = SCSI */
	uint8_t bus_mode;		/* Bit 0 = PIO suported;
					   Bit 1 = DMA supportd. */

	uint8_t ide_channel;

	unsigned int scsi_device_id;
	unsigned int scsi_device_lun;
	
	unsigned int sound_on;
	unsigned int atapi_dma;

	uint8_t speed;
} cdrom_drive_t;

typedef struct {
	int image_is_iso;
	wchar_t image_path[1024];
	wchar_t *prev_image_path;
	FILE* image;
} cdrom_image_t;

typedef struct {
	char ioctl_path[8];
	int actual_requested_blocks;
	int last_track_pos;
	int last_track_nr;
	int capacity_read;
} cdrom_ioctl_t;


extern cdrom_t		*cdrom[CDROM_NUM];
extern cdrom_drive_t	cdrom_drives[CDROM_NUM];
extern cdrom_image_t	cdrom_image[CDROM_NUM];
extern cdrom_ioctl_t	cdrom_ioctl[CDROM_NUM];
extern uint8_t		atapi_cdrom_drives[8];
extern uint8_t		scsi_cdrom_drives[16][8];

#define cdrom_sense_error cdrom[id]->sense[0]
#define cdrom_sense_key cdrom[id]->sense[2]
#define cdrom_asc cdrom[id]->sense[12]
#define cdrom_ascq cdrom[id]->sense[13]
#define cdrom_drive cdrom_drives[id].host_drive


#ifdef __cplusplus
extern "C" {
#endif

extern int	(*ide_bus_master_read)(int channel, uint8_t *data, int transfer_length);
extern int	(*ide_bus_master_write)(int channel, uint8_t *data, int transfer_length);
extern void	(*ide_bus_master_set_irq)(int channel);
extern void	ioctl_close(uint8_t id);

extern uint32_t	cdrom_mode_sense_get_channel(uint8_t id, int channel);
extern uint32_t	cdrom_mode_sense_get_volume(uint8_t id, int channel);
extern void	build_atapi_cdrom_map(void);
extern void	build_scsi_cdrom_map(void);
extern int	cdrom_CDROM_PHASE_to_scsi(uint8_t id);
extern int	cdrom_atapi_phase_to_scsi(uint8_t id);
extern void	cdrom_command(uint8_t id, uint8_t *cdb);
extern void	cdrom_phase_callback(uint8_t id);
extern uint32_t	cdrom_read(uint8_t channel, int length);
extern void	cdrom_write(uint8_t channel, uint32_t val, int length);

extern int	cdrom_lba_to_msf_accurate(int lba);
extern void	cdrom_destroy_drives(void);

extern void     cdrom_close(uint8_t id);
extern void	cdrom_reset(uint8_t id);
extern void	cdrom_set_signature(int id);
extern void	cdrom_request_sense_for_scsi(uint8_t id, uint8_t *buffer, uint8_t alloc_length);
extern void	cdrom_update_cdb(uint8_t *cdb, int lba_pos, int number_of_blocks);
extern void	cdrom_insert(uint8_t id);
extern void	cdrom_new_image(uint8_t id);

extern int	find_cdrom_for_scsi_id(uint8_t scsi_id, uint8_t scsi_lun);
extern int	cdrom_read_capacity(uint8_t id, uint8_t *cdb, uint8_t *buffer, uint32_t *len);

extern void	cdrom_global_init(void);
extern void	cdrom_global_reset(void);
extern void	cdrom_hard_reset(void);

#ifdef __cplusplus
}
#endif


#endif	/*EMU_CDROM_H*/
