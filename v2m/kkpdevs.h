#ifndef _KKPDEVSH
#define  _KKPDEVSH

/*-----------------------------------------------Константы задания ограничений*/
#define MAXKP		255	/* Максимальное количество КП*/
#define MAXPORTS	128	/* Максимальное количество портов*/
/*----------------------------------------------------------------Типы каналов*/
/* Port types*/
#define CH_RS232	0
#define CH_HART		1
#define CH_DIALUP	2
#define CH_LLINE        3
#define CH_RADIO	4
#define CH_NETBIOS     	8
#define CH_TCP_SDTM   	9
#define CH_TCP         	9
#define CH_TCP_CLNT     10
#define CH_TCP_SRV      11
#define CH_POP          12
#define CH_PCSLOT  	13
#define CH_FILE         14
#define CH_EXTERNAL	15
/*----------------------------------------------------Настройки портов каналов*/
#define INUSE_FL        1
#define CTS_RTS_FL      2
#define RS485_FL      	4
/*	Block types*/
#define TI	0
#define TS	1
#define TU	2
#define TII	3
#define DIAG	4
#define TI2	5
#define TR	6
#define STAT	7
#define STIME	0xD     /*Signal time mark*/
#define SQUALITY 0xE     /*Signal quality*/
#define BERR	0xF	/*Block Error*/
/*----------------------------------------------------------Типы КП телемеханики
   Протокол:	№     Наменование	IN	OUT
-------------------------------------------------------------------------
		  1 - ППДИ1		КП/ПУ	КП/ПУ
		  2 - "Гранит"		ПУ	ПУ
		  3 - "МКТ-2"		ПУ	КП
                  4 - "ВРТФ-3"		ПУ	ПУ
                  5 - "ЛИСНА-Ч"		ПУ	ПУ
                  6 - "MKT-3"		ПУ	ПУ
                106 - "MKT-3"		КП	КП
                  7 - "TМ-800а"		ПУ	КП
                  8 - "TМ-800в"		ПУ	ПУ
		  9 - "TM-120"		ПУ	ПУ
		 10 - "TM-320"		ПУ	ПУ
		 11 - "TM-512"		ПУ	ПУ
		 12 - "УТК-1"		ПУ	КП
		 13 - "УТС-8"		ПУ	КП
		 14 - "УТМ-7"		ПУ	КП
		 15 - "РПТ-РПТ"		КП/ПУ	КП/ПУ
		 16 - "КА-96"		ПУ	ПУ
------------------------------------------------------------------------------*/
#define OCTAGON         1
#define OCTAGONRTU      101
#define GRANIT          2
#define GRANITRTU       102
#define MKT2            3
#define MKT2RTU         103
#define VRTF3           4
#define VRTF3RTU        104
#define LISNA           5
#define LISNARTU        105
#define MKT3            6
#define MKT3RTU         106
#define TM800A          7
#define TM800ARTU       107
#define TM800V          8
#define TM800VRTU       108
#define TM120           9
#define TM120RTU        109
#define TM320           10
#define TM320RTU        110
#define TM512           11
#define TM512RTU        111
#define UTK1            12
#define UTK1RTU         112
#define UTS1            13
#define UTS1RTU         113
#define UTM7            14
#define UTM7RTU         114
#define RPTRPT          15
#define RPTRPTRTU       115
#define KA96          	16
/*---------------------------*/
#define i7000		30
#define i7000CHK        31
#define i7060           32
#define i7060CHK        33
#define NMEA0183        39
#define iENCODER300	40
#define MODBUSTCP	44
#define MODBUSRTU	45
#define MODBUSASCII	46
#define MODBUSRTU2	47
#define BTU16		48
#define MB25		50
#define MBCLOCK		51
#define SPRUT		52
#define MBKPR		53
#define M870_5_101	60
#define M870_5_104	62
#define S870_5_101	160
#define S870_5_101EXT	161
#define S870_5_104	162
#endif
