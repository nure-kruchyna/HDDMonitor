#pragma once

#include "resource.h"

//Индексы SMART аттрибутов
#define SMART_RAW_READ_ERROR_RATE                    1
#define SMART_THROUGHPUT_PERFORMANCE                 2
#define SMART_SPIN_UP_TIME                           3
#define SMART_START_STOP_COUNT                       4
#define SMART_REALLOCATED_SECTOR_COUNT               5
#define SMART_SEEK_ERROR_RATE                        7
#define SMART_SEEK_TIME_PERFORMANCE                  8
#define SMART_POWER_ON_HOURS_COUNT                   9
#define SMART_SPIN_RETRY_COUNT                       10
#define SMART_RECALIBRATION_RETRIES                  11
#define SMART_DEVICE_POWER_CYCLE_COUNT               12
#define SMART_SOFT_READ_ERROR_RATE1                  13
#define SMART_ERASE_CYCLES                           100
#define SMART_TRANSLATION_TABLE_REBUID               103
#define SMART_RESERVED_BLOCK_COUNT                   170
#define SMART_PROGRAM_FAIL_COUNT1                    171
#define SMART_ERASE_FAIL_COUNT1                      172
#define SMART_WEAR_LEVELLER_WORST_CASE_ERASE_COUNT   173
#define SMART_UNEXPECTED_POWER_LOSS                  174
#define SMART_PROGRAM_FAIL_COUNT2                    175
#define SMART_ERASE_FAIL_COUNT2                      176
#define SMART_WEAR_LEVELING_COUNT                    177
#define SMART_USED_RESERVED_BLOCK_COUNT1             178
#define SMART_USED_RESERVED_BLOCK_COUNT2             179
#define SMART_UNUSED_RESERVED_BLOCK_COUNT            180
#define SMART_PROGRAM_FAIL_COUNT3                    181
#define SMART_ERASE_FAIL_COUNT3                      182
#define SMART_SATA_DOWNSHIFTS                        183
#define SMART_END_TO_END_ERROR                       184
#define SMART_HEAD_STABILITY                         185
#define SMART_REPORTED_UNC_ERRORS                    187
#define SMART_COMMAND_TIMEOUT                        188
#define SMART_HIGH_FLY_WRITES                        189
#define SMART_AIRFLOW_TEMPERATURE                    190
#define SMART_G_SENSE_ERROR_RATE                     191
#define SMART_POWER_OFF_RETRACT_COUNT                192
#define SMART_LOAD_UNLOAD_CYCLE_COUNT                193
#define SMART_TEMPERATURE                            194
#define SMART_ECC_ON_THE_FLY_COUNT                   195
#define SMART_REALLOCATION_EVENT_COUNT               196
#define SMART_CURRENT_PENDING_SECTOR_COUNT           197
#define SMART_UNCORRECTABLE_SECTOR_COUNT             198
#define SMART_ULTRA_DMA_CRC_ERROR_COUNT              199
#define SMART_WRITE_ERROR_RATE                       200
#define SMART_SOFT_READ_ERROR_RATE2                  201
#define SMART_VENDOR_SPECIFIC                        202
#define SMART_ECC_ERRORS                             203
#define SMART_SOFT_ECC_ERRORS                        204
#define SMART_TAR                                    205
#define SMART_FLYING_HEIGHT                          206
#define SMART_SPIN_HEIGHT_CURRENT                    207
#define SMART_SPIN_BUZZ                              208
#define SMART_OFFLINE_SEEK_PERFORMANCE               209
#define SMART_VIBRATION_DURING_WRITE                 210
#define SMART_VIBRATION_DURING_READ                  211
#define SMART_SHOCK_DURING_WRITE                     212
#define SMART_DISK_SHIFT                             220
#define SMART_GSENSE_ERROR_RATE                      221
#define SMART_LOADED_HOURS                           222
#define SMART_LOAD_UNLOAD_RETRY_COUNT                223
#define SMART_LOAD_FRICTION                          224
#define SMART_LOAD_CYCLE_COUNT                       225
#define SMART_LOAD_TIME                              226
#define SMART_TORQUE_AMPLIFICATION_COUNT             227
#define SMART_POWER_OFF_RETRACT_CYCLE                228
#define SMART_GMR_HEAD_AMPLITUDE                     230
#define SMART_SSD_LIFE_LEFT                          231
#define SMART_AVALIABLE_RESERVED_SPACE               232
#define SMART_POWER_ON_HOURS                         233
#define SMART_UNCORRECTABLE_ECC                      234
#define SMART_POWER_FAIL_BACKUP_HEALTH               235
#define SMART_HEAD_FLYING_HOURS                      240
#define SMART_TOTAL_LBA_WRITTEN                      241
#define SMART_TOTAL_LBA_READ                         242
#define SMART_NAND_WRITES                            249
#define SMART_READ_ERROR_RETRY_RATE                  250
#define SMART_FREE_FALL_EVENT_COUNT                  254

