#include "irq.h"
#include "keyboard.h"
#include "low_level.h"
#include "screen.h"
#include "math.h"
#include "std.h"
#include "memory.h"
#include "mouse.h"
#include "ps2.h"

// status byte for keyboard
// [n,n,n,n,caps,shift,alt,ctrl]
static unsigned char kstatus = 0;
static bool taking_input = false;
static bool taking_char = false;
static char *input_buffer;
static char *char_buffer;
static unsigned int buffer_index = 0;
static unsigned int buffer_limit = 0;
static unsigned int orig_scroll_index = 0;

void enable_shift()
{

  kstatus |= 0b00000100;
  switch_top_bar_value(SHIFT_OFF,5);
}

void disable_shift()
{

  kstatus &= ~0b00000100;
  switch_top_bar_value(SHIFT_OFF,5);

}

int check_shift()
{

  return kstatus & 0b00000100;

}

void enable_ctrl()
{

  kstatus |= 0b00000001;
  switch_top_bar_value(CTRL_OFF,4);

}

void disable_ctrl()
{

  kstatus &= ~0b00000001;
  switch_top_bar_value(CTRL_OFF,4);

}

int check_ctrl()
{

  return kstatus & 0b00000001;

}

void enable_caps()
{

  kstatus |= 0b00001000;
  switch_top_bar_value(CAPS_OFF,4);

}

void disable_caps()
{

  kstatus &= ~0b00001000;
  switch_top_bar_value(CAPS_OFF,4);

}

int check_caps()
{

  return kstatus & 0b00001000;

}

void enable_alt()
{

  kstatus |= 0b00000010;
  switch_top_bar_value(ALT_OFF,3);

}

void disable_alt()
{

  kstatus &= ~0b00000010;
  switch_top_bar_value(ALT_OFF,3);

}

int check_alt()
{

  return kstatus & 0b00000010;

}


// us keyboard layout scancode->ascii
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

unsigned char kbdus_shift[128] =
{
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
  '(', ')', '_', '+', '\b', /* Backspace */
  '\t',     /* Tab */
  'Q', 'W', 'E', 'R', /* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', /* Enter key */
    0,      /* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
 '\"', '~',   0,    /* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',      /* 49 */
  'M', '<', '>', '?',   0,        /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

//! read status from keyboard controller
char kybrd_ctrl_read_status () {

  return inb (PS_CTRL);
}

/* Handles the keyboard interrupt */
void keyboard_handler()
{

    if (!(kybrd_ctrl_read_status()&1))
        return;

    unsigned char scancode;

    /* Read from the keyboard's data buffer */
    scancode = inb(PS_DATA);

    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    if (scancode & 0x80)
    {
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */

      if (scancode == LSHIFT_REL || scancode == RSHIFT_REL) // shift
      {

        disable_shift();

      }
      else if (scancode == CTRL_REL) // ctrl
      {

        disable_ctrl();

      }
      else if (scancode == ALT_REL) // alt
      {

        disable_alt();

      }
      else if (scancode == CAPS_REL) // caps
      {

        if(check_caps())
          disable_caps();
        else
          enable_caps();

      }

    }
    else
    {
        if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) // shift
        {

          enable_shift();
          return;

        }
        else if (scancode == CTRL_PRESS) // ctrl
        {

          enable_ctrl();
          return;

        }
        else if (scancode == ALT_PRESS) // alt
        {

          enable_alt();
          return;

        }
        else if (scancode == CAPS_PRESS) // caps
        {
          return;
        }
        else if (scancode == DOWN_ARROW_PRESS)
        {

          scroll_down();
          return;

        }
        else if (scancode == UP_ARROW_PRESS)
        {

          scroll_up();
          return;

        }

        handle_character(scancode);
    }
}

