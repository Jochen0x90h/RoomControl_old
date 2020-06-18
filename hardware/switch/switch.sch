EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_ST_STM32F0:STM32F042F6Px U2
U 1 1 5EC14E6C
P 5950 5150
F 0 "U2" H 5500 5800 50  0000 C CNN
F 1 "STM32F042F6Px" H 6200 5800 50  0000 C CNN
F 2 "Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm" H 5450 4450 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00105814.pdf" H 5950 5150 50  0001 C CNN
	1    5950 5150
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push_SPDT SW1
U 1 1 5EC1697E
P 9200 2700
F 0 "SW1" H 9200 2985 50  0000 C CNN
F 1 "D2F-01" H 9200 2894 50  0000 C CNN
F 2 "RoomControl:Omron_D2F-01" H 9200 2700 50  0001 C CNN
F 3 "~" H 9200 2700 50  0001 C CNN
	1    9200 2700
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push_SPDT SW2
U 1 1 5EC18290
P 9200 3450
F 0 "SW2" H 9200 3735 50  0000 C CNN
F 1 "D2F-01" H 9200 3644 50  0000 C CNN
F 2 "RoomControl:Omron_D2F-01" H 9200 3450 50  0001 C CNN
F 3 "~" H 9200 3450 50  0001 C CNN
	1    9200 3450
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push_SPDT SW3
U 1 1 5EC18D00
P 10100 2700
F 0 "SW3" H 10100 2985 50  0000 C CNN
F 1 "D2F-01" H 10100 2894 50  0000 C CNN
F 2 "RoomControl:Omron_D2F-01" H 10100 2700 50  0001 C CNN
F 3 "~" H 10100 2700 50  0001 C CNN
	1    10100 2700
	1    0    0    -1  
$EndComp
$Comp
L Switch:SW_Push_SPDT SW4
U 1 1 5EC19284
P 10100 3450
F 0 "SW4" H 10100 3735 50  0000 C CNN
F 1 "D2F-01" H 10100 3644 50  0000 C CNN
F 2 "RoomControl:Omron_D2F-01" H 10100 3450 50  0001 C CNN
F 3 "~" H 10100 3450 50  0001 C CNN
	1    10100 3450
	1    0    0    -1  
$EndComp
$Comp
L Relay:G6SU-2 K1
U 1 1 5EC216D8
P 4050 1350
F 0 "K1" V 3283 1350 50  0000 C CNN
F 1 "G6SU-2" V 3374 1350 50  0000 C CNN
F 2 "RoomControl:Omron_G6SU-2" H 4050 1350 50  0001 L CNN
F 3 "http://omronfs.omron.com/en_US/ecb/products/pdf/en-g6s.pdf" H 4050 1350 50  0001 C CNN
	1    4050 1350
	0    1    1    0   
$EndComp
$Comp
L Relay:G6SU-2 K2
U 1 1 5EC22979
P 4050 3000
F 0 "K2" V 3283 3000 50  0000 C CNN
F 1 "G6SU-2" V 3374 3000 50  0000 C CNN
F 2 "RoomControl:Omron_G6SU-2" H 4050 3000 50  0001 L CNN
F 3 "http://omronfs.omron.com/en_US/ecb/products/pdf/en-g6s.pdf" H 4050 3000 50  0001 C CNN
	1    4050 3000
	0    1    1    0   
$EndComp
$Comp
L Relay:G6SU-2 K3
U 1 1 5EC23816
P 5800 1350
F 0 "K3" V 5033 1350 50  0000 C CNN
F 1 "G6SU-2" V 5124 1350 50  0000 C CNN
F 2 "RoomControl:Omron_G6SU-2" H 5800 1350 50  0001 L CNN
F 3 "http://omronfs.omron.com/en_US/ecb/products/pdf/en-g6s.pdf" H 5800 1350 50  0001 C CNN
	1    5800 1350
	0    1    1    0   
