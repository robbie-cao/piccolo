SET RVCT40INC=C:\Keil_ARM\ARM\RV31\INC
SET RVCT40LIB=C:\Keil_ARM\ARM\RV31\LIB
SET CPU_TYPE=Cortex-M0
SET CPU_VENDOR=ARM
SET UV2_TARGET=Smpl_RecPlay
SET CPU_CLOCK=0x00000000
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\core_cm0.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\startup_isd9xx.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\system_isd9xx.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\main.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\audfile.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\isd9_audout.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\isd9_audin.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\ffx_io_flash.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\menu.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\hw.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\lcd_uc1601.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\drvsys.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\drvgpio.__i"
"C:\Keil_ARM\ARM\BIN40\ArmCC" --Via ".\obj\drvpdma.__i"
"C:\Keil_ARM\ARM\BIN40\ArmLink" --Via ".\obj\Smpl_RecPlay.lnp"
C:\Keil_ARM\ARM\BIN40\fromelf.exe ".\obj\Smpl_RecPlay.axf" --i32combined --output ".\obj\Smpl_RecPlay.hex"
fromelf --bin ".\obj\Smpl_RecPlay.axf" --output ".\Smpl_RecPlay.bin"
