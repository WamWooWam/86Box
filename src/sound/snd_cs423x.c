/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Crystal CS423x (SBPro/WSS compatible sound chips) emulation.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>  
#include <86box/86box.h>
#include <86box/io.h>
#include <86box/timer.h>
#include <86box/pic.h>
#include <86box/dma.h>
#include <86box/device.h>
#include <86box/i2c.h>
#include <86box/isapnp.h>
#include <86box/sound.h>
#include <86box/midi.h>
#include <86box/snd_ad1848.h>
#include <86box/snd_opl.h>
#include <86box/snd_sb.h>


enum {
    CRYSTAL_CS4237B = 37
};


static const uint8_t cs4237b_eeprom[384] = {
    0x55, 0xbb, 0x00, 0x00, 0x00, 0x03, 0x80, 0x80, 0x0b, 0x20, 0x04, 0x08, 0x10, 0x80, 0x00, 0x00, 0x00, 0x48, 0x75, 0xb9, 0xfc, 0x10, 0x03, /* CS4237B stuff */

    0x0e, 0x63, 0x42, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, /* CSC4237, dummy checksum (filled in by isapnp_add_card) */
    0x0a, 0x10, 0x01, /* PnP version 1.0, vendor version 0.1 */
    0x82, 0x0e, 0x00, 'C', 'r', 'y', 's', 't', 'a', 'l', ' ', 'C', 'o', 'd', 'e' ,'c', 0x00, /* ANSI identifier */

    0x15, 0x0e, 0x63, 0x00, 0x00, 0x00, /* logical device CSC0000 */
	0x82, 0x07, 0x00, 'W', 'S', 'S', '/', 'S', 'B', 0x00, /* ANSI identifier */
	0x31, 0x00, /* start dependent functions, preferred */
		0x2a, 0x02, 0x28, /* DMA 1, type A, no count by word, count by byte, not bus master, 8-bit only */
		0x2a, 0x09, 0x28, /* DMA 0/3, type A, no count by word, count by byte, not bus master, 8-bit only */
		0x22, 0x20, 0x00, /* IRQ 5 */
		0x47, 0x01, 0x34, 0x05, 0x34, 0x05, 0x04, 0x04, /* I/O 0x534, decodes 16-bit, 4-byte alignment, 4 addresses */
		0x47, 0x01, 0x88, 0x03, 0x88, 0x03, 0x08, 0x04, /* I/O 0x388, decodes 16-bit, 8-byte alignment, 4 addresses */
		0x47, 0x01, 0x20, 0x02, 0x20, 0x02, 0x20, 0x10, /* I/O 0x220, decodes 16-bit, 32-byte alignment, 16 addresses */
	0x31, 0x01, /* start dependent functions, acceptable */
		0x2a, 0x0a, 0x28, /* DMA 1/3, type A, no count by word, count by byte, not bus master, 8-bit only */
		0x2a, 0x0b, 0x28, /* DMA 0/1/3, type A, no count by word, count by byte, not bus master, 8-bit only */
		0x22, 0xa0, 0x9a, /* IRQ 5/7/9/11/12/15 */
		0x47, 0x01, 0x34, 0x05, 0xfc, 0x0f, 0x04, 0x04, /* I/O 0x534-0xFFC, decodes 16-bit, 4-byte alignment, 4 addresses */
		0x47, 0x01, 0x88, 0x03, 0x88, 0x03, 0x08, 0x04, /* I/O 0x388, decodes 16-bit, 8-byte alignment, 4 addresses */
		0x47, 0x01, 0x20, 0x02, 0x60, 0x02, 0x20, 0x10, /* I/O 0x220-0x260, decodes 16-bit, 32-byte alignment, 16 addresses */
	0x31, 0x02, /* start dependent functions, sub-optimal */
		0x2a, 0x0b, 0x28, /* DMA 0/1/3, type A, no count by word, count by byte, not bus master, 8-bit only */
		0x22, 0xa0, 0x9a, /* IRQ 5/7/9/11/12/15 */
		0x47, 0x01, 0x34, 0x05, 0xfc, 0x0f, 0x04, 0x04, /* I/O 0x534-0xFFC, decodes 16-bit, 4-byte alignment, 4 addresses */
		0x47, 0x01, 0x88, 0x03, 0xf8, 0x03, 0x08, 0x04, /* I/O 0x388-0x3F8, decodes 16-bit, 8-byte alignment, 4 addresses */
		0x47, 0x01, 0x20, 0x02, 0x00, 0x03, 0x20, 0x10, /* I/O 0x220-0x300, decodes 16-bit, 32-byte alignment, 16 addresses */
	0x38, /* end dependent functions */
#if 0
    0x15, 0x0e, 0x63, 0x00, 0x01, 0x00, /* logical device CSC0001 */
	0x82, 0x05, 0x00, 'G', 'A', 'M', 'E', 0x00, /* ANSI identifier */
	0x31, 0x00, /* start dependent functions, preferred */
		0x47, 0x01, 0x00, 0x02, 0x00, 0x02, 0x08, 0x08, /* I/O 0x200, decodes 16-bit, 8-byte alignment, 8 addresses */
	0x31, 0x01, /* start dependent functions, acceptable */
		0x47, 0x01, 0x00, 0x02, 0x00, 0x02, 0x08, 0x08, /* I/O 0x208, decodes 16-bit, 8-byte alignment, 8 addresses */
	0x38, /* end dependent functions */
#endif
    0x15, 0x0e, 0x63, 0x00, 0x10, 0x00, /* logical device CSC0010 */
	0x82, 0x05, 0x00, 'C', 'T', 'R', 'L', 0x00, /* ANSI identifier */
	0x47, 0x01, 0x20, 0x01, 0xf8, 0x0f, 0x08, 0x08, /* I/O 0x120-0xFF8, decodes 16-bit, 8-byte alignment, 8 addresses */

    0x15, 0x0e, 0x63, 0x00, 0x03, 0x00, /* logical device CSC0003 */
	0x82, 0x04, 0x00, 'M', 'P', 'U', 0x00, /* ANSI identifier */
	0x31, 0x00, /* start dependent functions, preferred */
		0x22, 0x00, 0x02, /* IRQ 9 */
		0x47, 0x01, 0x30, 0x03, 0x30, 0x03, 0x08, 0x02, /* I/O 0x330, decodes 16-bit, 8-byte alignment, 2 addresses */
	0x31, 0x01, /* start dependent functions, acceptable */
		0x22, 0x00, 0x9a, /* IRQ 9/11/12/15 */
		0x47, 0x01, 0x30, 0x03, 0x60, 0x03, 0x08, 0x02, /* I/O 0x330-0x360, decodes 16-bit, 8-byte alignment, 2 addresses */
	0x31, 0x02, /* start dependent functions, sub-optimal */
		0x47, 0x01, 0x30, 0x03, 0xe0, 0x03, 0x08, 0x02, /* I/O 0x330-0x3E0, decodes 16-bit, 8-byte alignment, 2 addresses */
	0x38, /* end dependent functions */

    0x79, 0x00 /* end tag, dummy checksum (filled in by isapnp_add_card) */
};