$EndComp
$Comp
L Relay:G6SU-2 K4
U 1 1 5EC24B90
P 5800 3050
F 0 "K4" V 5033 3050 50  0000 C CNN
F 1 "G6SU-2" V 5124 3050 50  0000 C CNN
F 2 "RoomControl:Omron_G6SU-2" H 5800 3050 50  0001 L CNN
F 3 "http://omronfs.omron.com/en_US/ecb/products/pdf/en-g6s.pdf" H 5800 3050 50  0001 C CNN
	1    5800 3050
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x07 J1
U 1 1 5EC0B47C
P 7350 1400
F 0 "J1" H 7350 1800 50  0000 C CNN
F 1 "Conn_01x07" V 7450 1400 50  0000 C CNN
F 2 "RoomControl:Wago_236-407" H 7350 1400 50  0001 C CNN
F 3 "~" H 7350 1400 50  0001 C CNN
	1    7350 1400
	-1   0    0    -1  
$EndComp
Text GLabel 2100 1700 2    50   Input ~ 0
AX
Text GLabel 2100 1500 2    50   Input ~ 0
AY
Text GLabel 2100 1400 2    50   Input ~ 0
AZ
Text GLabel 2100 1600 2    50   Input ~ 0
BX
Text GLabel 2100 1300 2    50   Input ~ 0
BY
Text GLabel 2100 1200 2    50   Input ~ 0
BZ
Text GLabel 4350 950  2    50   Input ~ 0
AX
Text GLabel 3750 950  0    50   Input ~ 0
AY
Text GLabel 4350 2600 2    50   Input ~ 0
AZ
Text GLabel 6100 950  2    50   Input ~ 0
BX
Text GLabel 5500 950  0    50   Input ~ 0
BY
Text GLabel 6100 2650 2    50   Input ~ 0
BZ
$Comp
L power:GND #PWR0101
U 1 1 5EC4C020
P 1750 2650
F 0 "#PWR0101" H 1750 2400 50  0001 C CNN
F 1 "GND" H 1755 2477 50  0000 C CNN
F 2 "" H 1750 2650 50  0001 C CNN
F 3 "" H 1750 2650 50  0001 C CNN
	1    1750 2650
	1    0    0    -1  
$EndComp
Connection ~ 1750 2650
Wire Wire Line
	1750 2650 1850 2650
$Comp
L power:GND #PWR0102
U 1 1 5EC4D16A
P 2950 5550
F 0 "#PWR0102" H 2950 5300 50  0001 C CNN
F 1 "GND" H 2955 5377 50  0000 C CNN
F 2 "" H 2950 5550 50  0001 C CNN
F 3 "" H 2950 5550 50  0001 C CNN
	1    2950 5550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0103
U 1 1 5EC4D5B9
P 5750 5950
F 0 "#PWR0103" H 5750 5700 50  0001 C CNN
F 1 "GND" H 5755 5777 50  0000 C CNN
F 2 "" H 5750 5950 50  0001 C CNN
F 3 "" H 5750 5950 50  0001 C CNN
	1    5750 5950
	1    0    0    -1  
$EndComp
Text GLabel 7550 1300 2    50   Input ~ 0
L
Text GLabel 7550 1500 2    50   Input ~ 0
L
Text GLabel 7550 1100 2    50   Input ~ 0
KA1
Text GLabel 7550 1600 2    50   Input ~ 0
KB0
Text GLabel 7550 1700 2    50   Input ~ 0
KB1
Text GLabel 7550 1400 2    50   Input ~ 0
PE
Text GLabel 5500 3050 0    50   Input ~ 0
KB1
Text GLabel 5500 3450 0    50   Input ~ 0
KB1
Text GLabel 5500 1350 0    50   Input ~ 0
KB0
Text GLabel 5500 1750 0    50   Input ~ 0
KB0
Text GLabel 3750 3000 0    50   Input ~ 0
KA1
Text GLabel 3750 3400 0    50   Input ~ 0
KA1
Text GLabel 3750 1350 0    50   Input ~ 0
KA0
Text GLabel 3750 1750 0    50   Input ~ 0
KA0
Text GLabel 4350 1250 2    50   Input ~ 0
PE
Text GLabel 4350 1650 2    50   Input ~ 0
PE
Text GLabel 4350 2900 2    50   Input ~ 0
PE
Text GLabel 4350 3300 2    50   Input ~ 0
PE
Text GLabel 6100 1250 2    50   Input ~ 0
PE
Text GLabel 6100 1650 2    50   Input ~ 0
PE
Text GLabel 6100 2950 2    50   Input ~ 0
PE
Text GLabel 6100 3350 2    50   Input ~ 0
PE
$Comp
L power:+3V3 #PWR0106
U 1 1 5EC5052A
P 5750 4450
F 0 "#PWR0106" H 5750 4300 50  0001 C CNN
F 1 "+3V3" H 5765 4623 50  0000 C CNN
F 2 "" H 5750 4450 50  0001 C CNN
F 3 "" H 5750 4450 50  0001 C CNN
	1    5750 4450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5850 4450 5750 4450
