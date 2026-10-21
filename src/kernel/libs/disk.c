#include "disk.h"
#include "io.h"
#include "stddef.h"
#include "stdio.h"

#define ATA_PRIMARY_DATA          0x1F0
#define ATA_PRIMARY_ERROR         0x1F1
#define ATA_PRIMARY_SECTOR_COUNT  0x1F2
#define ATA_PRIMARY_LBA_LOW       0x1F3
#define ATA_PRIMARY_LBA_MID       0x1F4
#define ATA_PRIMARY_LBA_HIGH      0x1F5
#define ATA_PRIMARY_DRIVE_HEAD    0x1F6
#define ATA_PRIMARY_STATUS        0x1F7
#define ATA_PRIMARY_COMMAND       0x1F7
#define ATA_PRIMARY_CONTROL       0x3F6

#define ATA_STATUS_BUSY           0x80
#define ATA_STATUS_DRIVE_READY    0x40
#define ATA_STATUS_DEVICE_FAULT   0x20
#define ATA_STATUS_DATA_REQUEST   0x08
#define ATA_STATUS_ERROR          0x01

#define ATA_CMD_READ_SECTORS      0x20
#define ATA_CMD_WRITE_SECTORS     0x30
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_IDENTIFY_DEVICE   0xEC

#define ATA_LBA28_MAX             0x0FFFFFFF
#define ATA_POLL_LIMIT            1000000u

typedef struct {
    uint16_t io_base;
    uint16_t control_base;
} ATA_Channel;

static ATA_Channel g_primary_channel = {
    .io_base = ATA_PRIMARY_DATA,
    .control_base = ATA_PRIMARY_CONTROL
};

static uint8_t ata_status(ATA_Channel* channel)
{
    return inb(channel->io_base + 7);
}

static void ata_delay_400ns(ATA_Channel* channel)
{
    for (int i = 0; i < 4; ++i)
        (void)inb(channel->control_base);
}

static bool ata_wait_not_busy(ATA_Channel* channel)
{
    for (uint32_t i = 0; i < ATA_POLL_LIMIT; ++i) {
        uint8_t status = ata_status(channel);
        if (status == 0 || status == 0xFF)
            return false;
        if (!(status & ATA_STATUS_BUSY))
            return true;
    }
    return false;
}

static bool ata_wait_data_request(ATA_Channel* channel)
{
    for (uint32_t i = 0; i < ATA_POLL_LIMIT; ++i) {
        uint8_t status = ata_status(channel);
        if (status == 0 || status == 0xFF)
            return false;
        if (status & ATA_STATUS_BUSY)
            continue;
        if (status & (ATA_STATUS_ERROR | ATA_STATUS_DEVICE_FAULT))
            return false;
        if (status & ATA_STATUS_DATA_REQUEST)
            return true;
    }
    return false;
}

static bool ata_wait_command_complete(ATA_Channel* channel)
{
    for (uint32_t i = 0; i < ATA_POLL_LIMIT; ++i) {
        uint8_t status = ata_status(channel);
        if (status == 0 || status == 0xFF)
            return false;
        if (status & ATA_STATUS_BUSY)
            continue;
        return !(status & (ATA_STATUS_ERROR | ATA_STATUS_DEVICE_FAULT));
    }
    return false;
}

static bool ata_select(DISK* disk, ATA_Channel** channel_out)
{
    if (!disk || disk->type != DISK_TYPE_ATA ||
        disk->driver_data != &g_primary_channel || disk->id > 1)
        return false;

    ATA_Channel* channel = (ATA_Channel*)disk->driver_data;
    if (!ata_wait_not_busy(channel))
        return false;

    outb(channel->io_base + 6, (uint8_t)(0xE0 | (disk->id << 4)));
    ata_delay_400ns(channel);

    if (!ata_wait_not_busy(channel))
        return false;

    *channel_out = channel;
    return true;
}

static bool ata_transfer_args_valid(DISK* disk, uint32_t lba, uint8_t count,
                                    const void* buffer)
{
    if (!disk || !buffer || count == 0 || disk->id > 1 ||
        disk->type != DISK_TYPE_ATA ||
        disk->driver_data != &g_primary_channel)
        return false;

    return lba <= ATA_LBA28_MAX &&
           (uint32_t)(count - 1) <= ATA_LBA28_MAX - lba;
}

static void ata_program_lba28(ATA_Channel* channel, DISK* disk,
                              uint32_t lba, uint8_t count)
{
    outb(channel->io_base + 6,
         (uint8_t)(0xE0 | (disk->id << 4) | ((lba >> 24) & 0x0F)));
    ata_delay_400ns(channel);
    outb(channel->io_base + 2, count);
    outb(channel->io_base + 3, (uint8_t)lba);
    outb(channel->io_base + 4, (uint8_t)(lba >> 8));
    outb(channel->io_base + 5, (uint8_t)(lba >> 16));
}

static void ata_insw(uint16_t port, uint16_t* buffer, uint32_t words)
{
    __asm__ volatile ("cld; rep insw"
                      : "+D"(buffer), "+c"(words)
                      : "d"(port)
                      : "memory");
}

