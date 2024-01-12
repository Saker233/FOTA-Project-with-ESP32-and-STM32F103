# FOTA-Project-with-ESP32-and-STM32F103

In this project I impelmented UART Bootloader which takes a the .hex file from ESP32 wifi module

The ESP32 takes the .hex file from Google firebase over the internet

ESP32 "Download" the code and store it in its flash memory and after that it communciate with the STM32 over UART to trasnfer the .hex file after parsing it


![digram](https://github.com/Saker233/FOTA-Project-with-ESP32-and-STM32F103/assets/130178079/b22637ac-e6cc-40b6-8415-7e85728a7499)

the STM32 Bootloader receive this code and falsh it in its falsh memory

After that it gives a reset signal and start over from the new .hex file

And by this I implemented FOTA
# FOTA-Project-with-ESP32-and-STM32F103
# FOTA-Project-with-ESP32-and-STM32F103
