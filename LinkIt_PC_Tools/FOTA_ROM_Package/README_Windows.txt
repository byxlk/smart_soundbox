NOTE: Run this FOTA (Firmware Over the Air) tool on Microsoft Windows.

The steps to run the FOTA tool are as follows:
1. Put your new binary in "_Load" folder.

2. Configure FOTARomPacker.ini file in "_ini" folder.
    a) In section "general setting", set your load path in "Base_Directory".
    b) One flash partition is mapping to one description section: 
        1) set binary file name in "File".
        2) set the start address of this bin on flash in "Start_Address".
        3) set the size of partition where your .bin want to burn in "Partition".
        4) set if the .bin is compressed or not, "true" means to apply lzma compress, other choice means no compression is needed.

3. Click "gen_image.bat", find the generated fota package file named "image.bin" in "_Output folder".