typedef struct cs423x_t
{
    void	*pnp_card;
    ad1848_t	ad1848;
    sb_t	*sb;
    void	*i2c, *eeprom;

    uint16_t	wss_base, opl_base, sb_base, ctrl_base, ram_addr, eeprom_size;
    uint8_t	regs[8], indirect_regs[16], eeprom_data[2048], ram_dl;
} cs423x_t;


static uint8_t
cs423x_read(uint16_t port, void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;
    uint8_t reg = port & 7;
    uint8_t ret = dev->regs[reg];

    switch (reg) {
	case 1: /* EEPROM Interface */
		ret &= ~0x04;
		if ((dev->regs[1] & 0x04) && i2c_gpio_get_sda(dev->i2c))
			ret |= 0x04;
		break;

	case 4: /* Control Indirect Data Register */
		ret = dev->indirect_regs[dev->regs[3]];
		break;

	case 7: /* Global Status */
		ret = 0x00;
		if (dev->sb->mpu->state.irq_pending)
			ret |= 0x08;
		if (dev->ad1848.regs[10] & 2)
			ret |= 0x10;
		if (dev->sb->dsp.sb_irq8 || dev->sb->dsp.sb_irq16 || dev->sb->dsp.sb_irq401)
			ret |= 0x20;
    }

    return ret;
}