Connection ~ 5750 4450
$Comp
L power:+3V3 #PWR0107
U 1 1 5EC516D8
P 1500 850
F 0 "#PWR0107" H 1500 700 50  0001 C CNN
F 1 "+3V3" H 1515 1023 50  0000 C CNN
F 2 "" H 1500 850 50  0001 C CNN
F 3 "" H 1500 850 50  0001 C CNN
	1    1500 850 
	1    0    0    -1  
$EndComp
$Comp
L power:+12V #PWR0108
U 1 1 5EC52118
P 1800 850
F 0 "#PWR0108" H 1800 700 50  0001 C CNN
F 1 "+12V" H 1815 1023 50  0000 C CNN
F 2 "" H 1800 850 50  0001 C CNN
F 3 "" H 1800 850 50  0001 C CNN
	1    1800 850 
	1    0    0    -1  
$EndComp
Wire Wire Line
	1700 850  1800 850 
$Comp
L power:GND #PWR0111
U 1 1 5EC54481
P 9400 2800
F 0 "#PWR0111" H 9400 2550 50  0001 C CNN
F 1 "GND" H 9405 2627 50  0000 C CNN
F 2 "" H 9400 2800 50  0001 C CNN
F 3 "" H 9400 2800 50  0001 C CNN
	1    9400 2800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0112
U 1 1 5EC54A53
P 9400 3550
F 0 "#PWR0112" H 9400 3300 50  0001 C CNN
F 1 "GND" H 9405 3377 50  0000 C CNN
F 2 "" H 9400 3550 50  0001 C CNN
F 3 "" H 9400 3550 50  0001 C CNN
	1    9400 3550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0113
U 1 1 5EC54F6A
P 10300 2800
F 0 "#PWR0113" H 10300 2550 50  0001 C CNN
F 1 "GND" H 10305 2627 50  0000 C CNN
F 2 "" H 10300 2800 50  0001 C CNN
F 3 "" H 10300 2800 50  0001 C CNN
	1    10300 2800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0114
U 1 1 5EC5534F
P 10300 3550
F 0 "#PWR0114" H 10300 3300 50  0001 C CNN
F 1 "GND" H 10305 3377 50  0000 C CNN
F 2 "" H 10300 3550 50  0001 C CNN
F 3 "" H 10300 3550 50  0001 C CNN
	1    10300 3550
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0115
U 1 1 5EC5898C
P 9400 2600
F 0 "#PWR0115" H 9400 2450 50  0001 C CNN
F 1 "+3V3" H 9415 2773 50  0000 C CNN
F 2 "" H 9400 2600 50  0001 C CNN
F 3 "" H 9400 2600 50  0001 C CNN
	1    9400 2600
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0116
U 1 1 5EC58F68
P 9400 3350
F 0 "#PWR0116" H 9400 3200 50  0001 C CNN
F 1 "+3V3" H 9415 3523 50  0000 C CNN
F 2 "" H 9400 3350 50  0001 C CNN
F 3 "" H 9400 3350 50  0001 C CNN
	1    9400 3350
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0117
U 1 1 5EC5955B
P 10300 2600
F 0 "#PWR0117" H 10300 2450 50  0001 C CNN
F 1 "+3V3" H 10315 2773 50  0000 C CNN
F 2 "" H 10300 2600 50  0001 C CNN
F 3 "" H 10300 2600 50  0001 C CNN
	1    10300 2600
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0118
U 1 1 5EC599C7
P 10300 3350
F 0 "#PWR0118" H 10300 3200 50  0001 C CNN
F 1 "+3V3" H 10315 3523 50  0000 C CNN
F 2 "" H 10300 3350 50  0001 C CNN
F 3 "" H 10300 3350 50  0001 C CNN
	1    10300 3350
	1    0    0    -1  
