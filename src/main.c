#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define true 1
#define false 0

#define TEXTCOLOR ((Color){105, 2, 2, 255})

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}
int Width = 500, Height = 500;

Font default_font;

#define draw_text(string, x, y, size, color) \
    DrawTextEx(default_font, string, (Vector2){x, y}, size, size / 3, color); // Draw text using font and additional parameters

typedef enum
{
    DRAWBUFFER,
} global_state;

typedef enum
{
    EDIT,
} mode;

mode current_mode = EDIT;
global_state state = DRAWBUFFER;

typedef struct
{
    int x, y;
} vec2;

typedef struct
{
    size_t column, row;
} cursor_t;

cursor_t cursor = (cursor_t){1, 1};

static bool keyboard[512];
bool shift, cntrl, alt;

// takes in float from 0 to 1
vec2 to_screen_space(float x, float y)
{
    return (vec2){x * Width, y * Height};
}

typedef struct
{
    char *text;
    size_t length;
    bool readonly;
    char *title;
} buffer_t;

void write_to_buffer(buffer_t *buffer, char *text)
{
    if (!buffer)
    {
        printf("Buffer Doesn't Exist.\n");
        return;
    }
    if (buffer->readonly)
    {
        printf("Cannot Write To Buffer, The Buffer Is ReadOnly.\n");
        return;
    }
    if (!text)
    {
        printf("Cannot Write Nothing To Buffer.\n");
        return;
    }
    if (!buffer->text)
    {
        buffer->text = malloc(strlen(text));
        memcpy(buffer->text, text, strlen(text));
        buffer->length = strlen(text);
        return;
    }
    char *new_text = malloc(buffer->length + strlen(text) + 1);
    memset(new_text, 0, buffer->length + strlen(text) + 1); // just in case
    memcpy(new_text, buffer->text, buffer->length);
    free(buffer->text);
    buffer->text = new_text;
    char *end_of_old_text = new_text + buffer->length;
    memcpy(end_of_old_text, text, strlen(text));
    buffer->length += strlen(text);
}

char *null_terminate_string(char *string, size_t bytes)
{
    char *null_terminated = malloc(bytes + 1);
    memset(null_terminated, 0, bytes + 1);
    memcpy(null_terminated, string, bytes);
    return null_terminated;
}
char *null_terminate_buffer(buffer_t buffer)
{
    char *null_terminated = null_terminate_string(buffer.text, buffer.length);
    return null_terminated;
}

void draw_title(buffer_t buffer)
{
    vec2 Position = to_screen_space(0, 0);
    vec2 Rect = (vec2){Width, 51};
    DrawRectangle(Position.x, Position.y, Rect.x, Rect.y, (Color){124, 114, 84, 255});
    draw_text(
        buffer.title, Position.x + 10,
        Position.y + 20,
        20,
        ((Color){3, 29, 66, 255}));
}

size_t get_number_of_lines(char *text, size_t length)
{
    size_t n = 0;
    size_t ret = 1;
    while (n < length)
    {
        if (text[n] == '\n')
        {
            ++ret;
        }
        ++n;
    }
    return ret;
}
void draw_all_lines(char *text, size_t length, int x, int y)
{
    size_t n_newlines = get_number_of_lines(text, length);
    for (size_t i = 0; i < n_newlines; ++i)
    {
        char *buffer = malloc(32);
        memset(buffer, 0, 32);
        sprintf(buffer, "%ld", i + 1);
        draw_text(buffer, to_screen_space(0.005, 0).x, y + (i * 30), 20, ((Color){3, 29, 66, 255}));
        free(buffer);
    }
    DrawRectangle(22, 51, to_screen_space(0.003, 0).x, Height - 51, BLACK);
    char *str = null_terminate_string(text, length);
    draw_text(str, x, y, 20, TEXTCOLOR);
    free(str);
}

void draw_buffer(buffer_t buffer)
{
    draw_title(buffer);
    vec2 pos = (vec2){30, 60};
    draw_all_lines(buffer.text, buffer.length, pos.x, pos.y);
}

char *read_file(char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("File %s could not be read.\n", path);
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    char *string = malloc(fsize);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    return string;
}

void draw_cursor()
{
    size_t y = cursor.row * 30;
    y += 28;
    size_t x = 40;
    DrawLine(x + ((cursor.column - 1) * 15) + (20 / 3), y, x + ((cursor.column - 1) * 15) + (20 / 3), y + 20, BLACK);
}

