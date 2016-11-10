;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _image, DATA, READONLY

    EXPORT  loaderImageBase
    EXPORT  loaderImageLimit
    
    ALIGN   4
        
loaderImageBase
	IF :DEF:SEMIHOST
    INCBIN .\obj\Smpl_DrvFMC_SimpleLD_semihost.bin
	ELSE
    INCBIN .\obj\Smpl_DrvFMC_SimpleLD.bin
	ENDIF
loaderImageLimit

    
    END