//индексы значений в структуре данных
#define INDEX_ATTRIB_INDEX                                  0
#define INDEX_ATTRIBUTE_STATUSFLAGS                         1
#define INDEX_ATTRIB_UNKNOWN                                2
#define INDEX_ATTRIB_VALUE                                  3
#define INDEX_ATTRIB_WORST                                  4
#define INDEX_ATTRIB_RAW                                    5


LPCWSTR SmartToWstrParametr(DWORD dwParam) // переведение индексов в читаемую форму
{
    switch (dwParam)
    {
    case SMART_RAW_READ_ERROR_RATE:                      return L"Raw read error rate";
    case SMART_THROUGHPUT_PERFORMANCE:                   return L"Performance";
    case SMART_SPIN_UP_TIME:                             return L"Spin up time";
    case SMART_START_STOP_COUNT:                         return L"Start stop count";
    case SMART_REALLOCATED_SECTOR_COUNT:                 return L"Reloacated sector count";
    case SMART_SEEK_ERROR_RATE:                          return L"Seek error rate";
    case SMART_SEEK_TIME_PERFORMANCE:                    return L"Seek time";
    case SMART_POWER_ON_HOURS_COUNT:                     return L"Power on hours";
    case SMART_SPIN_RETRY_COUNT:                         return L"Spin up retries";
    case SMART_RECALIBRATION_RETRIES:                    return L"Recalibration retries";
    case SMART_DEVICE_POWER_CYCLE_COUNT:                 return L"Power cycles";
    case SMART_SOFT_READ_ERROR_RATE1:
    case SMART_SOFT_READ_ERROR_RATE2:                    return L"Soft read error";
    case SMART_ERASE_CYCLES:                             return L"Erase cycles";
    case SMART_TRANSLATION_TABLE_REBUID:                 return L"Translation table rebuild";
    case SMART_RESERVED_BLOCK_COUNT:                     return L"Reserved block count";
    case SMART_PROGRAM_FAIL_COUNT1:
    case SMART_PROGRAM_FAIL_COUNT2:
    case SMART_PROGRAM_FAIL_COUNT3:                      return L"Program fail count";
    case SMART_ERASE_FAIL_COUNT1:
    case SMART_ERASE_FAIL_COUNT2:
    case SMART_ERASE_FAIL_COUNT3:                        return L"Erase fail count";
    case SMART_WEAR_LEVELLER_WORST_CASE_ERASE_COUNT:     return L"Worst erase count";
    case SMART_UNEXPECTED_POWER_LOSS:                    return L"Unexpected power loss";
    case SMART_WEAR_LEVELING_COUNT:                      return L"Wear leveling count";
    case SMART_USED_RESERVED_BLOCK_COUNT1:
    case SMART_USED_RESERVED_BLOCK_COUNT2:               return L"Used reserved blocks";
    case SMART_UNUSED_RESERVED_BLOCK_COUNT:              return L"Unused reserved blocks";
    case SMART_SATA_DOWNSHIFTS:                          return L"Sata downshifts";
    case SMART_END_TO_END_ERROR:                         return L"End-to-end error";
    case SMART_HEAD_STABILITY:                           return L"Head stability";
    case SMART_REPORTED_UNC_ERRORS:                      return L"UNC errors";
    case SMART_COMMAND_TIMEOUT:                          return L"Command timeout";
    case SMART_HIGH_FLY_WRITES:                          return L"High fly writes";
    case SMART_AIRFLOW_TEMPERATURE:                      return L"Airflow temp";
    case SMART_G_SENSE_ERROR_RATE:                       return L"G-sensor errors";
    case SMART_POWER_OFF_RETRACT_COUNT:                  return L"Power off retract";
    case SMART_LOAD_UNLOAD_CYCLE_COUNT:                  return L"Load/unload cycles";
    case SMART_TEMPERATURE:                              return L"HDD temperature";
    case SMART_ECC_ON_THE_FLY_COUNT:                     return L"ECC on the fly";
    case SMART_REALLOCATION_EVENT_COUNT:                 return L"Relocation events";
    case SMART_CURRENT_PENDING_SECTOR_COUNT:             return L"Current pending sector count";
    case SMART_UNCORRECTABLE_SECTOR_COUNT:               return L"Unclorrectable sector count";
    case SMART_ULTRA_DMA_CRC_ERROR_COUNT:                return L"Ultra DMA CRC errors";
    case SMART_WRITE_ERROR_RATE:                         return L"Write error rate";
    case SMART_VENDOR_SPECIFIC:                          return L"VENDOR SPECIFIC";
    case SMART_ECC_ERRORS:                               return L"ECC errors";
    case SMART_SOFT_ECC_ERRORS:                          return L"Soft ECC errors";
    case SMART_TAR:                                      return L"Thermal asperity rate";
    case SMART_FLYING_HEIGHT:                            return L"Flying height";
    case SMART_SPIN_HEIGHT_CURRENT:                      return L"Spin curent";
    case SMART_SPIN_BUZZ:                                return L"Spin buzz";
    case SMART_OFFLINE_SEEK_PERFORMANCE:                 return L"Offline seek performance";
    case SMART_VIBRATION_DURING_WRITE:                   return L"Vibration during write";
    case SMART_VIBRATION_DURING_READ:                    return L"Vibration during read";
    case SMART_SHOCK_DURING_WRITE:                       return L"Shock during write";
    case SMART_DISK_SHIFT:                               return L"Disk shift";
    case SMART_GSENSE_ERROR_RATE:                        return L"G-sensor errors";
    case SMART_LOADED_HOURS:                             return L"Loaded hours";
    case SMART_LOAD_UNLOAD_RETRY_COUNT:                  return L"Load/unload retry count";
    case SMART_LOAD_FRICTION:                            return L"Load friction";
    case SMART_LOAD_CYCLE_COUNT:                         return L"Load cycle count";
    case SMART_LOAD_TIME:                                return L"Loadtime";
    case SMART_TORQUE_AMPLIFICATION_COUNT:               return L"Torque amplification count";
    case SMART_POWER_OFF_RETRACT_CYCLE:                  return L"Power off retract cycle";
    case SMART_GMR_HEAD_AMPLITUDE:                       return L"GMR head amplitude";
    case SMART_SSD_LIFE_LEFT:                            return L"SSD life left";
    case SMART_AVALIABLE_RESERVED_SPACE:                 return L"Avaliable reserved space";
    case SMART_POWER_ON_HOURS:                           return L"Power on hours";
    case SMART_UNCORRECTABLE_ECC:                        return L"Uncorrectable ECC";
    case SMART_POWER_FAIL_BACKUP_HEALTH:                 return L"Power-fail recovery count";
    case SMART_HEAD_FLYING_HOURS:                        return L"Head flying hours";
    case SMART_TOTAL_LBA_WRITTEN:                        return L"Total LBA written";
    case SMART_TOTAL_LBA_READ:                           return L"Total LBA read";
    case SMART_NAND_WRITES:                              return L"Nang writes";
    case SMART_READ_ERROR_RETRY_RATE:                    return L"Read error retry rate";
    case SMART_FREE_FALL_EVENT_COUNT:                    return L"Free fall count";
    }
    return L"UNKNOWN";
}