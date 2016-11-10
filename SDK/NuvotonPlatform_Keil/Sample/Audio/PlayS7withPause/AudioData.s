;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


;    AREA _image, CODE, READONLY	   	;Address: AudioDataBegin-1
	AREA _image, DATA, READONLY	 		;Address: no need -1

    EXPORT  AudioDataBegin
    EXPORT  AudioDataEnd


	ALIGN   4 
    
AudioDataBegin
	INCBIN VP\SampleSound
AudioDataEnd
    
    
    END