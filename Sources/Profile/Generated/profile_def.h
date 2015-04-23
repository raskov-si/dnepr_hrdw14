/* ProfileCreator V.3.6 (UAV) Jun 22 2012 */
/*  3.2: Only : CMin WMin WMax CMax */
/*  3.3: Add section VALUE, new PARAM struct */
/*  3.4: Add GRP_ACCESS (#MFLAG) */
/*  3.6: dynamic param colors to .value, ?_getvalue()*/
#define Header1 0
#define Header2 1
#define InformSect 2
#define CMHwNumber 3
#define SwNumber 4
#define CMSwNumber 5
#define CMSrNumber 6
#define CMPtNumber 7
#define Destination 8
#define Descr 9
#define Location 10
#define CMPort1Info 11
#define CMPort2Info 12
#define RebootCounter 13
#define CMMCURebootCnt 14
#define CMMCUWatchdogRebootCnt 15
#define CMSFP1Vendor 16
#define CMSFP1PtNumber 17
#define CMSFP1SrNumber 18
#define CMSFP1WL 19
#define CMSFP1Rev 20
#define CMSFP1DDMState 21
#define CMSFP2Vendor 22
#define CMSFP2PtNumber 23
#define CMSFP2SrNumber 24
#define CMSFP2WL 25
#define CMSFP2Rev 26
#define CMSFP2DDMState 27
#define CMSequencerState 28
#define VPtNumber 29
#define VSrNumber 30
#define VType 31
#define CountSlots 32
#define PS1Vendor 33
#define PS1PtNumber 34
#define PS1Power 35
#define PS1SrNumber 36
#define PS2Vendor 37
#define PS2PtNumber 38
#define PS2Power 39
#define PS2SrNumber 40
#define PS3Vendor 41
#define PS3PtNumber 42
#define PS3Power 43
#define PS3SrNumber 44
#define PS4Vendor 45
#define PS4PtNumber 46
#define PS4Power 47
#define PS4SrNumber 48
#define FUFanModels 49
#define FUFanMinRPM 50
#define FUFanMaxRPM 51
#define FUFanDefaulMinRPM 52
#define FUFanDefaulMaxRPM 53
#define FUFanNum 54
#define FUHwNumber 55
#define FUSrNumber 56
#define pId 57
#define KernelVersion 58
#define sysObjectID 59
#define CMSwHash 60
#define sysLanguage 61
#define sysDevType 62
#define DynamicSect 63
#define ALARM1 64
#define ALARM2 65
#define BlockColor 66
#define TCase 67
#define CPUUsage 68
#define MemLoad 69
#define DiskSpace 70
#define LocalTime 71
#define LocalDate 72
#define CMSFP1Pres 73
#define CMSFP1Pin 74
#define CMSFP1Pout 75
#define CMSFP1ThrMode 76
#define CMSFP2Pres 77
#define CMSFP2Pin 78
#define CMSFP2Pout 79
#define CMSFP2ThrMode 80
#define CMTime 81
#define CMOSCAlarm 82
#define OVLPCUMainRole 83
#define OVLPCURsrvRole 84
#define OVLPCPUMainState 85
#define OVLPMCUMainState 86
#define OVLPCPUReservState 87
#define OVLPMCUReservState 88
#define UpTime 89
#define CMI33V 90
#define CMI12V 91
#define VState 92
#define VPowerReserve 93
#define SlotsState 94
#define PassiveSlotState 95
#define VEEPROMState 96
#define V33V 97
#define V12V 98
#define VPMBusAlarm 99
#define PS1Pres 100
#define PS1InPwrStatus 101
#define PS1OutPwrStatus 102
#define PS1OutCurrent 103
#define PS2Pres 104
#define PS2InPwrStatus 105
#define PS2OutPwrStatus 106
#define PS2OutCurrent 107
#define PS3Pres 108
#define PS3InPwrStatus 109
#define PS3OutPwrStatus 110
#define PS3OutCurrent 111
#define PS4Pres 112
#define PS4InPwrStatus 113
#define PS4OutPwrStatus 114
#define PS4OutCurrent 115
#define FUPresent 116
#define FUFan1Speed 117
#define FUFan2Speed 118
#define FUFan3Speed 119
#define FUFan4Speed 120
#define FUFan5Speed 121
#define FUFan6Speed 122
#define FUV12I 123
#define FUV33I 124
#define FUEEPROMState 125
#define Slot1PowerStatus 126
#define Slot1Name 127
#define Slot1Power 128
#define Slot2PowerStatus 129
#define Slot2Name 130
#define Slot2Power 131
#define Slot3PowerStatus 132
#define Slot3Name 133
#define Slot3Power 134
#define Slot4PowerStatus 135
#define Slot4Name 136
#define Slot4Power 137
#define Slot5PowerStatus 138
#define Slot5Name 139
#define Slot5Power 140
#define Slot6PowerStatus 141
#define Slot6Name 142
#define Slot6Power 143
#define Slot7PowerStatus 144
#define Slot7Name 145
#define Slot7Power 146
#define Slot8PowerStatus 147
#define Slot8Name 148
#define Slot8Power 149
#define Slot9PowerStatus 150
#define Slot9Name 151
#define Slot9Power 152
#define Slot10PowerStatus 153
#define Slot10Name 154
#define Slot10Power 155
#define Slot11PowerStatus 156
#define Slot11Name 157
#define Slot11Power 158
#define Slot12PowerStatus 159
#define Slot12Name 160
#define Slot12Power 161
#define Slot13PowerStatus 162
#define Slot13Name 163
#define Slot13Power 164
#define SlotMaxPower 165
#define FUPowerStatus 166
#define FUName 167
#define FUPower 168
#define CMProfDelay 169
#define SetSect 170
#define LogIdleTime 171
#define LogDays 172
#define LogSize 173
#define LogAlarms 174
#define LogDebug 175
#define CMPhyAddr 176
#define CMSFP1TxEnable 177
#define CMSFP2TxEnable 178
#define CMSFP1ThrReset 179
#define CMSFP2ThrReset 180
#define CMSFP1AutoNeg 181
#define CMSFP2AutoNeg 182
#define CMReset 183
#define VFormatEEPROM 184
#define FUFanSpeedSet 185
#define FUFanMode 186
#define FUFanThreshold75 187
#define FUFanThreshold90 188
#define SlotPowerSet 189
#define SlotNameSet 190
#define Slot12v0Bypass 191
#define Slot12v0OCAR 192
#define Slot12v0UVAR 193
#define Slot12v0OVAR 194
#define Slot3v3Bypass 195
#define Slot3v3OCAR 196
#define Slot3v3UVAR 197
#define Slot3v3OVAR 198
#define SlotWrEEPROM 199
#define SlotOptEEPROMClear 200
#define SlotOptPassive 201
#define SlotOptDescription 202
#define SlotOptDestination 203
#define SlotOptSerial 204
#define SlotOptClass 205
#define SlotOptEEPROMWrite 206
#define Slot1Enable 207
#define Slot2Enable 208
#define Slot3Enable 209
#define Slot4Enable 210
#define Slot5Enable 211
#define Slot6Enable 212
#define Slot7Enable 213
#define Slot8Enable 214
#define Slot9Enable 215
#define Slot10Enable 216
#define Slot11Enable 217
#define Slot12Enable 218
#define Slot13Enable 219
#define PSVendorSet 220
#define PSPtNumberSet 221
#define PSPowerSet 222
#define PSSrNumberSet 223
#define PSWrEEPROM 224
#define PS1CurrentK 225
#define PS1CurrentB 226
#define PS2CurrentK 227
#define PS2CurrentB 228
#define PS3CurrentK 229
#define PS3CurrentB 230
#define PS4CurrentK 231
#define PS4CurrentB 232
#define VPowerLimitSource 233
#define VPowerLimit 234
#define LimitSect 235
#define MemLoadCMax 236
#define MemLoadWMax 237
#define DiskSpaceCMax 238
#define DiskSpaceWMax 239
#define LogIdleTimeHMax 240
#define LogIdleTimeHMin 241
#define LogDaysHMax 242
#define LogDaysHMin 243
#define LogAlarmsHMax 244
#define LogAlarmsHMin 245
#define LogSizeHMax 246
#define LogSizeHMin 247
#define TCaseCMax 248
#define TCaseWMax 249
#define TCaseWMin 250
#define TCaseCMin 251
#define V33VCMax 252
#define V33VWMax 253
#define V33VWMin 254
#define V33VCMin 255
#define V12VCMax 256
#define V12VWMax 257
#define V12VWMin 258
#define V12VCMin 259
#define CMSFP1PinCMax 260
#define CMSFP1PinWMax 261
#define CMSFP1PinWMin 262
#define CMSFP1PinCMin 263
#define CMSFP1PoutCMax 264
#define CMSFP1PoutWMax 265
#define CMSFP1PoutWMin 266
#define CMSFP1PoutCMin 267
#define CMSFP2PinCMax 268
#define CMSFP2PinWMax 269
#define CMSFP2PinWMin 270
#define CMSFP2PinCMin 271
#define CMSFP2PoutCMax 272
#define CMSFP2PoutWMax 273
#define CMSFP2PoutWMin 274
#define CMSFP2PoutCMin 275
#define PS2TempCMax 276
#define PS2TempWMax 277
#define PS2TempWMin 278
#define PS2TempCMin 279
#define PS2VoutCMax 280
#define PS2VoutWMax 281
#define PS2VoutWMin 282
#define PS2VoutCMin 283
#define FANRPMCMax 284
#define FANRPMWMax 285
#define FANRPMWMin 286
#define FANRPMCMin 287
#define LEDSect 288
#define Led1State 289
#define Led2State 290
#define Led3State 291
#define AddSect100 292
#define Address 293
#define Slot 294
#define ModeWork 295
#define ClassDevice 296
#define LoadingProfileTime 297
#define AlarmState 298
#define AddSect101 299
#define IPAddress 300
#define IPMask 301
#define IPGateway 302
#define CUNetCPUIPAddress 303
#define CUNetMCUIPAddress 304
#define CUNetIPMask 305
#define Compname 306
#define Compgroup 307
#define IPSNMP1 308
#define IPSNMP2 309
#define IPSNMP3 310
#define IPSNMP4 311
#define IPSNTP 312
#define IPSyslog 313
#define TFTPServerIP 314
#define TFTPClientIP 315
#define NMSServerUrl 316
#define AddSect102 317
#define DisableTraps 318
#define Sect200 319
#define Slot1 320
#define Slot2 321
#define Slot3 322
#define Slot4 323
#define Slot5 324
#define Slot6 325
#define Slot7 326
#define Slot8 327
#define Slot9 328
#define Slot10 329
#define Slot11 330
#define Slot12 331
#define Slot13 332
#define Slot14 333


#define MAX_VALUE_LEN 761
#define MAX_DATA_LEN 128
#define ALL_MAX_LEN 761
#define DYN_PAR_ALL 106
