# Programming guide
#### Required software
You will need to have the following software installed
* [Arduino IDE](https://www.arduino.cc/en/Main/software)
* [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)

#### Install STM32 cores
1. In the Arduino IDE goto 'File' and then 'Preferences'
2. Add the following link to the 'Additional Boards Manager URLs' field:
https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
3. Click 'Ok' to close the menu
4. Goto 'Tools', then 'Boards' and then click on 'Boards Manager'
5. Select 'STM32' Cores and click 'Install'
6. STM32 boards should now show up in the board selection menu

#### Board settings
In the tools menu make sure your board settings match this image
![Board settings](https://github.com/edwardatki/Antdrive/blob/master/documentation/programming_settings.png)

#### Programming
1. Connect and FTDI and select the appropriate COM port
2. Press the FTDI to the corresponding set of pads on the board
3. Press upload
4. Keep pressing the FTDI to the board until the upload it complete