$EndComp
Text GLabel 9000 2700 0    50   Input ~ 0
SWA0
Text GLabel 9000 3450 0    50   Input ~ 0
SWA1
Text GLabel 9900 3450 0    50   Input ~ 0
SWB1
Text GLabel 9900 2700 0    50   Input ~ 0
SWB0
Text GLabel 7550 1200 2    50   Input ~ 0
KA0
Text GLabel 6100 3150 2    50   Input ~ 0
L
Text GLabel 6100 3550 2    50   Input ~ 0
L
Text GLabel 6100 1850 2    50   Input ~ 0
L
Text GLabel 6100 1450 2    50   Input ~ 0
L
Text GLabel 4350 3500 2    50   Input ~ 0
L
Text GLabel 4350 3100 2    50   Input ~ 0
L
Text GLabel 4350 1850 2    50   Input ~ 0
L
Text GLabel 4350 1450 2    50   Input ~ 0
L
$Comp
L power:GND #PWR0120
U 1 1 5EC5C24A
P 1000 5100
F 0 "#PWR0120" H 1000 4850 50  0001 C CNN
F 1 "GND" H 1005 4927 50  0000 C CNN
F 2 "" H 1000 5100 50  0001 C CNN
F 3 "" H 1000 5100 50  0001 C CNN
	1    1000 5100
	1    0    0    -1  
$EndComp
$Comp
L power:+12V #PWR0121
U 1 1 5EC5C5A0
P 1500 4800
F 0 "#PWR0121" H 1500 4650 50  0001 C CNN
F 1 "+12V" H 1515 4973 50  0000 C CNN
F 2 "" H 1500 4800 50  0001 C CNN
F 3 "" H 1500 4800 50  0001 C CNN
	1    1500 4800
	1    0    0    -1  
$EndComp
Text GLabel 2600 5150 0    50   Input ~ 0
LIN
Text GLabel 1000 4900 2    50   Input ~ 0
LIN
$Comp
L Device:C C10
U 1 1 5EC6CB33
P 5750 7350
F 0 "C10" H 5750 7450 50  0000 L CNN
F 1 "C" H 5750 7250 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 5788 7200 50  0001 C CNN
F 3 "~" H 5750 7350 50  0001 C CNN
	1    5750 7350
	1    0    0    -1  
$EndComp
$Comp
L Device:C C11
U 1 1 5EC6D041
P 6050 7350
F 0 "C11" H 6050 7450 50  0000 L CNN
F 1 "C" H 6050 7250 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 6088 7200 50  0001 C CNN
F 3 "~" H 6050 7350 50  0001 C CNN
	1    6050 7350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0123
U 1 1 5EC6D63B
P 5750 7500
F 0 "#PWR0123" H 5750 7250 50  0001 C CNN
F 1 "GND" H 5755 7327 50  0000 C CNN
F 2 "" H 5750 7500 50  0001 C CNN
F 3 "" H 5750 7500 50  0001 C CNN
	1    5750 7500
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0124
U 1 1 5EC6DB6C
P 6050 7500
F 0 "#PWR0124" H 6050 7250 50  0001 C CNN
F 1 "GND" H 6055 7327 50  0000 C CNN
F 2 "" H 6050 7500 50  0001 C CNN
F 3 "" H 6050 7500 50  0001 C CNN
	1    6050 7500
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0126
U 1 1 5EC6DE4D
P 5750 7200
F 0 "#PWR0126" H 5750 7050 50  0001 C CNN
F 1 "+3V3" H 5765 7373 50  0000 C CNN
F 2 "" H 5750 7200 50  0001 C CNN
F 3 "" H 5750 7200 50  0001 C CNN
	1    5750 7200
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0127
U 1 1 5EC6E2B1
P 6050 7200
F 0 "#PWR0127" H 6050 7050 50  0001 C CNN
F 1 "+3V3" H 6065 7373 50  0000 C CNN
F 2 "" H 6050 7200 50  0001 C CNN
F 3 "" H 6050 7200 50  0001 C CNN
	1    6050 7200
	1    0    0    -1  
