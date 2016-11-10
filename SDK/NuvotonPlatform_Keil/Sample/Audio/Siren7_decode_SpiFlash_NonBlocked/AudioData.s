;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _image, CODE, READONLY

    EXPORT  AudioDataBegin
    EXPORT  AudioDataEnd


	ALIGN   4 
    
AudioDataBegin
	INCBIN VP\SampleSound
AudioDataEnd
    
    
    END