static void handle_character(unsigned char scancode)
{

  if (!kbdus[scancode])
    return;

  int character_status = 0; // 0 = print

  if (check_shift())
  {
    if (taking_input)
      character_status = keyboard_input_character(kbdus_shift[scancode]);
    else if (taking_char)
      keyboard_get_character(kbdus_shift[scancode]);
    if (character_status != -1)
      putchar(kbdus_shift[scancode]);
  }
  else
  {

    char ascii = kbdus[scancode];
    if (check_caps() && 'a' <= ascii && 'z' >= ascii)
      ascii -= 'a'-'A';
    if (taking_input)
      character_status = keyboard_input_character(ascii);
    else if (taking_char)
      keyboard_get_character(ascii);
    if (character_status != -1)
      putchar(ascii);
  }

  if (taking_input)
  {
    set_cursor_input_coords(get_cursor_row(), get_cursor_col());
  }

}

void virtual_keyboard_input(unsigned char ascii)
{

  int character_status = 0; // 0 = print

  if (taking_input)
    character_status = keyboard_input_character(ascii);
  if (character_status != -1)
    putchar(ascii);
  if (taking_input)
  {
    set_cursor_input_coords(get_cursor_row(), get_cursor_col());
  }

}

// row = the row you want to use, col = the col you want to use, bf_size = how many characters, including \n should you take from the user buffer = pointer to buffer.
// if row<0 then row=cursor row, if col<0 then col=cursor col.
void keyboard_input(int row, int col, char *buffer, int bf_size)
{

  if(taking_char)
    return;

  taking_input = true;

  if(row < 0)
    row = get_cursor_row();
  if(col < 0)
    col = get_cursor_col();

  if (row < TOP)
    row = TOP;
  else if (row >= MAX_ROWS)
    row = MAX_ROWS-1;

  if (col < 0)
    col = 0;
  else if (col >= MAX_COLS)
    col = MAX_COLS-1;

  set_cursor_input_coords(row, col);
  buffer_index = 0;

  buffer_limit = bf_size;
  input_buffer = buffer;

  orig_scroll_index = get_scroll_index() + get_cursor_row() - TOP;

}

bool is_taking_input()
{

  return taking_input;

}

bool is_taking_char()
{

  return taking_char;

}

// returns -1 if reached limit (start/end) and must be supplied with \n, zero if continuing and 1 if finished successfully.
static int keyboard_input_character(char character)
{
  fit_to_scroll(orig_scroll_index);
  attach_cursor_to_input();

  if (character == '\b') // handle backspace
  {

    if (buffer_index == 0)
      return -1;

    buffer_index--;
    return 0;

  }

  else if (character == '\n') // handle newline (submit string)
  {
    input_buffer[buffer_index] = 0; // end of string
    taking_input = false;
    return 1;
  }

  if (buffer_index == buffer_limit-1) // only if character is \n let it pass and finish taking the input
    return -1;

  input_buffer[buffer_index] = character;
  buffer_index++;

  return 0;

}

// row = the row you want to use, col = the col you want to use
// if row<0 then row=cursor row, if col<0 then col=cursor col.
void getchar(int row, int col, char *buffer)
{

  if(taking_input)
    return;

  taking_char = true;

  if(row < 0)
    row = get_cursor_row();
  if(col < 0)
    col = get_cursor_col();

  if (row < TOP)
    row = TOP;
  else if (row >= MAX_ROWS)
    row = MAX_ROWS-1;

  if (col < 0)
    col = 0;
  else if (col >= MAX_COLS)
    col = MAX_COLS-1;

  set_cursor_input_coords(row, col);

  char_buffer = buffer;

  orig_scroll_index = get_scroll_index() + get_cursor_row() - TOP;

}

static void keyboard_get_character(char character)
{
  fit_to_scroll(orig_scroll_index);
  attach_cursor_to_input();
  taking_char = false;
  *char_buffer = character;

}

void keyboard_install()
{
    // // reset the keyboard
    // uint8_t tmp = inb(0x61);
    // outb(0x61, tmp | 0x80);
    // outb(0x61, tmp & 0x7F);
    // inb(PS_DATA); // ack

    irq_install_handler(1, keyboard_handler);

}