static void ata_outsw(uint16_t port, const uint16_t* buffer, uint32_t words)
{
    __asm__ volatile ("cld; rep outsw"
                      : "+S"(buffer), "+c"(words)
                      : "d"(port)
                      : "memory");
}

bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
    if (!disk || (driveNumber != 0x80 && driveNumber != 0x81)) {
        kprintf("DISK: Supports primary ATA drives 0x80 and 0x81 only.\n");
        return false;
    }

    disk->id = (uint8_t)(driveNumber - 0x80);
    disk->type = DISK_TYPE_ATA;
    disk->driver_data = &g_primary_channel;

    ATA_Channel* channel = &g_primary_channel;
    outb(channel->io_base + 6, (uint8_t)(0xA0 | (disk->id << 4)));
    ata_delay_400ns(channel);

    outb(channel->control_base, 0x04);
    io_wait();
    outb(channel->control_base, 0x00);
    ata_delay_400ns(channel);

    if (!ata_wait_not_busy(channel)) {
        kprintf("DISK: ATA reset timed out or no drive is present.\n");
        disk->driver_data = NULL;
        return false;
    }

    outb(channel->io_base + 6, (uint8_t)(0xA0 | (disk->id << 4)));
    ata_delay_400ns(channel);
    outb(channel->io_base + 2, 0);
    outb(channel->io_base + 3, 0);
    outb(channel->io_base + 4, 0);
    outb(channel->io_base + 5, 0);
    outb(channel->io_base + 7, ATA_CMD_IDENTIFY_DEVICE);
    ata_delay_400ns(channel);

    uint8_t status = ata_status(channel);
    if (status == 0 || status == 0xFF) {
        kprintf("DISK: No ATA drive found.\n");
        disk->driver_data = NULL;
        return false;
    }

    if (!ata_wait_not_busy(channel)) {
        kprintf("DISK: ATA IDENTIFY timed out.\n");
        disk->driver_data = NULL;
        return false;
    }

    if (inb(channel->io_base + 4) != 0 || inb(channel->io_base + 5) != 0) {
        kprintf("DISK: Selected device is not an ATA disk.\n");
        disk->driver_data = NULL;
        return false;
    }

    if (!ata_wait_data_request(channel)) {
        kprintf("DISK: ATA IDENTIFY failed.\n");
        disk->driver_data = NULL;
        return false;
    }

    uint16_t identify_data[256];
    ata_insw(channel->io_base, identify_data, 256);
    (void)identify_data;

    if (!ata_wait_command_complete(channel)) {
        kprintf("DISK: ATA IDENTIFY reported an error.\n");
        disk->driver_data = NULL;
        return false;
    }

    kprintf("DISK: Initialized primary ATA %s.\n",
            disk->id == 0 ? "master" : "slave");
    return true;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* buffer)
{
    if (!ata_transfer_args_valid(disk, lba, count, buffer)) {
        kprintf("DISK: Invalid ATA read request.\n");
        return false;
    }

    ATA_Channel* channel;
    if (!ata_select(disk, &channel)) {
        kprintf("DISK: ATA device is not ready for reading.\n");
        return false;
    }

    ata_program_lba28(channel, disk, lba, count);
    outb(channel->io_base + 7, ATA_CMD_READ_SECTORS);

    uint16_t* target = (uint16_t*)buffer;
    for (uint32_t sector = 0; sector < count; ++sector) {
        if (!ata_wait_data_request(channel)) {
            kprintf("DISK: ATA read failed at LBA %u.\n", lba + sector);
            return false;
        }
        ata_insw(channel->io_base, target, 256);
        target += 256;
    }

    if (!ata_wait_command_complete(channel)) {
        kprintf("DISK: ATA read completion failed.\n");
        return false;
    }
    return true;
}

bool DISK_WriteSectors(DISK* disk, uint32_t lba, uint8_t count,
                       const void* buffer)
{
    if (!ata_transfer_args_valid(disk, lba, count, buffer)) {
        kprintf("DISK: Invalid ATA write request.\n");
        return false;
    }

    ATA_Channel* channel;
    if (!ata_select(disk, &channel)) {
        kprintf("DISK: ATA device is not ready for writing.\n");
        return false;
    }

    ata_program_lba28(channel, disk, lba, count);
    outb(channel->io_base + 7, ATA_CMD_WRITE_SECTORS);

    const uint16_t* source = (const uint16_t*)buffer;
    for (uint32_t sector = 0; sector < count; ++sector) {
        if (!ata_wait_data_request(channel)) {
            kprintf("DISK: ATA write failed at LBA %u.\n", lba + sector);
            return false;
        }
        ata_outsw(channel->io_base, source, 256);
        source += 256;
    }

    if (!ata_wait_command_complete(channel)) {
        kprintf("DISK: ATA write completion failed.\n");
        return false;
    }

    outb(channel->io_base + 7, ATA_CMD_CACHE_FLUSH);
    if (!ata_wait_command_complete(channel)) {
        kprintf("DISK: ATA cache flush failed.\n");
        return false;
    }
    return true;
}
