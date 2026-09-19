
#include <linux/string.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <scsi/scsi.h>
#include "mi_memory_sysfs.h"
#include "mem_interface.h"

enum field_width {
	BYTE	= 1,
	WORD	= 2,
	DWORD   = 4,
};

struct desc_field_offset {
	char *name;
	int offset;
	enum field_width width_byte;
};

u16 get_ufs_id(void)
{
	u16 ufs_id = 0;

	ufs_read_desc_param(QUERY_DESC_IDN_DEVICE, 0, DEVICE_DESC_PARAM_MANF_ID, &ufs_id, 2);

	return ufs_id;
}


static ssize_t dump_health_desc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 value = 0;
	int count = 0, i = 0;

	struct desc_field_offset health_desc_field_name[] = {
		{"bLength", HEALTH_DESC_PARAM_LEN, BYTE},
		{"bDescriptorType", HEALTH_DESC_PARAM_TYPE, BYTE},
		{"bPreEOLInfo",	HEALTH_DESC_PARAM_EOL_INFO, BYTE},
		{"bDeviceLifeTimeEstA", HEALTH_DESC_PARAM_LIFE_TIME_EST_A, BYTE},
		{"bDeviceLifeTimeEstB", HEALTH_DESC_PARAM_LIFE_TIME_EST_B, BYTE},
	};

	struct desc_field_offset *tmp = NULL;

	for (i = 0; i < ARRAY_SIZE(health_desc_field_name); ++i) {
		tmp = &health_desc_field_name[i];

		ufs_read_desc_param(QUERY_DESC_IDN_HEALTH, 0, tmp->offset, &value, tmp->width_byte);

		count += snprintf((buf + count), PAGE_SIZE,
			"Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
			tmp->offset, tmp->name, value);
	}

	return count;

}
static DEVICE_ATTR_RO(dump_health_desc);

static ssize_t dump_string_desc_serial_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 ser_number[128] = { 0 };
	int i = 0, count = 0;

	ufs_get_string_desc(&ser_number, sizeof(ser_number), DEVICE_DESC_PARAM_SN, SD_RAW);

	count += snprintf((buf + count), PAGE_SIZE, "serial:");

	for (i = 2; i <  ser_number[QUERY_DESC_LENGTH_OFFSET]; i += 2)
		count += snprintf((buf + count), PAGE_SIZE, "%02x%02x",
			ser_number[i], ser_number[i+1]);

	count += snprintf((buf + count), PAGE_SIZE, "\n");

	return count;
}
static DEVICE_ATTR_RO(dump_string_desc_serial);