static void
cs423x_write(uint16_t port, uint8_t val, void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;
    uint8_t reg = port & 7;

    switch (reg) {
	case 1: /* EEPROM Interface */
		if (val & 0x04)
			i2c_gpio_set(dev->i2c, val & 0x01, val & 0x02);
		break;

	case 3: /* Control Indirect Access Register */
		val &= 0x0f;
		break;

	case 4: /* Control Indirect Data Register */
		switch (dev->regs[3] & 15) {
			case 0: /* WSS Master Control */
				if (val & 0x80)
					ad1848_init(&dev->ad1848, AD1848_TYPE_DEFAULT);
				val = 0x00;
				break;

			case 1: /* Version / Chip ID */
			case 7: /* Reserved */
			case 9 ... 15: /* unspecified */
				return;

			case 3: /* 3D Enable */
				val &= 0xe0;
				break;

			case 4: /* Consumer Serial Port Enable */
				val &= 0xf0;
				break;

			case 5: /* Lower Channel Status */
				val &= 0xfe;
				break;

			case 8: /* CS9236 Wavetable Control */
				val &= 0xf0;
				break;
		}
		dev->indirect_regs[dev->regs[3]] = val;
		break;

	case 5: /* Control/RAM Access */
		switch (dev->ram_dl) {
			case 0: /* commands */
				switch (val) {
					case 0x55: /* Disable PnP Key */
						isapnp_enable_card(dev->pnp_card, 0);
						break;

					/* TODO: Crystal's PnP bypass method? */

					case 0xaa: /* Download RAM */
						dev->ram_dl = 1;
						break;
				}
				break;

			case 1: /* low address byte */
				dev->ram_addr = val;
				dev->ram_dl++;
				break;

			case 2: /* high address byte */
				dev->ram_addr |= (val << 8);
				dev->ram_dl++;
				break;

			case 3: /* data */
				/* The only documented RAM region is 0x4000 (384 bytes in size), for
				   loading the chip's configuration and PnP ROM without an EEPROM. */
				if ((dev->ram_addr >= 0x4000) && (dev->ram_addr < 0x4180))
					dev->eeprom_data[dev->ram_addr - 0x3ffc] = val; /* skip first 4 bytes (EEPROM header) */
				dev->ram_addr++;
				break;
		}
		break;

	case 6: /* RAM Access End */
		if (!val)
			dev->ram_dl = 0;
		break;

	case 7: /* Global Status */
		return;
    }

    dev->regs[reg] = val;
}


static uint8_t
cs423x_ctxswitch_read(uint16_t addr, void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;
    uint8_t prev_context = dev->regs[7] & 0x80, switched = 0;

    /* Determine the active context (WSS or SBPro) through the address being read/written. */
    if ((prev_context == 0x80) && ((addr & 0xfff0) == dev->sb_base)) {
    	dev->regs[7] &= ~0x80;
    	switched = 1;
    } else if ((prev_context == 0x00) && ((addr & 0xfffc) == dev->wss_base)) {
    	dev->regs[7] |= 0x80;
    	switched = 1;
    }

    /* Fire the context switch interrupt if enabled. */
    if (switched && (dev->regs[0] & 0x20) && dev->ad1848.irq)
    	picint(1 << dev->ad1848.irq);

    return 0xff; /* don't interfere with the actual handlers */
}


static void
cs423x_ctxswitch_write(uint16_t addr, uint8_t val, void *priv)
{
    cs423x_ctxswitch_read(addr, priv);
}


static void
cs423x_get_buffer(int32_t *buffer, int len, void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;
    int c;

    /* Output audio from the WSS codec. SBPro and OPL3 are
       already handled by the Sound Blaster emulation. */
    ad1848_update(&dev->ad1848);

    for (c = 0; c < len * 2; c++) {
	buffer[c] += (dev->ad1848.buffer[c] / 2);
    }

    dev->ad1848.pos = 0;
}


