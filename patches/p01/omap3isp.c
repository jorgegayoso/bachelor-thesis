#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct omap3isp_stat_data {
    struct {
        int64_t tv_sec;
        int64_t tv_usec;
    } ts;
    void *buf;
    uint32_t buf_size;
    uint16_t frame_number;
    uint16_t cur_frame;
    uint16_t config_counter;
};

struct omap3isp_stat_data_time32 {
    struct {
        int32_t tv_sec;
        int32_t tv_usec;
    } ts;
    uint32_t buf;
    uint32_t buf_size;
    uint16_t frame_number;
    uint16_t cur_frame;
    uint16_t config_counter;
};

/* IOO bug function */
int omap3isp_stat_request_statistics_time32(
    struct omap3isp_stat_data_time32 *data)
{
    struct omap3isp_stat_data data64;

    memset(&data64, 0xAA, sizeof(data64));
    data64.ts.tv_sec  = 1234;
    data64.ts.tv_usec = 5678;

    data->ts.tv_sec  = (int32_t)data64.ts.tv_sec;
    data->ts.tv_usec = (int32_t)data64.ts.tv_usec;

    /* destination typed as &data->buf (one field),
       but size spans buf + buf_size + frame_number + cur_frame + config_counter */
    memcpy(&data->buf, &data64.buf, sizeof(*data) - sizeof(data->ts));

    return 0;
}