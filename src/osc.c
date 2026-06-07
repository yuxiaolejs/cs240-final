// reference: https://github.com/yuxiaolejs/midas-control
#include "osc.h"
#include "cstr.h"
#include <stdarg.h>

uint32_t encode_message_va(char *buf, char *path, char *types, va_list args)
{
    uint32_t path_len = strlen(path) + 1;
    uint32_t current_offset = path_len;
    memcpy(buf, path, path_len);
    while (current_offset % 4 != 0)
        buf[current_offset++] = '\0';
    buf[current_offset++] = ',';
    uint32_t type_len = strlen(types) + 1;
    memcpy(buf + current_offset, types, type_len);
    current_offset += type_len;
    while (current_offset % 4 != 0)
        buf[current_offset++] = '\0';
    while (*types)
    {
        switch (*types)
        {
        case 'i':
        {
            int val = va_arg(args, int);
            buf[current_offset++] = (val >> 24) & 0xFF;
            buf[current_offset++] = (val >> 16) & 0xFF;
            buf[current_offset++] = (val >> 8) & 0xFF;
            buf[current_offset++] = val & 0xFF;
            break;
        }
        case 'f':
        {
            float val = (float)va_arg(args, double); // float is promoted to double in varargs
            uint32_t as_int;
            memcpy(&as_int, &val, sizeof(float));
            buf[current_offset++] = (as_int >> 24) & 0xFF;
            buf[current_offset++] = (as_int >> 16) & 0xFF;
            buf[current_offset++] = (as_int >> 8) & 0xFF;
            buf[current_offset++] = as_int & 0xFF;
            break;
        }
        case 's':
        {
            char *val = va_arg(args, char *);
            uint32_t str_len = strlen(val) + 1;
            memcpy(buf + current_offset, val, str_len);
            current_offset += str_len;
            while (current_offset % 4 != 0)
                buf[current_offset++] = '\0';
            break;
        }
        default:
            printk("Unsupported type: %c\n", *types);
            break;
        }
        types++;
    }
    return current_offset;
}

uint32_t encode_message(char *buf, char *path, char *types, ...)
{
    va_list args;
    va_start(args, types);
    uint32_t current_offset = encode_message_va(buf, path, types, args);
    va_end(args);
    return current_offset;
}

uint32_t decode_message(char *buf, char *path, char *types, uint8_t *data)
{
    uint32_t offset = 0;
    uint32_t path_len = strlen(buf) + 1;
    memcpy(path, buf, path_len);
    offset += path_len;
    while (offset % 4 != 0)
        offset++;
    if (buf[offset] != ',')
    {
        printk("Invalid OSC message: missing type tag\n");
        return 0;
    }
    offset++;
    uint32_t type_len = strlen(buf + offset) + 1;
    memcpy(types, buf + offset, type_len);
    offset += type_len;
    while (offset % 4 != 0)
        offset++;
    uint32_t data_offset = 0;
    for (uint32_t i = 0; types[i]; i++)
    {
        switch (types[i])
        {
        case 'i':
            data[data_offset++] = buf[offset + 3];
            data[data_offset++] = buf[offset + 2];
            data[data_offset++] = buf[offset + 1];
            data[data_offset++] = buf[offset];
            offset += 4;
            break;
        case 'f':
            data[data_offset++] = buf[offset + 3];
            data[data_offset++] = buf[offset + 2];
            data[data_offset++] = buf[offset + 1];
            data[data_offset++] = buf[offset];
            offset += 4;
            break;
        case 's':
        {
            uint32_t str_start = offset;
            data[data_offset++] = (str_start >> 0) & 0xFF;
            data[data_offset++] = (str_start >> 8) & 0xFF;
            data[data_offset++] = (str_start >> 16) & 0xFF;
            data[data_offset++] = (str_start >> 24) & 0xFF;
            while (buf[offset] != '\0')
                offset++;
            uint32_t str_len = offset - str_start + 1;
            offset++;
            while (offset % 4 != 0)
                offset++;
            break;
        }
        case 'b':
        {
            uint32_t blob_len = (buf[offset] << 24) | (buf[offset + 1] << 16) | (buf[offset + 2] << 8) | buf[offset + 3];
            // printk("[osc decode] blob len: %d\n", blob_len);
            offset += 4;
            for(uint32_t j = 0; j < blob_len; j++)
                data[data_offset++] = buf[offset + j];
            offset += blob_len;
            break;
        }
        default:
            printk("Unsupported type: %c\n", types[i]);
            break;
        }
    }
    return data_offset;
}