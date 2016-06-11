#include "jpegdecoder.h"


static int MaskT, MaskL, MaskR, MaskB;	/* Active drawing area */

/****************************************************************************************************************************
*函数名称：JpgDecoderRead()
*参数    :
*返回值  :
*功能描述:用于JPG解码读取内存卡JPG文件数据，内部调用
******************************************************************************************************************************/
static UINT JpgDecoderRead(JDEC* jd,		/* Decoder object */BYTE* buff,		/* Pointer to the read buffer (NULL:skip) */UINT nd			/* Number of bytes to read/skip from input stream */)
{
	UINT rb;
	FIL *fil = (FIL*)jd->device;	/* Input stream of this session */


	if (buff) {	/* Read nd bytes from the input strem */
		f_read(fil, buff, nd, &rb);
		return rb;	/* Returns number of bytes could be read */

	} else {	/* Skip nd bytes on the input stream */
		return (f_lseek(fil, f_tell(fil) + nd) == FR_OK) ? nd : 0;
	}
}
void SetJPGDisplayAera(uint16_t xs,uint16_t ys,uint16_t xe,uint16_t ye)
{
		MaskL = xs;
		MaskR = xe;
		MaskT = ys;
		MaskB = ye;
}
/****************************************************************************************************************************
*函数名称：Jpg_Display()
*参数    :
*返回值  :
*功能描述:用于JPG解码后将RGB数据显示到TFT液晶屏，内部调用
******************************************************************************************************************************/
static UINT Jpg_Display (
	JDEC* jd,		/* Decoder object */
	void* bitmap,	/* Bitmap data to be output */
	JRECT* rect		/* Rectangular region to output */
)
{
		int yc, xc, xl, xs;
	 uint16_t *pData=(uint16_t*)bitmap;
	jd = jd;	/* Suppress warning (device identifier is not needed) */

	/*将解码数据显示到液晶屏区域*/
	//disp_blt(rect->left, rect->right, rect->top, rect->bottom, (uint16_t*)bitmap);
	


	if (rect->left > rect->right || rect->top > rect->bottom) return 0; 	/* Check varidity */
	if (rect->left > MaskR || rect->right < MaskL  || rect->top > MaskB || rect->bottom < MaskT) return 0;	/* Check if in active area */

	yc = rect->bottom - rect->top + 1;			/* Vertical size */
	xc = rect->right - rect->left + 1; xs = 0;	/* Horizontal size and skip */

	if (rect->top < MaskT) {		/* Clip top of source image if it is out of active area */
		pData += xc * (MaskT - rect->top);
		yc -= MaskT - rect->top;
		rect->top = MaskT;
	}
	if (rect->bottom > MaskB) {	/* Clip bottom of source image if it is out of active area */
		yc -= rect->bottom - MaskB;
		rect->bottom = MaskB;
	}
	if (rect->left < MaskL) {		/* Clip left of source image if it is out of active area */
		pData += MaskL - rect->left;
		xc -= MaskL - rect->left;
		xs += MaskL - rect->left;
		rect->left = MaskL;
	}
	if (rect->right > MaskR) {	/* Clip right of source image it is out of active area */
		xc -= rect->right - MaskR;
		xs += rect->right - MaskR;
		rect->right = MaskR;
	}
	/*设置水平方向X开始坐标*/

//     LCD_WriteReg(0x50, rect->left);
//   /*设置水平方向X结束坐标*/
//   LCD_WriteReg(0x51, rect->right);
//   /*设置竖直方向Y开始坐标*/
//     LCD_WriteReg(0x52, rect->top);
//   /*设置竖直方向Y结束坐标*/
//   LCD_WriteReg(0x53, rect->bottom);
  LCD_SetCursor(rect->left, rect->top);
	LCD_WriteRAM_Start();
	do {	/* Send image data */
		xl = xc;
		do {
			//pd = *pat++;
			LCD_WriteRAM(*pData++);			//显示像素
		} while (--xl);
		pData += xs;
	} while (--yc);
	return 1;	/* Continue decompression */	
}
/****************************************************************************************************************************
*函数名称：Load_Jpg()
*参数    :
*返回值  :
*功能描述:用于JPG解码并显示到TFT液晶屏，外部调用即可进行解码显示
******************************************************************************************************************************/
void Load_Jpg (
	FIL *fp,		/* Pointer to the open file object to load */
	void *work,		/* Pointer to the working buffer (must be 4-byte aligned) */
	UINT sz_work	/* Size of the working buffer (must be power of 2) */
)
{
	JDEC jd;		/* Decoder object (124 bytes) */
	JRESULT rc;
	BYTE scale;

	/*载入需要解码的JPG数据 */
	rc = jd_prepare(&jd, JpgDecoderRead, work, sz_work, fp);
	if (rc == JDR_OK) 
	{

		/*判断JPG的缩放，以适应屏幕大小*/
		for (scale = 0; scale < 3; scale++) 
		{
			if ((jd.width >> scale) <= LCD_XMAX && (jd.height >> scale) <= LCD_YMAX) break;
		}

		/* 开始解码JPG文件并显示到液晶屏 */
		rc = jd_decomp(&jd, Jpg_Display, scale);	/* Start to decompress */

	} else {
		LCD_SetColors(RED,WHITE);
    LCD_DrawString(30,140,"prepare ERR");
	}
}





