#ifndef __BMP_H__
#define __BMP_H__
#include "sys.h"
#include "fat.h"
//BMP信息头
typedef __packed struct
{
    DWORD biSize ;		   //说明BITMAPINFOHEADER结构所需要的字数。
    LONG  biWidth ;		   //说明图象的宽度，以象素为单位 
    LONG  biHeight ;	   //说明图象的高度，以象素为单位 
    WORD  biPlanes ;	   //为目标设备说明位面数，其值将总是被设为1 
    WORD  biBitCount ;	   //说明比特数/象素，其值为1、4、8、16、24、或32
    DWORD biCompression ;  //说明图象数据压缩的类型。其值可以是下述值之一：
	//BI_RGB：没有压缩；
	//BI_RLE8：每个象素8比特的RLE压缩编码，压缩格式由2字节组成(重复象素计数和颜色索引)；  
    //BI_RLE4：每个象素4比特的RLE压缩编码，压缩格式由2字节组成
  	//BI_BITFIELDS：每个象素的比特由指定的掩码决定。
    DWORD biSizeImage ;//说明图象的大小，以字节为单位。当用BI_RGB格式时，可设置为0  
    LONG  biXPelsPerMeter ;//说明水平分辨率，用象素/米表示
    LONG  biYPelsPerMeter ;//说明垂直分辨率，用象素/米表示
    DWORD biClrUsed ;	   //说明位图实际使用的彩色表中的颜色索引数
    DWORD biClrImportant ; //说明对图象显示有重要影响的颜色索引的数目，如果是0，表示都重要。 
}BITMAPINFOHEADER ;
//BMP头文件
typedef __packed struct
{
    WORD  bfType ;     //文件标志.只对'BM',用来识别BMP位图类型
    DWORD bfSize ;	   //文件大小,占四个字节
    WORD  bfReserved1 ;//保留
    WORD  bfReserved2 ;//保留
    DWORD bfOffBits ;  //从文件开始到位图数据(bitmap data)开始之间的的偏移量
}BITMAPFILEHEADER ;
//彩色表 
typedef __packed struct 
{
    BYTE rgbBlue ;    //指定蓝色强度
    BYTE rgbGreen ;	  //指定绿色强度 
    BYTE rgbRed ;	  //指定红色强度 
    BYTE rgbReserved ;//保留，设置为0 
}RGBQUAD ;
//位图信息头
typedef __packed struct
{ 
	BITMAPFILEHEADER bmfHeader;
	BITMAPINFOHEADER bmiHeader;  
	//RGBQUAD bmiColors[256];  
}BITMAPINFO; 

typedef RGBQUAD * LPRGBQUAD;//彩色表  

//图象数据压缩的类型
#define BI_RGB       0L
#define BI_RLE8      1L
#define BI_RLE4      2L
#define BI_BITFIELDS 3L 

#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DRI   0xdd
#define M_APP0  0xe0  	  
//反Z字形编码表
const int Zig_Zag[8][8]={{0,1,5,6,14,15,27,28},
					  	  {2,4,7,13,16,26,29,42},
						  {3,8,12,17,25,30,41,43},
						  {9,11,18,24,31,40,44,53},
						  {10,19,23,32,39,45,52,54},
						  {20,22,33,38,46,51,55,60},
						  {21,34,37,47,50,56,59,61},
						  {35,36,48,49,57,58,62,63}
						 };
const BYTE And[9]={0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff};

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */
												 
	  					    		  
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
	   
//宏定义					   
#define WIDTHBYTES(i)    ((i+31)/32*4)//??????????
#define PI 3.1415926535
//函数返回值定义
#define FUNC_OK 0
#define FUNC_MEMORY_ERROR 1
#define FUNC_FILE_ERROR 2
#define FUNC_FORMAT_ERROR 3
//用到的外部参数
//extern FileInfoStruct F_Info[8];//文件结构体
//extern unsigned int POINT_COLOR;//画笔颜色				       
//////////////////////////////////////////////////
//BMP解码函数
BOOL BmpDecode(FileInfoStruct *BmpFileName);
//////////////////////////////////////////////////
//JPEG 解码函数				 
int  InitTag(void);
void InitTable(void);						   //初始化数据表
int  Decode(void);							   //解码
int  DecodeMCUBlock(void);
int  HufBlock(BYTE dchufindex,BYTE achufindex);//哈夫曼解码
int  DecodeElement(void);					   //解码一个像素
void IQtIZzMCUComponent(short flag);		   //反量化
void IQtIZzBlock(short  *s ,short * d,short flag);
void GetYUV(short flag);		 //色彩转换的实现,得到色彩空间数据
void StoreBuffer(void);

BYTE ReadByte(void);             //从文件里面读取一个字节出来
void Initialize_Fast_IDCT(void); //初始化反离散傅立叶变换
void Fast_IDCT(int * block);	 //快速反离散傅立叶变换
void idctrow(int * blk);
void idctcol(int * blk);  
////对缓冲区数据进行移动处理,使操作SD卡就像操作sram一样
unsigned int P_Cal(unsigned char*pc);		    
#endif








