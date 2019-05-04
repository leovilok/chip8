#ifndef C8_MEDIA_H
#define C8_MEDIA_H

int m_init(int argc, char **argv);

void m_quit(void);

unsigned short get_input(unsigned short input);
unsigned short wait_input(unsigned short input);

void clear_screen(void);
void draw(int x, int y, int value);
void send_draw(void);
void set_buzzer_state(int state);

void wait_tick(void);

#endif /* C8_MEDIA_H */
