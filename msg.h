/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#ifndef MSG_H
#define MSG_H

#ifdef __cplusplus
extern "C" {
#endif

/* defines for the message system */
#define MSG_BLANK 256
#define MSG_INFO 4
#define MSG_DEBUG 3
#define MSG_ERROR 2
#define MSG_DIALOG 1
#define MSG_FATAL 0
#define MSG_DEFAULT MSG_ERROR

void msg(int, char *, ...);
void msg_setlevel(int);


#ifdef __cplusplus
}
#endif

#endif