$EndComp
Text GLabel 6550 4650 2    50   Input ~ 0
SWA0
Text GLabel 6550 4750 2    50   Input ~ 0
SWA1
Text GLabel 6550 4850 2    50   Input ~ 0
SWB0
Text GLabel 6550 4950 2    50   Input ~ 0
SWB1
Text GLabel 6550 5350 2    50   Input ~ 0
HB_MOSI
Text GLabel 6550 5250 2    50   Input ~ 0
HB_MISO
Text GLabel 6550 5150 2    50   Input ~ 0
HB_SCK
Text GLabel 6550 5050 2    50   Input ~ 0
HB_SS
$Comp
L power:GND #PWR0129
U 1 1 5EC7AF0D
P 7600 5450
F 0 "#PWR0129" H 7600 5200 50  0001 C CNN
F 1 "GND" H 7605 5277 50  0000 C CNN
F 2 "" H 7600 5450 50  0001 C CNN
F 3 "" H 7600 5450 50  0001 C CNN
	1    7600 5450
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0130
U 1 1 5EC7B239
P 8000 5350
F 0 "#PWR0130" H 8000 5200 50  0001 C CNN
F 1 "+3V3" H 8015 5523 50  0000 C CNN
F 2 "" H 8000 5350 50  0001 C CNN
F 3 "" H 8000 5350 50  0001 C CNN
	1    8000 5350
	1    0    0    -1  
$EndComp
Text GLabel 6550 5650 2    50   Input ~ 0
SWDIO
Text GLabel 6550 5750 2    50   Input ~ 0
SWCLK
Text GLabel 7600 5150 2    50   Input ~ 0
SWCLK
Text GLabel 7600 5250 2    50   Input ~ 0
SWDIO
Text GLabel 6550 5450 2    50   Input ~ 0
LIN_TX
Text GLabel 6550 5550 2    50   Input ~ 0
LIN_RX
Text GLabel 3400 5050 2    50   Input ~ 0
LIN_RX
Text GLabel 3400 5150 2    50   Input ~ 0
LIN_TX
Text GLabel 1150 1700 0    50   Input ~ 0
HB_SS
Text GLabel 1150 1400 0    50   Input ~ 0
HB_SCK
Text GLabel 1150 1600 0    50   Input ~ 0
HB_MISO
Text GLabel 1150 1500 0    50   Input ~ 0
HB_MOSI
$Comp
L Jumper:SolderJumper_2_Open JP1
U 1 1 5ECA807C
P 9200 4550
F 0 "JP1" H 9200 4755 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 9200 4664 50  0000 C CNN
F 2 "RoomControl:Jumper2Triangular" H 9200 4550 50  0001 C CNN
F 3 "~" H 9200 4550 50  0001 C CNN
	1    9200 4550
	1    0    0    -1  
$EndComp
$Comp
L Jumper:SolderJumper_2_Open JP2
U 1 1 5ECAAA17
P 9200 5000
F 0 "JP2" H 9200 5205 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 9200 5114 50  0000 C CNN
F 2 "RoomControl:Jumper2Triangular" H 9200 5000 50  0001 C CNN
F 3 "~" H 9200 5000 50  0001 C CNN
	1    9200 5000
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0134
U 1 1 5ECAAF06
P 9350 4550
F 0 "#PWR0134" H 9350 4300 50  0001 C CNN
F 1 "GND" H 9355 4377 50  0000 C CNN
F 2 "" H 9350 4550 50  0001 C CNN
F 3 "" H 9350 4550 50  0001 C CNN
	1    9350 4550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0135
U 1 1 5ECAB384
P 9350 5000
F 0 "#PWR0135" H 9350 4750 50  0001 C CNN
F 1 "GND" H 9355 4827 50  0000 C CNN
F 2 "" H 9350 5000 50  0001 C CNN
F 3 "" H 9350 5000 50  0001 C CNN
	1    9350 5000
	1    0    0    -1  
$EndComp
Text GLabel 9050 4550 0    50   Input ~ 0
JPA
Text GLabel 9050 5000 0    50   Input ~ 0
JPB
Text GLabel 5350 5650 0    50   Input ~ 0
JPA
Text GLabel 5350 5750 0    50   Input ~ 0
JPB
Text GLabel 7600 5050 2    50   Input ~ 0
RESET
$Comp
L Connector_Generic:Conn_01x03 J2
U 1 1 5ED31000
P 800 5000
F 0 "J2" H 800 5200 50  0000 C CNN
F 1 "Conn_01x03" V 900 5000 50  0000 C CNN
F 2 "RoomControl:Wago_236-403" H 800 5000 50  0001 C CNN
F 3 "~" H 800 5000 50  0001 C CNN
	1    800  5000
	-1   0    0    -1  
