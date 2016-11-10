;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


;  AREA _image, CODE, READONLY			;Need minus address with  one 
	AREA _image, DATA, READONLY

    EXPORT  u32AudioDataBegin
    EXPORT  u32AudioDataEnd
    EXPORT  u32AudioDataBegin1
    EXPORT  u32AudioDataEnd1

	ALIGN   4 
    
u32AudioDataBegin
    INCBIN ON.PCM
u32AudioDataEnd        
u32AudioDataBegin1
    INCBIN ON.PCM
u32AudioDataEnd1      
    
    END