void write_char_at_cursor(buffer_t buffer, char key)
{
    char *string = buffer.text;
    char *substring_a = null_terminate_string(buffer.text, cursor.column);
    char *substring_b = null_terminate_string(buffer.text + cursor.column, buffer.length - cursor.column);
    char *Buff = malloc(1024 * 1024 * 8);
    ++buffer.length;
    memset(Buff, 0, 1024 * 1024 * 8);
    sprintf(Buff, "%s %c %s", substring_a, key, substring_b);
    free(buffer.text);
    buffer.text = 0;
    buffer.length = 0;
    write_to_buffer(&buffer, Buff);
    buffer.length = strlen(Buff);
    printf(Buff);
}

void handle_keyboard(buffer_t buffer)
{
    msleep(60);
    if (IsKeyDown(KEY_LEFT_SHIFT) && !shift)
    {
        shift = true;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) && !cntrl)
    {
        cntrl = true;
    }
    if (IsKeyUp(KEY_LEFT_SHIFT) && shift)
    {
        shift = false;
    }
    if (IsKeyUp(KEY_LEFT_CONTROL) && cntrl)
    {
        cntrl = false;
    }
    if (IsKeyDown(KEY_LEFT_ALT) && !alt)
    {
        alt = true;
    }
    if (IsKeyUp(KEY_LEFT_ALT) && alt)
    {
        alt = false;
    }
    int key = GetKeyPressed();
    if (IsKeyDown(KEY_ENTER))
    {
        key = '\n';
    }
    for (int i = 0; i != 512; ++i)
    {
        if (keyboard[i])
        {
            if (i >= 'a' && i <= 'z')
            {
                keyboard[i] = IsKeyDown(i - 'a' + 'A') && !IsKeyDown(KEY_LEFT_SHIFT);
                continue;
            }
            keyboard[i] = IsKeyDown(i);
        }
    }
    while (key)
    {
        if (key >= '0' && key <= '9')
        {
            if (shift)
            {
                char *modifiers = ")!@#$%^&*(";
                key = modifiers[key - '0'];
            }
        }
        if (key == ';' && shift)
            key = ':';
        if (key == '\'' && shift)
            key = '"';
        if (key == ',' && shift)
            key = '<';
        if (key == '.' && shift)
            key = '>';
        if (key == '[' && shift)
            key = '{';
        if (key == ']' && shift)
            key = '}';
        if (key >= 'A' && key <= 'Z')
        {
            if (!IsKeyDown(KEY_LEFT_SHIFT))
            {
                key = key - 'A' + 'a';
            }
        }
        if (key == KEY_LEFT_SHIFT && shift)
        {
            key = GetKeyPressed();
            continue;
        }
        if (key == KEY_LEFT_CONTROL && cntrl)
        {
            key = GetKeyPressed();
            continue;
        }
        if (key == KEY_LEFT_ALT && alt)
        {
            key = GetKeyPressed();
            continue;
        }
        if (key != KEY_LEFT && key != KEY_RIGHT && key != KEY_UP && key != KEY_DOWN)
        {
            write_char_at_cursor(buffer, key);
        }
        // printf("%c\n", key);
        keyboard[key] = true;
        key = GetKeyPressed();
    }
}

int main(void)
{
reload:
    buffer_t buffer = {0};
    buffer.title = "hello.txt";
    buffer.length = 0;
    write_to_buffer(&buffer, read_file("hello.txt"));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(Width, Height, "Test");
    SetTargetFPS(60);
    int arrowDelay = 0;
    bool arrowKeyPressed = false;
    while (!WindowShouldClose())
    {
        BeginDrawing();

        if (keyboard[KEY_RIGHT] || keyboard[KEY_LEFT])
        {
            if (arrowKeyPressed)
            {
                // Continue moving after a delay
                if (arrowDelay >= 20)
                {
                    if (keyboard[KEY_RIGHT])
                    {
                        ++cursor.column;
                        msleep(60);
                    }
                    if (keyboard[KEY_LEFT])
                    {
                        --cursor.column;
                        msleep(60);
                    }
                }
                else
                {
                    ++arrowDelay;
                }
            }
            else
            {
                arrowKeyPressed = true;
                arrowDelay = 0;
                if (keyboard[KEY_RIGHT])
                {
                    ++cursor.column;
                }
                if (keyboard[KEY_LEFT])
                {
                    --cursor.column;
                }
            }
        }
        else
        {
            // Reset arrow key state
            arrowKeyPressed = false;
            arrowDelay = 0;
        }

        if (IsWindowResized())
        {
            Width = GetScreenWidth();
            Height = GetScreenHeight();
        }
        char *string = null_terminate_buffer(buffer);
        ClearBackground((Color){225, 193, 143});
        // DrawText(string, 50, 50, 20, LIGHTGRAY);
        if (state == DRAWBUFFER)
        {
            draw_buffer(buffer);
            draw_cursor();
            handle_keyboard(buffer);
        }
        free(string);
        EndDrawing();
    }
    UnloadFont(default_font);
}