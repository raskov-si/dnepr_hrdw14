s8  val_Header2[23] = "3.0;dnepr2;dnepra;2;5";
s8  val_CMHwNumber[33] = "1.4";
s8  val_SwNumber[33] = "0";
s8  val_CMSwNumber[33] = "3.3.27.2";
s8  val_CMSrNumber[33] = " ";
s8  val_CMPtNumber[33] = " ";
s8  val_Destination[65] = " ";
s8  val_Descr[65] = " ";
s8  val_Location[65] = " ";
s8  val_CMPort1Info[65] = "Port 1";
s8  val_CMPort2Info[65] = "Port 2";
u32 val_RebootCounter = 0;
s8  val_CMSFP1PtNumber[17] = " ";
s8  val_CMSFP1SrNumber[17] = " ";
s8  val_CMSFP2PtNumber[17] = " ";
s8  val_CMSFP2SrNumber[17] = " ";
s8  val_VPtNumber[33] = " ";
s8  val_VSrNumber[33] = " ";
u32 val_VType = 2;
u32 val_CountSlots = 13;
s8  val_PS1Vendor[17] = " ";
s8  val_PS1PtNumber[33] = " ";
s8  val_PS1SrNumber[9] = " ";
s8  val_PS2Vendor[17] = " ";
s8  val_PS2PtNumber[33] = " ";
s8  val_PS2SrNumber[9] = " ";
u32 val_FUFanDefaulMinRPM = 1000;
u32 val_FUFanDefaulMaxRPM = 4000;
s8  val_FUSrNumber[33] = "0";
s8  val_pId[33] = "CM-S-2G-3";
s8  val_sysObjectID[33] = " ";
u32 val_sysLanguage = 1;
s8  val_sysDevType[17] = "0";
u8  col_LocalTime[3] = {7,5,3};
u8  col_LocalDate[3] = {7,5,3};
u8  col_CMSFP1Pres[3] = {7,5,3};
u8  col_CMSFP1Pin[3] = {7,5,3};
u8  col_CMSFP1Pout[3] = {7,5,3};
u8  col_CMSFP1ThrMode[3] = {7,5,3};
u8  col_CMSFP2Pres[3] = {7,5,3};
u8  col_CMSFP2Pin[3] = {7,5,3};
u8  col_CMSFP2Pout[3] = {7,5,3};
u8  col_CMSFP2ThrMode[3] = {7,5,3};
u8  col_CMFE1Link[3] = {7,5,3};
u8  col_CMFE2Link[3] = {7,5,3};
u8  col_CMFE3Link[3] = {7,5,3};
u8  col_CMGE1Link[3] = {7,5,3};
u8  col_CMGE2Link[3] = {7,5,3};
u8  col_CMOSCAlarm[3] = {7,5,3};
u8  col_UpTime[3] = {7,5,3};
u8  col_CMI33V[3] = {7,5,3};
u8  col_CMI12V[3] = {7,5,3};
u8  col_SlotsState[3] = {7,5,3};
u8  col_PassiveSlotState[3] = {7,5,3};
u8  col_V33V[3] = {7,5,3};
u8  col_V12V[3] = {7,5,3};
u8  col_VPMBusAlarm[3] = {7,5,3};
u8  col_PSCommonPower[3] = {7,5,3};
u8  col_PS1Pres[3] = {7,5,3};
u8  col_PS1InPwrStatus[3] = {7,5,3};
u8  col_PS1OutPwrStatus[3] = {7,5,3};
u8  col_PS1TempFaultStatus[3] = {7,5,3};
u8  col_PS1FanWarningStatus[3] = {7,5,3};
u8  col_PS1FanFaultStatus[3] = {7,5,3};
u8  col_PS1FanSpeed[3] = {7,5,3};
u8  col_PS1TemperatureFlow[3] = {7,5,3};
u8  col_PS1TemperatureFlow2[3] = {7,5,3};
u8  col_PS1TemperatureHotSpot1[3] = {7,5,3};
u8  col_PS1TemperatureHotSpot2[3] = {7,5,3};
u8  col_PS1TemperatureHotSpot3[3] = {7,5,3};
u8  col_PS1Vin[3] = {7,5,3};
u8  col_PS1Iin[3] = {7,5,3};
u8  col_PS1VoltageOut[3] = {7,5,3};
u8  col_PS1Iout[3] = {7,5,3};
u8  col_PS1OutCurrent[3] = {7,5,3};
u8  col_PS2Pres[3] = {7,5,3};
u8  col_PS2InPwrStatus[3] = {7,5,3};
u8  col_PS2OutPwrStatus[3] = {7,5,3};
u8  col_PS2TempFaultStatus[3] = {7,5,3};
u8  col_PS2FanWarningStatus[3] = {7,5,3};
u8  col_PS2FanFaultStatus[3] = {7,5,3};
u8  col_PS2FanSpeed[3] = {7,5,3};
u8  col_PS2TemperatureFlow[3] = {7,5,3};
u8  col_PS2TemperatureFlow2[3] = {7,5,3};
u8  col_PS2TemperatureHotSpot1[3] = {7,5,3};
u8  col_PS2TemperatureHotSpot2[3] = {7,5,3};
u8  col_PS2TemperatureHotSpot3[3] = {7,5,3};
u8  col_PS2Vin[3] = {7,5,3};
u8  col_PS2Iin[3] = {7,5,3};
u8  col_PS2VoltageOut[3] = {7,5,3};
u8  col_PS2Iout[3] = {7,5,3};
u8  col_PS2OutCurrent[3] = {7,5,3};
u8  col_FUPresent[3] = {7,5,3};
u8  col_FUFan1Speed[3] = {7,5,3};
u8  col_FUFan2Speed[3] = {7,5,3};
u8  col_FUFan3Speed[3] = {7,5,3};
u8  col_FUFan4Speed[3] = {7,5,3};
u8  col_FUFan5Speed[3] = {7,5,3};
u8  col_FUFan6Speed[3] = {7,5,3};
u8  col_FUV12I[3] = {7,5,3};
u8  col_FUV33I[3] = {7,5,3};
u32 val_FUEEPROMState = 0;
u8  col_FUEEPROMState[3] = {7,5,3};
u32 val_Slot1PowerStatus = 0;
u8  col_Slot1PowerStatus[3] = {7,5,3};
u8  col_Slot1Name[3] = {7,5,3};
u8  col_Slot1Power[3] = {7,5,3};
u32 val_Slot2PowerStatus = 0;
u8  col_Slot2PowerStatus[3] = {7,5,3};
u8  col_Slot2Name[3] = {7,5,3};
u8  col_Slot2Power[3] = {7,5,3};
u32 val_Slot3PowerStatus = 0;
u8  col_Slot3PowerStatus[3] = {7,5,3};
u8  col_Slot3Name[3] = {7,5,3};
u8  col_Slot3Power[3] = {7,5,3};
u32 val_Slot4PowerStatus = 0;
u8  col_Slot4PowerStatus[3] = {7,5,3};
u8  col_Slot4Name[3] = {7,5,3};
u8  col_Slot4Power[3] = {7,5,3};
u32 val_Slot5PowerStatus = 0;
u8  col_Slot5PowerStatus[3] = {7,5,3};
u8  col_Slot5Name[3] = {7,5,3};
u8  col_Slot5Power[3] = {7,5,3};
u32 val_Slot6PowerStatus = 0;
u8  col_Slot6PowerStatus[3] = {7,5,3};
u8  col_Slot6Name[3] = {7,5,3};
u8  col_Slot6Power[3] = {7,5,3};
u32 val_Slot7PowerStatus = 0;
u8  col_Slot7PowerStatus[3] = {7,5,3};
u8  col_Slot7Name[3] = {7,5,3};
u8  col_Slot7Power[3] = {7,5,3};
u32 val_Slot8PowerStatus = 0;
u8  col_Slot8PowerStatus[3] = {7,5,3};
u8  col_Slot8Name[3] = {7,5,3};
u8  col_Slot8Power[3] = {7,5,3};
u32 val_Slot9PowerStatus = 0;
u8  col_Slot9PowerStatus[3] = {7,5,3};
u8  col_Slot9Name[3] = {7,5,3};
u8  col_Slot9Power[3] = {7,5,3};
u32 val_Slot10PowerStatus = 0;
u8  col_Slot10PowerStatus[3] = {7,5,3};
u8  col_Slot10Name[3] = {7,5,3};
u8  col_Slot10Power[3] = {7,5,3};
u32 val_Slot11PowerStatus = 0;
u8  col_Slot11PowerStatus[3] = {7,5,3};
u8  col_Slot11Name[3] = {7,5,3};
u8  col_Slot11Power[3] = {7,5,3};
u32 val_Slot12PowerStatus = 0;
u8  col_Slot12PowerStatus[3] = {7,5,3};
u8  col_Slot12Name[3] = {7,5,3};
u8  col_Slot12Power[3] = {7,5,3};
u32 val_Slot13PowerStatus = 0;
u8  col_Slot13PowerStatus[3] = {7,5,3};
u8  col_Slot13Name[3] = {7,5,3};
u8  col_Slot13Power[3] = {7,5,3};
u8  col_SlotMaxPower[3] = {7,5,3};
u8  col_FUPowerStatus[3] = {7,5,3};
u8  col_FUName[3] = {7,5,3};
u8  col_FUPower[3] = {7,5,3};
u8  col_CMProfDelay[3] = {7,5,3};
u8  col_ALARM1[3] = {0,5,3};
u8  col_ALARM2[3] = {7,0,3};
u8  col_BlockColor[3] = {0,0,3};
u8  col_TCase[3] = {7,5,3};
u8  col_CPUUsage[3] = {7,5,3};
u8  col_MemLoad[3] = {7,5,3};
u8  col_DiskSpace[3] = {7,5,3};
u8  col_CMTime[3] = {0,0,3};
u8  col_VState[3] = {7,5,3};
u8  col_VPowerReserve[3] = {7,5,3};
u8  col_VEEPROMState[3] = {7,5,3};
s32 val_LogIdleTime = 15;
s32 val_LogDays = 30;
s32 val_LogSize = 5000;
s32 val_LogAlarms = 50;
u32 val_LogDebug = 0;
s8  val_CMPhyAddr[19] = "0";
u32 val_CMSFP1TxEnable = 0;
u32 val_CMSFP2TxEnable = 0;
u32 val_CMSFP1ThrReset = 1;
u32 val_CMSFP2ThrReset = 1;
u32 val_CMSFP1AutoNeg = 1;
u32 val_CMSFP2AutoNeg = 1;
u32 val_VFormatEEPROM = 0;
u32 val_FUFanSpeedSet = 50;
u32 val_FUFanMode = 0;
u32 val_FUFanThreshold75 = 90;
u32 val_FUFanThreshold90 = 140;
u32 val_SlotPowerSet = 0;
s8  val_SlotNameSet[16] = "DeviceName";
f32 val_Slot12v0Bypass = 50;
u32 val_Slot12v0OCAR = 1;
u32 val_Slot12v0UVAR = 1;
u32 val_Slot12v0OVAR = 1;
f32 val_Slot3v3Bypass = 50;
u32 val_Slot3v3OCAR = 1;
u32 val_Slot3v3UVAR = 1;
u32 val_Slot3v3OVAR = 1;
u32 val_SlotWrEEPROM = 0;
u32 val_SlotOptEEPROMClear = 0;
u32 val_SlotOptPassive = 0;
s8  val_SlotOptDescription[32] = " ";
s8  val_SlotOptDestination[32] = " ";
s8  val_SlotOptSerial[32] = " ";
s8  val_SlotOptClass[32] = " ";
u32 val_SlotOptEEPROMWrite = 0;
u32 val_Slot1Enable = 2;
u32 val_Slot2Enable = 2;
u32 val_Slot3Enable = 2;
u32 val_Slot4Enable = 2;
u32 val_Slot5Enable = 2;
u32 val_Slot6Enable = 2;
u32 val_Slot7Enable = 2;
u32 val_Slot8Enable = 2;
u32 val_Slot9Enable = 2;
u32 val_Slot10Enable = 2;
u32 val_Slot11Enable = 2;
u32 val_Slot12Enable = 2;
u32 val_Slot13Enable = 2;
s8  val_PSVendorSet[33] = "T8";
s8  val_PSPtNumberSet[33] = " ";
u32 val_PSPowerSet = 200;
s8  val_PSSrNumberSet[33] = " ";
u32 val_PSWrEEPROM = 0;
f32 val_PS1CurrentK = 1;
f32 val_PS1CurrentB = 0;
f32 val_PS2CurrentK = 1;
f32 val_PS2CurrentB = 0;
u32 val_VPowerLimitSource = 0;
u32 val_VPowerLimit = 0;
u32 val_VPowerMinReserve = 0;
s32 val_MemLoadCMax = 90;
s32 val_MemLoadWMax = 75;
s32 val_DiskSpaceCMax = 90;
s32 val_DiskSpaceWMax = 75;
s32 val_LogIdleTimeHMax = 1440;
s32 val_LogIdleTimeHMin = 1;
s32 val_LogDaysHMax = 31;
s32 val_LogDaysHMin = 1;
s32 val_LogAlarmsHMax = 100;
s32 val_LogAlarmsHMin = 1;
s32 val_LogSizeHMax = 2000;
s32 val_LogSizeHMin = 1;
f32 val_TCaseCMax = 60;
f32 val_TCaseWMax = 50;
f32 val_TCaseWMin = 10;
f32 val_TCaseCMin = 0;
f32 val_CMSFP1PinCMax = 0;
f32 val_CMSFP1PinWMax = 0;
f32 val_CMSFP1PinWMin = 0;
f32 val_CMSFP1PinCMin = 0;
f32 val_CMSFP1PoutCMax = 8.2;
f32 val_CMSFP1PoutWMax = 0;
f32 val_CMSFP1PoutWMin = -40;
f32 val_CMSFP1PoutCMin = -40;
f32 val_CMSFP2PinCMax = 0;
f32 val_CMSFP2PinWMax = 0;
f32 val_CMSFP2PinWMin = 0;
f32 val_CMSFP2PinCMin = 0;
f32 val_CMSFP2PoutCMax = 8.2;
f32 val_CMSFP2PoutWMax = 0;
f32 val_CMSFP2PoutWMin = -40;
f32 val_CMSFP2PoutCMin = -40;
f32 val_PS2TempCMax = 50;
f32 val_PS2TempWMax = 45;
f32 val_PS2TempWMin = 0;
f32 val_PS2TempCMin = -10;
f32 val_PS2VoutCMax = 13;
f32 val_PS2VoutWMax = 12.5;
f32 val_PS2VoutWMin = 11.5;
f32 val_PS2VoutCMin = 11;
f32 val_FANRPMCMax = 100;
f32 val_FANRPMWMax = 90;
f32 val_FANRPMWMin = 10;
f32 val_FANRPMCMin = 0;
s32 val_Address = 0;
s32 val_Slot = 0;
s32 val_ModeWork = 0;
s32 val_ClassDevice = 0;
s32 val_LoadingProfileTime = 0;
s32 val_AlarmState = 0;
s8  val_IPAddress[18] = "192.168.1.1";
s8  val_IPMask[18] = "255.255.255.0";
s8  val_IPGateway[18] = "0.0.0.0";
s8  val_LocalSubnet[19] = " 169.254.100.0/24";
s8  val_Compname[17] = " ";
s8  val_Compgroup[17] = " ";
s8  val_IPSNMP1[18] = "0.0.0.0";
s8  val_IPSNMP2[18] = "0.0.0.0";
s8  val_IPSNMP3[18] = "0.0.0.0";
s8  val_IPSNMP4[18] = "0.0.0.0";
s8  val_IPSNTP[18] = "0.0.0.0";
s8  val_IPSyslog[18] = "0.0.0.0";
s8  val_TFTPServerIP[18] = "0.0.0.0";
s8  val_TFTPClientIP[18] = "0.0.0.0";
s8  val_NMSServerUrl[129] = "0.0.0.0";
u32 val_DisableTraps = 0;
s8  val_Slot1[3] = "0";
s8  val_Slot2[3] = "0";
s8  val_Slot3[3] = "0";
s8  val_Slot4[3] = "0";
s8  val_Slot5[3] = "0";
s8  val_Slot6[3] = "0";
s8  val_Slot7[3] = "0";
s8  val_Slot8[3] = "0";
s8  val_Slot9[3] = "0";
s8  val_Slot10[3] = "0";
s8  val_Slot11[3] = "0";
s8  val_Slot12[3] = "0";
s8  val_Slot13[3] = "0";
s8  val_Slot14[3] = "0";
