#ifndef __INC_UBNT_FLASH_PROTECT_H__
#define __INC_UBNT_FLASH_PROTECT_H__

/* Configure GPIO for WP signal and enable it or disable it */
void ubnt_flash_hw_write_protect(int protect);
/* Determine if HW write protect signal is active or not (also depends on flash config) */
int ubnt_get_hw_write_protect(void);

/* Diagnostic information about flash protection */
void ubnt_flash_print_protection_status(void);
/* Configure flash with all blocks protected and */
void ubnt_flash_enable_protection(int hw_wp);
void ubnt_flash_disable_protection(int hw_wp);
int ubnt_get_bp(void);
int ubnt_get_srwp(void);
uint8_t ath_spi_read_status(void);
void ath_spi_write_status(uint8_t value);
void ubnt_flash_factory_reset_for_art(void);

#endif // #ifdef __INC_UBNT_FLASH_PROTECT_H__
