#ifndef __JPGDECODER_H
#define __JPGDECODER_H
#include "tjpgd.h"
#include "ff.h"
#include "lcd.h"

void SetJPGDisplayAera(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye);
void Load_Jpg (
	FIL *fp,		/* Pointer to the open file object to load */
	void *work,		/* Pointer to the working buffer (must be 4-byte aligned) */
	UINT sz_work	/* Size of the working buffer (must be power of 2) */
);

#endif
