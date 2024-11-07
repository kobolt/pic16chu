#ifndef _CHIPVIEW_H
#define _CHIPVIEW_H

#include "pic.h"

void chipview_pause(void);
void chipview_resume(void);
void chipview_exit(void);
void chipview_update(pic_t *pic);
void chipview_init(pic_t *pic);

#endif /* _CHIPVIEW_H */
