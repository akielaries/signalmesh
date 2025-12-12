/**
 * @file driver_api.h
 * @brief Defines a common driver interface
 */

typedef struct {
  /**
   * @brief driver name
   */
  /**
   *
   */
  /***
   * @brief device initialization
   *
   * @param[in] device_id
   */
  int (*init)();
  int (*activate)();
  int (*set_config)();
  int (*get_config)();
  int (*query)();
  int (*ioctl)();

} driver_t;