$EndComp
$Comp
L power:+12V #PWR0104
U 1 1 5ED34A2B
P 2150 4850
F 0 "#PWR0104" H 2150 4700 50  0001 C CNN
F 1 "+12V" H 2165 5023 50  0000 C CNN
F 2 "" H 2150 4850 50  0001 C CNN
F 3 "" H 2150 4850 50  0001 C CNN
	1    2150 4850
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0105
U 1 1 5ED34F20
P 3850 4850
F 0 "#PWR0105" H 3850 4700 50  0001 C CNN
F 1 "+3V3" H 3865 5023 50  0000 C CNN
F 2 "" H 3850 4850 50  0001 C CNN
F 3 "" H 3850 4850 50  0001 C CNN
	1    3850 4850
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J3
U 1 1 5ED3867E
P 7400 5250
F 0 "J3" H 7400 5550 50  0000 C CNN
F 1 "Conn_01x05" V 7500 5250 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 7400 5250 50  0001 C CNN
F 3 "~" H 7400 5250 50  0001 C CNN
	1    7400 5250
	-1   0    0    -1  
$EndComp
$Comp
L Device:D D1
U 1 1 5ED3F30E
P 1350 5000
F 0 "D1" H 1400 4900 50  0000 C CNN
F 1 "D" H 1300 4900 50  0000 C CNN
F 2 "Diode_SMD:D_1206_3216Metric_Castellated" H 1350 5000 50  0001 C CNN
F 3 "~" H 1350 5000 50  0001 C CNN
	1    1350 5000
	-1   0    0    1   
$EndComp
$Comp
L Device:C C1
U 1 1 5ED41EBC
P 2150 5100
F 0 "C1" H 2150 5200 50  0000 L CNN
F 1 "10u/25V/7XR" V 2000 4800 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 2188 4950 50  0001 C CNN
F 3 "~" H 2150 5100 50  0001 C CNN
	1    2150 5100
	1    0    0    -1  
$EndComp
$Comp
L Device:C C2
U 1 1 5ED42734
P 3850 5100
F 0 "C2" H 3850 5200 50  0000 L CNN
F 1 "22u" H 3850 5000 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 3888 4950 50  0001 C CNN
F 3 "~" H 3850 5100 50  0001 C CNN
	1    3850 5100
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 4950 2150 4850
Wire Wire Line
	2150 4850 2600 4850
Connection ~ 2150 4850
Wire Wire Line
	3400 4850 3850 4850
Wire Wire Line
	3850 4950 3850 4850
Connection ~ 3850 4850
Wire Wire Line
	3850 5250 3850 5550
Wire Wire Line
	3850 5550 3050 5550
Wire Wire Line
	2150 5550 2150 5250
$Comp
L RoomControl:NCV7428MW U3
U 1 1 5ED69708
P 3000 5150
F 0 "U3" H 3000 5715 50  0000 C CNN
F 1 "NCV7428MW3" H 3000 5624 50  0000 C CNN
F 2 "Package_DFN_QFN:DFN-8-1EP_3x3mm_P0.65mm_EP1.55x2.4mm" H 3000 5150 50  0001 C CNN
F 3 "" H 3000 5150 50  0001 C CNN
	1    3000 5150
	1    0    0    -1  
$EndComp
Connection ~ 2950 5550
Wire Wire Line
	2950 5550 2150 5550
Connection ~ 3050 5550
Wire Wire Line
	2950 5550 3050 5550
$Comp
L RoomControl:NCV77xx U1
U 1 1 5EE87466
P 1650 1750
F 0 "U1" H 1300 2600 50  0000 C CNN
F 1 "NCV7718CDQR2G" H 2150 2600 50  0000 C CNN
F 2 "RoomControl:On_NCV77xx" H 1800 2100 50  0001 C CNN
F 3 "" H 1800 2100 50  0001 C CNN
	1    1650 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1650 2650 1750 2650
Connection ~ 1800 850 
Text GLabel 3750 2600 0    50   Input ~ 0
AY
Text GLabel 5500 2650 0    50   Input ~ 0
BY
$Comp
L Connector_Generic:Conn_01x04 J4
U 1 1 5EF26AB1
P 7350 2150
F 0 "J4" H 7350 2350 50  0000 C CNN
F 1 "Conn_01x04" V 7450 2100 50  0000 C CNN
F 2 "RoomControl:Switch_Mount" H 7350 2150 50  0001 C CNN
F 3 "~" H 7350 2150 50  0001 C CNN
	1    7350 2150
	-1   0    0    -1  