static void
cs423x_pnp_config_changed(uint8_t ld, isapnp_device_config_t *config, void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;

    switch (ld) {
	case 0: /* WSS, OPL3 and SBPro */
		if (dev->wss_base) {
			io_removehandler(dev->wss_base, 4, ad1848_read, NULL, NULL, ad1848_write, NULL, NULL, &dev->ad1848);
			io_removehandler(dev->wss_base, 4, cs423x_ctxswitch_read, NULL, NULL, cs423x_ctxswitch_write, NULL, NULL, dev);
			dev->wss_base = 0;
		}

		if (dev->opl_base) {
			io_removehandler(dev->opl_base, 4, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
			dev->opl_base = 0;
		}

		if (dev->sb_base) {
			sb_dsp_setaddr(&dev->sb->dsp, 0);
			io_removehandler(dev->sb_base,     4, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
			io_removehandler(dev->sb_base + 8, 2, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
			io_removehandler(dev->sb_base + 4, 2, sb_ct1345_mixer_read, NULL, NULL, sb_ct1345_mixer_write, NULL, NULL, dev->sb);
			io_removehandler(dev->sb_base,    16, cs423x_ctxswitch_read, NULL, NULL, cs423x_ctxswitch_write, NULL, NULL, dev);
			dev->sb_base = 0;
		}

		ad1848_setirq(&dev->ad1848, 0);
		sb_dsp_setirq(&dev->sb->dsp, 0);

		ad1848_setdma(&dev->ad1848, 0);
		sb_dsp_setdma8(&dev->sb->dsp, 0);

		if (config->activate) {
			if (config->io[0].base != ISAPNP_IO_DISABLED) {
				dev->wss_base = config->io[0].base;
				io_sethandler(dev->wss_base, 4, ad1848_read, NULL, NULL, ad1848_write, NULL, NULL, &dev->ad1848);
				io_sethandler(dev->wss_base, 4, cs423x_ctxswitch_read, NULL, NULL, cs423x_ctxswitch_write, NULL, NULL, dev);
			}

			if (config->io[1].base != ISAPNP_IO_DISABLED) {
				dev->opl_base = config->io[1].base;
				io_sethandler(dev->opl_base, 4, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
			}

			if (config->io[2].base != ISAPNP_IO_DISABLED) {
				dev->sb_base = config->io[2].base;
				sb_dsp_setaddr(&dev->sb->dsp, dev->sb_base);
				io_sethandler(dev->sb_base,     4, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
				io_sethandler(dev->sb_base + 8, 2, opl3_read, NULL, NULL, opl3_write, NULL, NULL, &dev->sb->opl);
				io_sethandler(dev->sb_base + 4, 2, sb_ct1345_mixer_read, NULL, NULL, sb_ct1345_mixer_write, NULL, NULL, dev->sb);
				io_sethandler(dev->sb_base,    16, cs423x_ctxswitch_read, NULL, NULL, cs423x_ctxswitch_write, NULL, NULL, dev);
			}

			if (config->irq[0].irq != ISAPNP_IRQ_DISABLED) {
				ad1848_setirq(&dev->ad1848, config->irq[0].irq);
				sb_dsp_setirq(&dev->sb->dsp, config->irq[0].irq);
			}

			if (config->dma[0].dma != ISAPNP_DMA_DISABLED) {
				ad1848_setdma(&dev->ad1848, config->dma[0].dma);
				sb_dsp_setdma8(&dev->sb->dsp, config->dma[0].dma);
			}
		}

		break;

#if 0
	case 1: /* Game Port */
		gameport_remap(0);

		if (config->activate && (config->io[0].base != ISAPNP_IO_DISABLED))
			gameport_remap(config->io[0].base);

		break;
#endif

	case 1: /* Control Registers */
		if (dev->ctrl_base) {
			io_removehandler(dev->ctrl_base, 8, cs423x_read, NULL, NULL, cs423x_write, NULL, NULL, dev);
			dev->ctrl_base = 0;
		}

		if (config->activate && (config->io[0].base != ISAPNP_IO_DISABLED)) {
			dev->ctrl_base = config->io[0].base;
			io_sethandler(dev->ctrl_base, 8, cs423x_read, NULL, NULL, cs423x_write, NULL, NULL, dev);
		}

		break;

	case 2: /* MPU-401 */
		mpu401_change_addr(dev->sb->mpu, 0);
		mpu401_setirq(dev->sb->mpu, 0);

		if (config->activate) {
			if (config->io[0].base != ISAPNP_IO_DISABLED)
				mpu401_change_addr(dev->sb->mpu, config->io[0].base);

			if (config->irq[0].irq != ISAPNP_IRQ_DISABLED)
				mpu401_setirq(dev->sb->mpu, config->irq[0].irq);
		}

		break;
    }
}


static void *
cs423x_init(const device_t *info)
{
    cs423x_t *dev = malloc(sizeof(cs423x_t));
    memset(dev, 0, sizeof(cs423x_t));

    dev->indirect_regs[1] = 0x88;

    switch (info->local) {
	case CRYSTAL_CS4237B:
		dev->eeprom_size = sizeof(cs4237b_eeprom);
		memcpy(dev->eeprom_data, cs4237b_eeprom, dev->eeprom_size);
		break;
    }

    dev->sb = (sb_t *) device_add(&sb_pro_cs423x_device);

    ad1848_init(&dev->ad1848, AD1848_TYPE_DEFAULT);

    sound_add_handler(cs423x_get_buffer, dev);

    dev->i2c = i2c_gpio_init("nvr_cs423x");

    if (dev->eeprom_size) {
	dev->eeprom_data[2] = dev->eeprom_size >> 8;
	dev->eeprom_data[3] = dev->eeprom_size & 0xff;

	dev->eeprom = i2c_eeprom_init(i2c_gpio_get_bus(dev->i2c), 0x50, dev->eeprom_data, sizeof(dev->eeprom_data), 1);
    }

    dev->pnp_card = isapnp_add_card(&dev->eeprom_data[23], dev->eeprom_size - 23, cs423x_pnp_config_changed, NULL, NULL, NULL, dev);

    return dev;
}


static void
cs423x_close(void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;

    if (dev->eeprom)
	i2c_eeprom_close(dev->eeprom);

    i2c_gpio_close(dev->i2c);

    free(dev);
}


static void
cs423x_speed_changed(void *priv)
{
    cs423x_t *dev = (cs423x_t *) priv;

    ad1848_speed_changed(&dev->ad1848);
}


const device_t cs4237b_device =
{
    "Crystal CS4237B",
    DEVICE_ISA | DEVICE_AT,
    CRYSTAL_CS4237B,
    cs423x_init, cs423x_close, NULL,
    { NULL },
    cs423x_speed_changed,
    NULL,
    NULL
};
