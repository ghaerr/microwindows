/* New code from #116 - Change functionality from UTF-8 to UCS-2 */
#include <stdlib.h>
#include <string.h>

/* Вспомогательная функция для преобразования UTF‑8 в UCS‑2.
 * Возвращает количество символов (unsigned short) в out,
 * out нужно освобождать после использования.
 * length – число байтов во входной строке.
 */
static int utf8_to_ucs2(const char* utf8, int length, unsigned short **out) {
    int count = 0;
    const unsigned char *p = (const unsigned char *)utf8;
    int remaining = length;
    unsigned short *buffer = malloc(length * sizeof(unsigned short));
    if (!buffer)
        return -1;
    while(remaining > 0) {
        unsigned int code = 0;
        int consumed = 0;
        if(p[0] < 0x80) {
            code = p[0];
            consumed = 1;
        } else if((p[0] & 0xE0) == 0xC0) {
            if(remaining < 2) break;
            code = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            consumed = 2;
        } else if((p[0] & 0xF0) == 0xE0) {
            if(remaining < 3) break;
            code = ((p[0] & 0x0F) << 12) |
                   ((p[1] & 0x3F) << 6)  |
                   (p[2] & 0x3F);
            consumed = 3;
        } else {
            /* 4‑байтовые последовательности не обрабатываются */
            consumed = 1;
        }
        buffer[count++] = (unsigned short) code;
        p += consumed;
        remaining -= consumed;
    }
    *out = buffer;
    return count;
}

/* Функция отрисовки текста.
 * Здесь length – число байтов в строке.
 * Мы преобразуем строку в UCS‑2 для корректного измерения,
 * затем вычисляем горизонтальное смещение с компенсацией для кириллицы.
 */
NK_API void
nk_draw_text(struct nk_command_buffer *b, struct nk_rect r,
    const char *string, int length, const struct nk_user_font *font,
    struct nk_color bg, struct nk_color fg)
{
    float text_width = 0;
    struct nk_command_text *cmd;
    int tx;
    int compensation = 0;

    NK_ASSERT(b);
    NK_ASSERT(font);
    if (!b || !string || !length || (bg.a == 0 && fg.a == 0))
        return;
    if (b->use_clipping) {
        const struct nk_rect *c = &b->clip;
        if (c->w == 0 || c->h == 0 ||
            !NK_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    /* Преобразуем UTF‑8 строку в UCS‑2 для корректного измерения */
    unsigned short *ucs2_str = NULL;
    int ucs2_length = utf8_to_ucs2(string, length, &ucs2_str);
    if (ucs2_length < 0) {
        return;
    }
    text_width = font->width(font->userdata, font->height, (const char*)ucs2_str, ucs2_length);
    free(ucs2_str);

    /* Если текст (с компенсацией) помещается в область, центрируем его.
       Если нет – выравниваем по левому краю с компенсацией. */
    /* Определяем компенсацию для кириллицы:
       Если первый байт строки равен 0xD0 или 0xD1, это кириллица */
    {
        unsigned char first = (unsigned char)string[0];
        if (first == 0xD0 || first == 0xD1) {
            compensation = 10; // Подберите оптимальное значение экспериментально
        }
    }

    float adjusted_width = text_width + compensation;
    if (adjusted_width <= r.w) {
        tx = r.x + (int)((r.w - adjusted_width) / 2.0f);
    } else {
        tx = r.x + compensation;
    }

    /* Вертикальное выравнивание: здесь используется верхняя граница,
       можно изменить на центрирование: r.y + (r.h - font->height) / 2 */
    int ty = r.y;

    if (!length)
        return;
    cmd = (struct nk_command_text*)
        nk_command_buffer_push(b, NK_COMMAND_TEXT, sizeof(*cmd) + (nk_size)(length + 1));
    if (!cmd)
        return;
    cmd->x = (short)tx;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->background = bg;
    cmd->foreground = fg;
    cmd->font = font;
    cmd->length = length;
    cmd->height = font->height;
    NK_MEMCPY(cmd->string, string, (nk_size)length);
    cmd->string[length] = '\0';
}
