cmake_minimum_required (VERSION 2.6)


set(rsyslog_dir "${SYSCONF_INSTALL_DIR}/rsyslog.d/")
set(logrotate_dir "${SYSCONF_INSTALL_DIR}/logrotate.d/")
set(conf_dir "${SYSCONF_INSTALL_DIR}/ugr.conf.d/")

IF(RSYSLOG_SUPPORT)
message(STATUS "rsyslog support enabled")

# install syslog scripts
install(FILES "conf/20-log-ugr.conf"
              DESTINATION "${rsyslog_dir}")
ENDIF(RSYSLOG_SUPPORT)


IF(LOGROTATE_SUPPORT)
message(STATUS "logrotate support enabled")

# install syslog scripts
install(FILES "conf/logrotate/ugr-server"
              DESTINATION "${logrotate_dir}")
ENDIF(LOGROTATE_SUPPORT)

# Install the config files
if (NOT EXISTS ${SYSCONF_INSTALL_DIR}/ugr.conf)
  install(FILES conf/ugr.conf
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${SYSCONF_INSTALL_DIR} 
  COMPONENT core)
endif()
if (NOT EXISTS ${conf_dir}/endpoints.conf)
  install(FILES ugr.conf.d/endpoints.conf
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${conf_dir}
  COMPONENT core)
endif()

