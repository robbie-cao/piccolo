;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _image, CODE, READONLY

    EXPORT  u32AudioDataBegin
    EXPORT  u32AudioDataEnd

	ALIGN   4 
    
u32AudioDataBegin
    INCBIN Sound\Carnon.PCM
u32AudioDataEnd        
    
    
    END