static ssize_t dump_device_desc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0, count = 0;
	u32 value = 0;

	struct desc_field_offset device_desc_field_name[] = {
		{"bLength",			DEVICE_DESC_PARAM_LEN,			BYTE},
		{"bDescriptorType",		DEVICE_DESC_PARAM_TYPE,			BYTE},
		{"bDevice",			DEVICE_DESC_PARAM_DEVICE_TYPE,		BYTE},
		{"bDeviceClass",		DEVICE_DESC_PARAM_DEVICE_CLASS,		BYTE},
		{"bDeviceSubClass",		DEVICE_DESC_PARAM_DEVICE_SUB_CLASS,	BYTE},
		{"bProtocol",			DEVICE_DESC_PARAM_PRTCL,		BYTE},
		{"bNumberLU",			DEVICE_DESC_PARAM_NUM_LU,		BYTE},
		{"bNumberWLU",			DEVICE_DESC_PARAM_NUM_WLU,		BYTE},
		{"bBootEnable",			DEVICE_DESC_PARAM_BOOT_ENBL,		BYTE},
		{"bDescrAccessEn",		DEVICE_DESC_PARAM_DESC_ACCSS_ENBL,	BYTE},
		{"bInitPowerMode",		DEVICE_DESC_PARAM_INIT_PWR_MODE,	BYTE},
		{"bHighPriorityLUN",		DEVICE_DESC_PARAM_HIGH_PR_LUN,		BYTE},
		{"bSecureRemovalType",		DEVICE_DESC_PARAM_SEC_RMV_TYPE,		BYTE},
		{"bSecurityLU",			DEVICE_DESC_PARAM_SEC_LU,		BYTE},
		{"Reserved",			DEVICE_DESC_PARAM_BKOP_TERM_LT,		BYTE},
		{"bInitActiveICCLevel",		DEVICE_DESC_PARAM_ACTVE_ICC_LVL,	BYTE},
		{"wSpecVersion",		DEVICE_DESC_PARAM_SPEC_VER,		WORD},
		{"wManufactureDate",		DEVICE_DESC_PARAM_MANF_DATE,		WORD},
		{"iManufactureName",		DEVICE_DESC_PARAM_MANF_NAME,		BYTE},
		{"iProductName",		DEVICE_DESC_PARAM_PRDCT_NAME,		BYTE},
		{"iSerialNumber",		DEVICE_DESC_PARAM_SN,			BYTE},
		{"iOemID",			DEVICE_DESC_PARAM_OEM_ID,		BYTE},
		{"wManufactureID",		DEVICE_DESC_PARAM_MANF_ID,		WORD},
		{"bUD0BaseOffset",		DEVICE_DESC_PARAM_UD_OFFSET,		BYTE},
		{"bUDConfigPLength",		DEVICE_DESC_PARAM_UD_LEN,		BYTE},
		{"bDeviceRTTCap",		DEVICE_DESC_PARAM_RTT_CAP,		BYTE},
		{"wPeriodicRTCUpdate",		DEVICE_DESC_PARAM_FRQ_RTC,		WORD},
		{"bUFSFeaturesSupport",		DEVICE_DESC_PARAM_FEAT_SUP,		BYTE},
		{"bFFUTimeout",			DEVICE_DESC_PARAM_FFU_TMT,		BYTE},
		{"bQueueDepth",			DEVICE_DESC_PARAM_Q_DPTH,		BYTE},
		{"wDeviceVersion",		DEVICE_DESC_PARAM_DEV_VER,		WORD},
		{"bNumSecureWpArea",		DEVICE_DESC_PARAM_NUM_SEC_WPA,		BYTE},
		{"dPSAMaxDataSize",		DEVICE_DESC_PARAM_PSA_MAX_DATA,		DWORD},
		{"bPSAStateTimeout",		DEVICE_DESC_PARAM_PSA_TMT,		BYTE},
		{"iProductRevisionLevel",	DEVICE_DESC_PARAM_PRDCT_REV,		BYTE},
	};

	struct desc_field_offset *tmp = NULL;
	u8 *p = (u8 *)&value;

	for (i = 0; i < ARRAY_SIZE(device_desc_field_name); ++i) {
		tmp = &device_desc_field_name[i];

		ufs_read_desc_param(QUERY_DESC_IDN_DEVICE, 0, tmp->offset, p, tmp->width_byte);
		switch (tmp->width_byte) {
		case BYTE:
			count += snprintf((buf + count), PAGE_SIZE,
			"Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
				tmp->offset, tmp->name, (u8)*p);
			break;
		case WORD:
			count += snprintf((buf + count), PAGE_SIZE,
			"Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
				tmp->offset, tmp->name, (u16)*p);
			break;
		case DWORD:
			count += snprintf((buf + count), PAGE_SIZE,
			"Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
				tmp->offset, tmp->name, (u32)*p);
			break;
		default:
			count += snprintf((buf + count), PAGE_SIZE,
			"Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
				tmp->offset, tmp->name, (u8)*p);
			break;
		}
	}

	return count;
}
static DEVICE_ATTR_RO(dump_device_desc);

/**
 * get toshiba hr inquiry
 */
static int mi_scsi_hr_inquiry(struct scsi_device *sdev, char *hr_inq, int len)
{
	int result;
	unsigned char cmd[16] = {0};

	if (!hr_inq)
		return -EINVAL;

	cmd[0] = INQUIRY;
	cmd[1] = 0x69;		/* EVPD */
	cmd[2] = 0xC0;
	cmd[3] = len >> 8;
	cmd[4] = len & 0xff;
	cmd[5] = 0;		/* Control byte */

	result = scsi_execute_req(sdev, cmd, DMA_FROM_DEVICE, hr_inq,
				  len, NULL, 30 * HZ, 3, NULL);
	if (result) {
		pr_err("ufs: get hr_inquiry result error\n");
		return -EIO;
	}

	/* Sanity check that we got the page back that we asked for */
	if (hr_inq[1] != 0xC0)
		pr_err("ufs: hr_inruiry data error\n");

	return 0;
}

/**
 * get sandisk device report
 */
static int mi_scsi_sdr(struct scsi_device *sdev, char *sdr, int len)
{
	int result;
	unsigned char cmd[16] = {0};

	if (!sdr)
		return -EINVAL;

	cmd[0] = READ_BUFFER;
	cmd[1] = 0x01;		/* mode vendor specific*/
	cmd[2] = 0x01;		/* buffer ID*/

	cmd[3] = 0x7D;
	cmd[4] = 0x9C;
	cmd[5] = 0x69;

	cmd[6] = 0x00;
	cmd[7] = 0x02;
	cmd[8] = 0x00;

	cmd[9] = 0;		/* Control byte */

	result = scsi_execute_req(sdev, cmd, DMA_FROM_DEVICE, sdr,
				  len, NULL, 30 * HZ, 3, NULL);

	if (result) {
		pr_err("ufs: get sdr result error\n");
		return -EIO;
	}

	return 0;
}

