/* ProfileCreator V.3.6 (UAV) Jun 22 2012 */
/*  3.2: Only : CMin WMin WMax CMax */
/*  3.3: Add section VALUE, new PARAM struct */
/*  3.4: Add GRP_ACCESS (#MFLAG) */
/*  3.6: dynamic param colors to .value, ?_getvalue()*/
u32 cmmcurebootcnt_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmmcuwatchdogrebootcnt_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1wl_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1rev_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1ddmstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2wl_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2rev_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2ddmstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsequencerstate_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 vtype_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 vtype_update(PARAM_INDEX* p_ix, void* buff);
u32 countslots_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1templimit1_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1templimit2_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1templimit3_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1templimit4_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps1templimit5_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2vendor_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2ptnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2power_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2srnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2templimit1_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2templimit2_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2templimit3_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2templimit4_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 ps2templimit5_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanmodels_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanmodels_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanminrpm_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanminrpm_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanmaxrpm_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanmaxrpm_update(PARAM_INDEX* p_ix, void* buff);
u32 fufannum_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufannum_update(PARAM_INDEX* p_ix, void* buff);
u32 fuhwnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fuhwnumber_update(PARAM_INDEX* p_ix, void* buff);
u32 fusrnumber_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fusrnumber_update(PARAM_INDEX* p_ix, void* buff);
u32 cmswhash_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 localtime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 dyn_par_access(PARAM_INDEX* p_id, void* buff, u32 buff_len);
u32 localdate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmsfppres_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmsfppin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmsfppout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmsfpthrmode_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmfelink_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmgelink_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmoscalarm_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 uptime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmi33v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmi12v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 slotsstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 passiveslotstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 v33v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 v12v_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 vpmbusalarm_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 pscommonpower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 pspres_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 psinpwrstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 psoutpwrstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 pstempfaultstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 psfanwarningstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 psfanfaultstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1fanspeed_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1temperatureflow_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1temperatureflow2_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1temperaturehotspot1_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1temperaturehotspot2_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1temperaturehotspot3_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1vin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1iin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1voltageout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1iout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps1outcurrent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2fanspeed_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2temperatureflow_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2temperatureflow2_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2temperaturehotspot1_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2temperaturehotspot2_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2temperaturehotspot3_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2vin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2iin_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2voltageout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2iout_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 ps2outcurrent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fupresent_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fufanspeed_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fuv12i_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fuv33i_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fueepromstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 slotpowerstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 slotname_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 slotpower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 slotmaxpower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fupowerstatus_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 funame_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 fupower_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmprofdelay_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 alarm1_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 alarm2_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 blockcolor_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 tcase_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cpuusage_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 memload_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 diskspace_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmtime_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 vstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 vpowerreserve_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 veepromstate_getvalue(PARAM_INDEX* p_ix,P32_PTR pPar);
u32 cmsfptxenable_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfptxenable_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfpthrreset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfpthrreset_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfpautoneg_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfpautoneg_update(PARAM_INDEX* p_ix, void* buff);
u32 cmreset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmreset_update(PARAM_INDEX* p_ix, void* buff);
u32 vformateeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 vformateeprom_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanspeedset_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanspeedset_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanmode_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanmode_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanthreshold75_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanthreshold75_update(PARAM_INDEX* p_ix, void* buff);
u32 fufanthreshold90_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 fufanthreshold90_update(PARAM_INDEX* p_ix, void* buff);
u32 slotwreeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slotwreeprom_update(PARAM_INDEX* p_ix, void* buff);
u32 slotopteepromclear_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slotopteepromclear_update(PARAM_INDEX* p_ix, void* buff);
u32 slotopteepromwrite_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slotenable_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slotenable_update(PARAM_INDEX* p_ix, void* buff);
u32 pswreeprom_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 pswreeprom_update(PARAM_INDEX* p_ix, void* buff);
u32 vpowerlimitsource_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 vpowerlimitsource_update(PARAM_INDEX* p_ix, void* buff);
u32 vpowerlimit_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 vpowerlimit_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1pincmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1pincmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1pinwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1pinwmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1pinwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1pinwmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1pincmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1pincmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1poutcmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1poutcmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1poutwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1poutwmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1poutwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1poutwmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp1poutcmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp1poutcmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2pincmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2pincmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2pinwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2pinwmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2pinwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2pinwmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2pincmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2pincmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2poutcmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2poutcmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2poutwmax_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2poutwmax_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2poutwmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2poutwmin_update(PARAM_INDEX* p_ix, void* buff);
u32 cmsfp2poutcmin_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 cmsfp2poutcmin_update(PARAM_INDEX* p_ix, void* buff);
u32 led1state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 led2state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 led3state_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slot_access(PARAM_INDEX* p_ix, void* buff, u32 buff_len);
u32 slot_update(PARAM_INDEX* p_ix, void* buff);