$EndComp
Text GLabel 7550 2050 2    50   Input ~ 0
PE
Text GLabel 7550 2150 2    50   Input ~ 0
PE
Text GLabel 7550 2250 2    50   Input ~ 0
PE
Text GLabel 7550 2350 2    50   Input ~ 0
PE
Text GLabel 2600 4950 0    50   Input ~ 0
LIN_EN
Text GLabel 4550 5350 0    50   Input ~ 0
LIN_EN
Text GLabel 3400 5250 2    50   Input ~ 0
RESET
$Comp
L Device:LED D3
U 1 1 5EFDFD1B
P 4700 5700
F 0 "D3" V 4750 5500 50  0000 L CNN
F 1 "LED" V 4650 5500 50  0000 L CNN
F 2 "LED_SMD:LED_0805_2012Metric_Castellated" H 4700 5700 50  0001 C CNN
F 3 "~" H 4700 5700 50  0001 C CNN
	1    4700 5700
	0    1    -1   0   
$EndComp
$Comp
L Device:R R1
U 1 1 5EFE13C0
P 4700 6000
F 0 "R1" H 4770 6046 50  0000 L CNN
F 1 "330R" H 4770 5955 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4630 6000 50  0001 C CNN
F 3 "~" H 4700 6000 50  0001 C CNN
	1    4700 6000
	1    0    0    -1  
$EndComp
Text GLabel 1150 1200 0    50   Input ~ 0
HB_EN
Text GLabel 4550 5450 0    50   Input ~ 0
HB_EN
Wire Wire Line
	5050 5550 5050 5450
Connection ~ 5050 5450
Wire Wire Line
	5050 5450 5350 5450
$Comp
L power:GND #PWR0109
U 1 1 5EDF7E00
P 5050 6150
F 0 "#PWR0109" H 5050 5900 50  0001 C CNN
F 1 "GND" H 5055 5977 50  0000 C CNN
F 2 "" H 5050 6150 50  0001 C CNN
F 3 "" H 5050 6150 50  0001 C CNN
	1    5050 6150
	1    0    0    -1  
$EndComp
Text GLabel 5350 4650 0    50   Input ~ 0
RESET
$Comp
L Device:D D2
U 1 1 5EE4724C
P 1350 4800
F 0 "D2" H 1400 4700 50  0000 C CNN
F 1 "D" H 1300 4700 50  0000 C CNN
F 2 "Diode_SMD:D_SOT-23_ANK" H 1350 4800 50  0001 C CNN
F 3 "~" H 1350 4800 50  0001 C CNN
	1    1350 4800
	-1   0    0    1   
$EndComp
Wire Wire Line
	1500 5000 1500 4800
Connection ~ 1500 4800
Wire Wire Line
	1000 5000 1200 5000
Wire Wire Line
	1200 5000 1200 4800
Connection ~ 1200 5000
Wire Wire Line
	7600 5350 8000 5350
$Comp
L Device:LED D4
U 1 1 5F02CF2B
P 5050 5700
F 0 "D4" V 5100 5500 50  0000 L CNN
F 1 "LED" V 5000 5500 50  0000 L CNN
F 2 "LED_SMD:LED_0805_2012Metric_Castellated" H 5050 5700 50  0001 C CNN
F 3 "~" H 5050 5700 50  0001 C CNN
	1    5050 5700
	0    1    -1   0   
$EndComp
$Comp
L Device:R R2
U 1 1 5F02D5C3
P 5050 6000
F 0 "R2" H 5120 6046 50  0000 L CNN
F 1 "330R" H 5120 5955 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4980 6000 50  0001 C CNN
F 3 "~" H 5050 6000 50  0001 C CNN
	1    5050 6000
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0110
U 1 1 5F02D8D1
P 4700 6150
F 0 "#PWR0110" H 4700 5900 50  0001 C CNN
F 1 "GND" H 4705 5977 50  0000 C CNN
F 2 "" H 4700 6150 50  0001 C CNN
F 3 "" H 4700 6150 50  0001 C CNN
	1    4700 6150
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 5350 4700 5350
Wire Wire Line
	4550 5450 5050 5450
Wire Wire Line
	4700 5550 4700 5350
Connection ~ 4700 5350
Wire Wire Line
	4700 5350 5350 5350
$EndSCHEMATC