/**
 * get micron hr
 */
static int mi_scsi_mhr(struct scsi_device *sdev, char *hr, int len)
{
	int result;
	unsigned char write_buffer[16] = {
		0x3B, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x00};
	unsigned char read_buffer[16] = {
		0x3C, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};

	char VU[0x2c] = {0};

	VU[0] = 0xFE;
	VU[1] = 0x40;
	VU[3] = 0x10;
	VU[4] = 0x01;

	if (!hr)
		return -EINVAL;

	result = scsi_execute_req(sdev, write_buffer, DMA_TO_DEVICE, VU,
				  0x2c, NULL, 30 * HZ, 3, NULL);
	if (result) {
		pr_err("ufs: hr write buffer  error\n");
		return -EIO;
	}
	result = scsi_execute_req(sdev, read_buffer, DMA_FROM_DEVICE, hr,
				  len, NULL, 30 * HZ, 3, NULL);
	if (result) {
		pr_err("ufs: hr read buffer  error\n");
		return -EIO;
	}

	return 0;
}

/**
 * get samsung osv
 */
static int mi_scsi_osv(struct scsi_device *sdev, char *osv, int len)
{
	int result;
	unsigned char cmd[16] = {0};

	if (!osv)
		return -EINVAL;

	cmd[0] = 0xc0; /*VENDOR_SPECIFIC_CDB;*/
	cmd[1] = 0x40;

	cmd[4] = 0x01;
	cmd[5] = 0x0c;

	cmd[15] = 0x1c;

	result = scsi_execute_req(sdev, cmd, DMA_FROM_DEVICE, osv,
				  len, NULL, 30 * HZ, 3, NULL);
	if (result) {
		pr_err("ufs: get osv result error\n");
		return -EIO;
	}

	return 0;
}

static int ufs_get_hynix_hr(struct ufs_hba *hba, u8 *buf, u32 size)
{
	size = QUERY_DESC_HEALTH_MAX_SIZE;
	return ufshcd_read_desc_mi(hba, QUERY_DESC_IDN_HEALTH, 0, buf, size);
}

static ssize_t hr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int err = 0, i = 0;
	int len = 512; /*0x200*/
	char *hr;
	struct ufs_hba *hba = NULL;
	uint32_t count = 0;
	struct scsi_device *sdev;

	send_ufs_hba_data(&hba);

	sdev = hba->sdev_ufs_device;

	hr =  kzalloc(len, GFP_KERNEL);
	if (!hr) {
		pr_err("kzalloc fail\n");
		return -ENOMEM;
	}

	if (!strncmp(sdev->vendor, "WDC", 3)) {
		err = mi_scsi_sdr(sdev, hr, len);
	} else if (!strncmp(sdev->vendor, "TOSHIBA", 7)) {
		err = mi_scsi_hr_inquiry(sdev, hr, len);
	} else if (!strncmp(sdev->vendor, "SAMSUNG", 7)) {
		err = mi_scsi_osv(sdev, hr, 0x1c);/*0x200 is the same with 0x1c*/
	} else if (!strncmp(sdev->vendor, "MICRON", 6)) {
		err = mi_scsi_mhr(sdev, hr, len);
	} else if (!strncmp(sdev->vendor, "SKhynix", 7)) {
		err = ufs_get_hynix_hr(hba, hr, len);
	} else {
		count += snprintf((buf + count),  PAGE_SIZE, "NOT SUPPORTED %s\n", sdev->vendor);
		goto out;
	}

	if (err) {
		count += snprintf((buf + count),  PAGE_SIZE, "Fail to get hr, err is: %d\n", err);
	} else {
		for (i = 0; i < len; i++)
			count += snprintf((buf + count), PAGE_SIZE, "%02x", hr[i]);
		count += snprintf((buf + count), PAGE_SIZE, "\n");
	}

out:
	kfree(hr);
	return count;
}

static DEVICE_ATTR_RO(hr);

/*
 * NOTE: the official kernel also exposes show_hba / tag_stats / err_state /
 * req_stats here.  Those four only work on top of the newer ufs_stats layout
 * (pa_err_cnt / dl_err_cnt / UFS_EC_* / tag_stats / req_stats) which this
 * UFS driver does not have.  They are pure debug nodes and are not read by
 * MIUI, so they are intentionally left out; everything MIUI's ExtM and
 * system_perf_init actually read is kept below.
 */
static struct attribute *ufshcd_sysfs[] = {
	&dev_attr_dump_health_desc.attr,
	&dev_attr_dump_string_desc_serial.attr,
	&dev_attr_dump_device_desc.attr,
	&dev_attr_hr.attr,
	NULL,
};

const struct attribute_group ufshcd_sysfs_group = {
	.name = "ufshcd0",
	.attrs = ufshcd_sysfs,
};
